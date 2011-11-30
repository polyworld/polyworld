import os
import sys

import datalib

COMPLEXITY_TYPES = ['A', 'P', 'I', 'B', 'HB']
COMPLEXITY_NAMES = {'A':'All', 'P':'Processing', 'I':'Input', 'B':'Behavior', 'H':'Health'}
DEFAULT_COMPLEXITIES = ['P']
FILENAME_AVR = 'AvrComplexity.plt'
DEFAULT_NUMBINS = 11

####################################################################################
###
### FUNCTION calc_script()
###
### Invoke the CalcComplexity.py script
###
####################################################################################
def calc_script( args ):
	
	script = os.path.join( os.path.dirname(sys.argv[0]), 'CalcComplexity.py' )
	
	cmd = 'CalcComplexity.py' + ' ' + ' '.join( map(lambda x: '"%s"' % x, args) )
	rc = os.system( cmd )

	if rc != 0:
		raise Exception( 'CalcComplexity.py failed' )

####################################################################################
###
### FUNCTION get_name()
###
####################################################################################
def get_name(type):
	result = []
	num_points = ''
	filter_events = ''
	
	for c in type:
		if c.isdigit():
			num_points += c
		elif c.islower():
			filter_events += c
		else:
			result.append(COMPLEXITY_NAMES[c])

	if num_points == '':
		precision = ''
	elif num_points == '0':
		precision = 'Full '
	elif num_points == '1':
		precision = ''
	else:
		precision = num_points + '-point '
	
	if filter_events:
		filter_events += '-filtered '
	
	name = filter_events + precision + '+'.join(result)

	return name

####################################################################################
###
### FUNCTION get_names()
###
####################################################################################
def get_names(types):
	return map(lambda x: get_name(x), types)

####################################################################################
###
### FUNCTION normalize_complexities()
###
####################################################################################
def normalize_complexities(data):
	data = map(float, data)
	# ignore 0.0, since it is an agent that was ignored in the complexity
	# calculation due to short lifespan.
	#
	# also, must be sorted
	data = filter(lambda x: x != 0.0 and str(x) != 'nan', data)
	data.sort()
	return data

####################################################################################
###
### FUNCTION parse_legacy_complexities()
###
####################################################################################
def parse_legacy_complexities(path):

	f = open(path, 'r')

	complexities = []

	for line in f:
		complexities.append(float(line.strip()))

	f.close()

	return complexities

####################################################################################
###
### FUNCTION relpath_avr()
###
####################################################################################
def relpath_avr(classification, recent_type):
	return os.path.join('brain', recent_type, FILENAME_AVR)

####################################################################################
###
### FUNCTION path_avr()
###
####################################################################################
def path_avr(path_run, classification, recent_type):
	return os.path.join(path_run, relpath_avr(classification, recent_type))

####################################################################################
###
### FUNCTION path_run_from_avr()
###
####################################################################################
def path_run_from_avr(path_avr, classification, recent_type):
	suffix = relpath_avr(classification, recent_type)

	return path_avr[:-(len(suffix) + 1)]

####################################################################################
###
### FUNCTION parse_avrs
###
####################################################################################
def parse_avrs(run_paths, classification, recent_type, complexities, run_as_key = False):
	# parse the AVRs for all the runs
	print 'run_paths =', run_paths, 'classification =', classification, 'recent_type =', recent_type, 'complexities =', complexities, 'run_as_key =', run_as_key
	avrs = datalib.parse_all( map(lambda x: path_avr( x, classification, recent_type ),
								  run_paths),
							  complexities,
							  datalib.REQUIRED,
							  keycolname = 'Timestep' )

	if run_as_key:
		# modify the map to use run dir as key, not Avr file
		avrs = dict( [(path_run_from_avr( x[0], classification, recent_type ),
					   x[1])
					  for x in avrs.items()] )

	return avrs
	
####################################################################################
###
### FUNCTION num_neurons
###
####################################################################################
def num_neurons(function_file):
	file = open(function_file, 'r')
	header = file.readline()
	file.close()
	entries = header.split()
	return int(entries[2])
