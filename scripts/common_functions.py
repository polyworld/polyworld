#!/usr/bin/env python
import os, re, sys
import glob
from subprocess import Popen, PIPE

import datalib

RECENT_TYPES = ['Recent', 'bestRecent']
DEBUG = False

# save this in case program stomps on argv later
__progpath = sys.argv[0]
__progname = os.path.basename(__progpath)

####################################################################################
###
### FUNCTION err()
###
####################################################################################
def err(msg, exit_value = 1):
	sys.stderr.write('%s: ERROR! %s\n' % (__progname, msg))
	sys.exit(exit_value)

####################################################################################
###
### FUNCTION warn()
###
####################################################################################
def warn(msg):
	sys.stderr.write('%s: WARNING! %s\n' % (__progname, msg))

####################################################################################
###
### FUNCTION verbose()
###
####################################################################################
_isverbose = False

def isverbose():
	return _isverbose

def verbose(msg):
	if _isverbose:
		print msg

####################################################################################
###
### FUNCTION classify_run()
###
####################################################################################
CLASSIFICATIONS = ['Driven', 'Passive', 'Fitness', 'Random']

# Key supercedes/preempts associated list
CLASSIFICATION_PREEMPTION = {'Fitness': ['Driven', 'Passive']}

class ClassificationError(Exception):
	def __init__(self, reason, message):
		super(Exception, self).__init__(message)

		self.reason = reason

def classify_run(path_run,
		 single_classification = False,
		 constraints = CLASSIFICATIONS):
	import common_metric
	import wfutil

	classifications = []

	def __nonzero(field):
		value = wfutil.get_parameter(path_run, field)
		return float(value) != 0

	def __not(field, notvalue):
		value = wfutil.get_parameter(path_run, field)
		return value != notvalue

	def __true(field):
		value = wfutil.get_parameter(path_run, field)
		return value == 'True'
		

	#
	# Fitness
	#
	if __nonzero("ComplexityFitnessWeight")	or __nonzero("HeuristicFitnessWeight"):
		classifications.append('Fitness')

	#
	# Driven/Passive
	#
	if __true("PassiveLockstep"):
		classifications.append('Passive')
	else:
		classifications.append('Driven')

	#
	# Random
	#
	classifications += map( lambda x: 'Random_' + x,
				common_metric.get_random_classifications(path_run) )
              
	#
	# Apply Constraints
	#
	if constraints != None:
		classifications_constrained = list_intersection( classifications,
							 constraints )
	else:
		classifications_constrained = classifications

	#
	# Apply Preemptions
	#
	for c in classifications_constrained:
		try:
			preempted = CLASSIFICATION_PREEMPTION[c]
			classifications_constrained = list_difference(classifications_constrained,
								      preempted)
		except KeyError:
			pass

	#
	# Validate Result
	#
	if len(classifications_constrained) == 0:
		if len(classifications) == 0:
			raise ClassificationError( 'notfound', path_run + ' cannot be classified' )
		else:
			raise ClassificationError( 'notfound', path_run + ' classification not in {' + ','.join(constraints) + '}' )

	if single_classification and len(classifications_constrained) > 1:
		raise ClassificationError( 'ambiguous', path_run + ' classification ambiguous (' + ','.join(classifications_constrained) + ')' )

	if single_classification:
		return classifications_constrained[0]
	else:
		return classifications_constrained

####################################################################################
###
### FUNCTION normalize_classification()
###
### Expand abbreviated classification name, and handle special classifications
### comprised of constituents/pieces delimited by '_'
###
### Example:
###
###  d        -->  Driven
###  r_10_np  -->  Random_10_np
###
####################################################################################
def normalize_classification( name,
			      classifications = CLASSIFICATIONS ):
	pieces = name.split( '_' )
	pieces[0] = expand_abbreviations( pieces[0],
					  classifications,
					  case_sensitive = False )
	return '_'.join( pieces )
	
def normalize_classifications( names,
			       classifications = CLASSIFICATIONS ):
	return map( lambda name: normalize_classification(name, classifications),
		    names )

####################################################################################
###
### FUNCTION friendly_classification()
###
####################################################################################
def friendly_classification( name ):
	parts = name.split('_')
	if len(parts) == 1:
		return name
	else:
		return parts[0] + ' (' + ','.join(parts[1:]) + ')'


####################################################################################
###
### FUNCTION get_timesteps()
###
####################################################################################
def get_timesteps(iter_tables, colname_timestep):
	timesteps = {}
	n = 0

	for table in iter_tables:
		n += 1
		for t in table.getColumn(colname_timestep):
			try:
				timesteps[t] += 1
			except KeyError:
				timesteps[t] = 1

	ret = [item[0] for item in timesteps.items() if item[1] == n]
	ret.sort()

	return ret


####################################################################################
###
### FUNCTION get_equals_decl()
###
####################################################################################
def get_equals_decl(line, name):
    regex = '\\b'+name+'\\b\\s*=\\s*(\\S+)'

    result = re.search(regex, line)
    if result: return result.group(1)
    else: return None

####################################################################################
###
### FUNCTION get_version()
###
####################################################################################
def get_version(line):
    version = get_equals_decl(line, 'version')
    if version:
        return int(version)
    else:
        return 0

####################################################################################
###
### FUNCTION expand_macros()
###
####################################################################################
def expand_macros(text, macros):
    return ''.join(__expand_macros(text, macros, []))

def __expand_macros(text, macros, ignore):
    tokens = re.split('({\s*\w+\s*})', text)

    result = []
    
    for tok in tokens:
        expanded = False

        if len(tok) > 0 and tok[0] == '{' and tok[-1] == '}':
            macroname = tok[1:-1].strip()
            if not macroname in ignore: # avoid infinite recursion
                expanded = True
                expansion = macros[macroname]
                result += __expand_macros(expansion, macros, ignore + [macroname])

        if not expanded:
            result.append(tok)

    return result

####################################################################################
###
### FUNCTION expand_abbreviations()
###
####################################################################################
class IllegalAbbreviationError(Exception):
	pass

def expand_abbreviations(abbreviations,
			 full,
			 case_sensitive = True):
	islist = True

	if isinstance(abbreviations, str):
		islist = False
		abbreviations = [abbreviations]

	result = []

	for a in abbreviations:
		match = None
		na = len(a)

		for f in full:
			nf = len(f)
			if na <= nf:
				failed = False
				for i in range(na):
					if case_sensitive:
						if a[i] != f[i]:
							failed = True
							break
					else:
						if a[i].upper() != f[i].upper():
							failed = True
							break;
				if not failed:
					if match:
						raise IllegalAbbreviationError( a+' is ambiguous ('+match+','+f+')' )
					else:
						match = f
		if not match:
			raise IllegalAbbreviationError( "Illegal abbreviation '"+a+"' (choices="+','.join(full)+")" )
		else:
			result.append(match)

	if islist:
		return result
	else:
		return result[0]

####################################################################################
###
### FUNCTION find_affix()
###
####################################################################################
def find_affix(names, direction):
    if len(names) < 2:
        return ""

    if direction < 0:
        start = -1
    else:
        start = 0

    n = min(map(len, names))
    for i in range(n):
        idx = start + (i * direction)
        c = names[0][idx]
        for j in (1, len(names) - 1):
            if c != names[j][idx]:
                if direction > 0:
                    return names[0][:i]
                else:
                    if idx == -1:
                        return ""
                    else:
                        return names[0][idx + 1:]

    # we fell through, meaning that one of the names is contained in all the
    # others (e.g. ['a', 'ab', 'abc'])
    return ''

####################################################################################
###
### FUNCTION find_prefix()
###
####################################################################################
def find_prefix(names):
    return find_affix(names, 1)

####################################################################################
###
### FUNCTION find_suffix()
###
####################################################################################
def find_suffix(names):
    return find_affix(names, -1)

####################################################################################
###
### FUNCTION get_unique()
###
####################################################################################
def get_unique(names):
    npre = len(find_prefix(names))
    nsuf = len(find_suffix(names))

    if nsuf == 0:
        return map(lambda x: x[npre:], names)
    else:
        return map(lambda x: x[npre: -nsuf], names)

####################################################################################
###
### FUNCTION truncate_paths()
###
####################################################################################
def truncate_paths(paths):
    paths = map(lambda x: x.rstrip('/'), paths)
    dirs = []
    files = []

    for path in paths:
        dirs.append(os.path.dirname(path))
	files.append(os.path.basename(path))

    files = get_unique(files)
    files = map(lambda x: x.strip('_'), files)

    paths = []

    for i in range(len(files)):
        paths.append( (dirs[i], {'i': i, 'name': files[i]}) )

    result = {}

    __truncate_paths(paths, result)

    return [result[i] for i in range(len(paths))]

def __truncate_paths(paths, result):
    accumulate = {}

    for path in paths:
        dir = path[0]
        name = path[1]['name']
	i = path[1]['i']
	entry = {'i': i, 'dir': dir}
        try:
            accumulate[name].append(entry)
        except KeyError:
            accumulate[name] = [entry]

    for name, entries in accumulate.items():
        if len(entries) == 1:
            entry = entries[0]		
            result[entry['i']] = name
        else:
            paths = []
            for entry in entries:
                dir = entry['dir']
		i = entry['i']
                basename = os.path.basename(dir)
		dirname = os.path.dirname(dir)
		newname = os.path.join(basename, name)
                paths.append( (dirname, {'i': i, 'name': newname}) )
            
            __truncate_paths(paths, result)

    return result

#################################################
###
### FUNCTION get_cmd_stdout()
###
### executes cmd with stderr to console
###
###   returns exit value, stdout as string
###
### ex: rc, out = get_cmd_stdout(['ls','-l'])
###
#################################################
def get_cmd_stdout(cmd):
	cmd = map( str, cmd )

	proc = Popen(cmd, stdout=PIPE)
	
	stdout, stderr = proc.communicate()

	return proc.returncode, stdout


#################################################
###
### FUNCTION env_check()
###
### executes env_check.sh
###
###   returns stdout
###
#################################################
def env_check(args):
	program = os.path.abspath(__progpath)

	try:
		dir = os.getenv('PW_SCRIPTS')
	except:
		pass

	if dir == None:
		dir = os.path.dirname(program)

	env_check = dir + '/env_check.sh'

	cmd = [env_check, program]
	cmd += args

	rc, stdout = get_cmd_stdout(cmd)
							       
	if(rc != 0):
		sys.exit(rc)
	
	return stdout.strip();

#################################################
###
### FUNCTION pw_env()
###
#################################################
def pw_env(name):
	return env_check(['--echo', name])

#################################################
###
### CLASS InvalidDirError
###
#################################################
class InvalidDirError(Exception):
	def __init__(self, path, type = None, required_subpath = None):
		self.type = type

		if type:
			type += ' '
		else:
			type = ''

		details = ' -- require %s' % required_subpath if required_subpath else ''

		Exception.__init__(self, 'Invalid %sdir (%s)%s' % (type, path, details))

####################################################################################
###
### FUNCTION isrundir()
###
### Tries to determine if a directory is a run dir by making sure it doesn't have
### some signature files from the home dir and contains important signature files
### from a run dir.
###
####################################################################################
def isrundir(path):
	import wfutil

	if os.path.exists( os.path.join(path, 'docs') ) and os.path.exists( os.path.join(path, 'scripts') ):
		return False
# 	if not os.path.exists( os.path.join(path, 'stats') ):
# 		return False
	if os.path.exists( wfutil.path_worldfile(path, legacy = False) ):
		return True
	if os.path.exists( wfutil.path_worldfile(path, legacy = True) ):
		return True
	return False

####################################################################################
###
### FUNCTION get_endStep()
###
####################################################################################
def get_endStep(path_run):
	path_endStep = os.path.join( path_run, 'endStep.txt' )
	f = open( path_endStep )
	return int( f.readline() )

####################################################################################
###
### FUNCTION find_run_paths()
###
####################################################################################
def find_run_paths(paths_arg, required_subpath = None):
	import farm

	def __isrundir( path ):
		if required_subpath == None:
			return isrundir( path )
		else:
			path = os.path.join( path, required_subpath )
			return len( glob.glob(path) ) > 0

	run_paths = []

	for path_arg in paths_arg:
		if not os.path.isdir(path_arg):
			if farm.is_valid_env():
				farm_runs = filter( __isrundir, farm.find_runs_local( path_arg ) )
				if len(farm_runs):
					run_paths += farm_runs
				else:
					raise InvalidDirError(path_arg)
			else:
				raise InvalidDirError(path_arg)
		else:
			if os.path.basename( path_arg ) == 'results':
				continue

			# if 'directory' is itself a run/ directory, just use that.
			if __isrundir(path_arg):
				run_paths.append(path_arg)
			else:
				found_run = False

				# 'directory' is a directory, but is NOT a run/ directory itself.  Is it a list of run directories?
				for potential_runpath in os.listdir(path_arg):
					subdir = os.path.join(path_arg, potential_runpath)
				
					# if potential_directory is a run/ directory, add it.
					if __isrundir(subdir):
						run_paths.append(subdir)
						found_run = True

				if not found_run:
					raise InvalidDirError(path_arg, 'run/run-parent', required_subpath)

	run_paths.sort()

	return map(lambda x: x.rstrip('/'), run_paths)

####################################################################################
###
### FUNCTION get_common_ancestor()
###
####################################################################################
def get_common_ancestor( paths ):
  assert( len(paths) )
  assert( os.path.sep == '/' )

  paths = map( os.path.realpath, paths )
  elements_matrix = [ path.split('/') for path in paths ]
  nelements = min( map(len, elements_matrix) )

  # elements of first path
  elements_first = elements_matrix[0]
  # elements of all other paths
  elements_matrix = elements_matrix[1:]

  for i in range(nelements):
    match = True
    for elements in elements_matrix:
      if elements[i] != elements_first[i]:
        match = False
        break

    if not match:
      i -= 1
      break

  result = elements_first[:i+1]
  # first element is just ''
  result[0] = '/'

  return os.path.join( *result )

####################################################################################
###
### FUNCTION get_results_dir()
###
####################################################################################
def get_results_dir( run_paths, make = True ):
	dir = os.path.join( get_common_ancestor(run_paths), 'results' )
	if make and not os.path.exists(dir):
		os.makedirs(dir)

	return dir

####################################################################################
###
### FUNCTION list_difference()
###
### returns list of elements in a but not in b
###
####################################################################################
def list_difference(a, b):
	return filter(lambda x: x not in b, a)

####################################################################################
###
### FUNCTION list_intersection()
###
### returns list of elements in both a and b
###
####################################################################################
def list_intersection(a, b):
	return filter(lambda x: x in b, a)

####################################################################################
###
### FUNCTION list_division()
###
### returns list of elements of a divided by elements of b
###
####################################################################################
def list_division(a, b):
	assert(len(a) == len(b))
	c = []
	for i in range(len(a)):
		if b[i] == 0:
			c.append(0.0)
		else:
			c.append(float(a[i])/float(b[i]))
	return c

####################################################################################
###
### FUNCTION list_subtraction()
###
### returns list of elements of b subtracted from elements of a
###
####################################################################################
def list_subtraction(a, b):
	assert(len(a) == len(b))
	c = []
	for i in range(len(a)):
		c.append(a[i]-b[i])
	return c

####################################################################################
###
### FUNCTION list_zscore()
###
### returns list of zscores of elements of a relative to elements of b with std dev from c
###
####################################################################################
def list_zscore(a, b, c):
	assert(len(a) == len(b) == len(c))
	z = []
	for i in range(len(a)):
		if c[i] == 0:
			z.append(0.0)
		else:
			z.append((a[i]-b[i])/float(c[i]))
	return z

####################################################################################
###
### FUNCTION list_unique()
###
### returns order-preserving list of unique elements in list a
###
####################################################################################
def list_unique(a):
	b = [] 
	[b.append(i) for i in a if i not in b] 
	return b

####################################################################################
###
### FUNCTION debug()
###
### works like print(), but can be toggled on and off via common_functions.DEBUG
###
####################################################################################
def debug(*args):
	if DEBUG:
		for arg in args:
			print arg,
		print

####################################################################################
###
### FUNCTION print_matrix()
###
####################################################################################
def print_matrix(m, fmt):
	for row in m:
		for col in row:
			print fmt % (col),
		print

