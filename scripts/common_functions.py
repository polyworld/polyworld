#!/usr/bin/python
import os, re, sys
from subprocess import Popen, PIPE
import datalib

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
### FUNCTION read_worldfile_parameter()
###
### Return a value from the worldfile parameter.  If it is not found the value is False
###
####################################################################################
def read_worldfile_parameter( worldfilename, parameter ):
#	print "worldfilename='%s'  parameter='%s'" % ( worldfilename, parameter )
	value = False

	if not os.path.isfile( worldfilename ):
		print "ERROR: '%s' was not a worldfile"
		sys.exit(1)
	worldfile = open( worldfilename )

	for line in worldfile:
		line = line.rstrip('\n')
		if line.endswith(parameter):
			value = line.split()[0]
			break

	worldfile.close()
	return value

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

    paths = []

    for i in range(len(files)):
        paths.append((dirs[i], files[i]))

    return __truncate_paths(paths)

def __truncate_paths(paths):
    accumulate = {}

    for path in paths:
        try:
            accumulate[path[1]].append(path[0])
        except KeyError:
            accumulate[path[1]] = [path[0]]

    result = []

    for suffix, dirs in accumulate.items():
        if len(dirs) == 1:
            result.append(suffix)
        else:
            paths = []
            for dir in dirs:
                basename = os.path.basename(dir)
                paths.append((os.path.dirname(dir), os.path.join(basename, suffix)))
            
            result += __truncate_paths(paths)

    return result

####################################################################################
###
### FUNCTION get_median()
###
### In addition to returning the median, get_median() also returns the upper and
### lower half of the list.
###
### IMPORTANT: This function ASSUMES THE LIST IS ALREADY SORTED.
###
####################################################################################
def get_median( listofnumbers ):

	length=len(listofnumbers)
	lenover2=int(length / 2)

	middle1=listofnumbers[ lenover2 ]			# this number is the answer if the length is ODD, and half of the answer is the length is EVEN
	lowerhalf=listofnumbers[: lenover2 ]	# first half of the numbers

		
	if length % 2 == 0:			# if the length of the list is an EVEN number
		upperhalf=listofnumbers[ lenover2 :]
		middle2=listofnumbers[ (lenover2 - 1) ]
		median = (middle1 + middle2) / 2.0
	else:						# the length of the list is an ODD number, so simply return the middle number.
		upperhalf=listofnumbers[(lenover2+1) :]	# second half of the numbers
		median = middle1
	
	return median, lowerhalf, upperhalf

####################################################################################
###
### FUNCTION sample_mean()
###
####################################################################################
def sample_mean( list ):
        N = float(len(list))
        mean = sum(list) / N
        SSE=0
        for item in list:
                SSE += (item - mean)**2.0

        try: variance = SSE / (N-1)
        except: variance = 0

        stderr = ( variance ** 0.5 ) / (N**0.5) # stderr = stddev / sqrt(N)

        return mean, stderr

####################################################################################
###
### FUNCTION compute_avr()
###
### Warning: This assumes the data is sorted
###
####################################################################################
def compute_avr(DATA, timesteps, regions):
	colnames = ['Timestep', 'min', 'q1', 'median', 'q3', 'max', 'mean', 'mean_stderr', 'sampsize']
	coltypes = ['int', 'float', 'float', 'float', 'float', 'float', 'float', 'float', 'int']

	tables = {}

	for region in regions:
		  table = datalib.Table(region, colnames, coltypes)
		  tables[region] = table

		  for t in timesteps:
			regiondata = DATA[t][region];
			assert(len(regiondata) > 1)

			try:
				minimum = regiondata[0]
				maximum = regiondata[-1]

				# sanity check (data should already be sorted)
				assert(minimum <= maximum)

				mean, mean_stderr = sample_mean(regiondata)

				median, lowerhalf, upperhalf=get_median(regiondata)

				q1=get_median( lowerhalf )[0]
				q3=get_median( upperhalf )[0]
			except ValueError:
				minimum, maximum, mean, mean_stderr, q1, q3, median = (0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0)

			row = table.createRow()
			row.set('Timestep', t)
			row.set('min', minimum)
			row.set('max', maximum)
			row.set('mean', mean)
			row.set('mean_stderr', mean_stderr)
			row.set('median', median)
			row.set('q1', q1)
			row.set('q3', q3)
			row.set('sampsize', len(regiondata))

	return tables

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
	except KeyError:
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
	def __init__(self, path, type = None):
		self.type = type

		if type:
			type += ' '
		else:
			type = ''

		Exception.__init__(self, 'Invalid %sdir (%s)' % (type, path))

####################################################################################
###
### FUNCTION find_run_paths()
###
####################################################################################
def find_run_paths(paths_arg, required_subpath):
    def __path_required(dir):
	    return os.path.join(dir, required_subpath)

    run_paths = []

    for path_arg in paths_arg:
        if not os.path.isdir(path_arg):
            raise InvalidDirError(path_arg)

        # if 'directory' is itself a run/ directory, just use that.
	print __path_required(path_arg)
        if os.path.exists(__path_required(path_arg)):
            run_paths.append(path_arg)
        else:
            found_run = False

            # 'directory' is a directory, but is NOT a run/ directory itself.  Is it a list of run directories?
            for potential_runpath in os.listdir(path_arg):
                subdir = os.path.join(path_arg, potential_runpath)
                
                # if potential_directory is a run/ directory, add it.
                if os.path.exists(__path_required(subdir)):
                    run_paths.append(subdir)
                    found_run = True

            if not found_run:
                raise InvalidDirError(path_arg, 'run/run-parent')

    return map(lambda x: x.rstrip('/'), run_paths)

####################################################################################
###
### FUNCTION list_difference()
###
### returns list of elements in a but not in b
###
####################################################################################
def list_difference(a, b):
	return filter(lambda x:x not in b, a)
