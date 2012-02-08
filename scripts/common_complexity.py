import os
import sys

import datalib
import abstractfile

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
### FUNCTION path_avr()
###
####################################################################################
def path_avr(path_run, recent_type):
	return os.path.join(path_run, 'brain', recent_type, FILENAME_AVR)

####################################################################################
###
### FUNCTION parse_avr
###
####################################################################################
def parse_avr(run_path, recent_type = 'Recent', complexities = None):
	# parse the AVRs for all the runs

	return datalib.parse( path_avr(run_path, recent_type),
			      tablenames = complexities,
			      required = datalib.REQUIRED,
			      keycolname = 'Timestep' )
	
####################################################################################
###
### FUNCTION num_neurons
###
####################################################################################
def num_neurons(function_file):
	file = abstractfile.open(function_file, 'r')
	header = file.readline()
	if 'version' in header:
		header = file.readline()
	file.close()
	entries = header.split()
	return int(entries[2])

####################################################################################
###
### FUNCTION num_synapses
###
####################################################################################
def num_synapses(function_file):
	file = abstractfile.open(function_file, 'r')
	header = file.readline()
	if 'version' in header:
		header = file.readline()
		index = 5
	else:
		index = 4
	file.close()
	entries = header.split()
	return int(entries[index])
