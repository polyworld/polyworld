#!/usr/bin/python
import os, re, sys
from subprocess import Popen, PIPE

import common_metric
import datalib
import glob

RECENT_TYPES = ['Recent', 'bestRecent']

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
### FUNCTION read_worldfile_parameter()
###
### Return a value from the worldfile parameter.  If it is not found the value is False
###
####################################################################################
def read_worldfile_parameter( path_worldfile_or_run, parameter ):
	if os.path.isdir(path_worldfile_or_run):
		worldfile = path_worldfile(path_worldfile_or_run)
	else:
		worldfile = path_worldfile_or_run

	value = None

	if not os.path.isfile( worldfile ):
		print "ERROR: '%s' was not a worldfile" % worldfile
		sys.exit(1)
	worldfile = open( worldfile )

	for line in worldfile:
		line = line.split('_')[0].rstrip('\n') # '_' infomrally functions as comment
		if line.endswith(parameter):
			value = line.split()[0]
			break

	worldfile.close()
	return value

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
	classifications = []

	def __nonzero(field):
		value = read_worldfile_parameter(path_run,
						 field)
		return value != None and float(value) != 0

	def __not(field, notvalue, default = None):
		value = read_worldfile_parameter(path_run,
						 field)
		if value == None:
			if default == None:
				value = notvalue
			else:
				value = default

		return value != notvalue

	#
	# Fitness
	#
	if __nonzero("complexityFitnessWeight")	or __nonzero("heuristicFitnessWeight") or __not("useComplexityAsFitness", "O"):
		classifications.append('Fitness')

	#
	# Driven/Passive
	#
	if __nonzero("LockStepWithBirthsDeathsLog"):
		classifications.append('Passive')
	else:
		classifications.append('Driven')

	#
	# Random
	#
	if common_metric.has_random( path_run ):
		if ( ('Driven' in constraints and 'Driven' in classifications) or
			 ('Passive' in constraints and 'Passive' in classifications) or
			 ('Driven' not in constraints and 'Passive' not in constraints) ):
			classifications.append('Random')
              
	#
	# Apply Constraints
	#
	classifications_constrained = list_intersection( classifications,
							 constraints )

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

	return classifications_constrained

####################################################################################
###
### FUNCTION classify_runs()
###
####################################################################################
def classify_runs(run_paths,
		  single_classification = False,
		  constraints = CLASSIFICATIONS,
		  func_notfound = None):

	result = {}

	for path in run_paths:
		try:
			classifications = classify_run( path,
							single_classification,
							constraints )

			for c in classifications:
				try:
					result[c].append(path)
				except KeyError:
					result[c] = [path]

		except ClassificationError, x:
			if x.reason == 'notfound':
				if func_notfound == None:
					raise x
				else:
					func_notfound(path, x)
			else:
				raise x

	return result

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
			raise IllegalAbbreviationError( "Illegal value '"+a+"' (valid="+','.join(full)+")" )
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
### FUNCTION find_run_paths()
###
####################################################################################
def find_run_paths(paths_arg, required_subpath):
    def __path_required(dir):
	    return os.path.join(dir, required_subpath)

    def __exists(path):
	    return len(glob.glob(path)) > 0

    run_paths = []

    for path_arg in paths_arg:
        if not os.path.isdir(path_arg):
            raise InvalidDirError(path_arg)

        # if 'directory' is itself a run/ directory, just use that.
        if __exists(__path_required(path_arg)):
            run_paths.append(path_arg)
        else:
            found_run = False

            # 'directory' is a directory, but is NOT a run/ directory itself.  Is it a list of run directories?
            for potential_runpath in os.listdir(path_arg):
                subdir = os.path.join(path_arg, potential_runpath)
                
                # if potential_directory is a run/ directory, add it.
                if __exists(__path_required(subdir)):
                    run_paths.append(subdir)
                    found_run = True

            if not found_run:
                raise InvalidDirError(path_arg, 'run/run-parent', required_subpath)

    run_paths.sort()

    return map(lambda x: x.rstrip('/'), run_paths)

####################################################################################
###
### FUNCTION path_worldfile()
###
####################################################################################
def path_worldfile(path_run):
	return os.path.join(path_run, "worldfile")

####################################################################################
###
### FUNCTION list_difference()
###
### returns list of elements in a but not in b
###
####################################################################################
def list_difference(a, b):
	return filter(lambda x: x not in b,
		      a)

####################################################################################
###
### FUNCTION list_intersection()
###
### returns list of elements in both a and b
###
####################################################################################
def list_intersection(a, b):
	return filter(lambda x: x in b,
		      a)
