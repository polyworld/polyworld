import glob
import os

import abstractfile
import common_functions
import datalib

METRIC_ROOT_TYPES = ['cc', 'ccd', 'cpl', 'npl', 'nl', 'cl', 'ccl', 'eff', 'nm', 'lm', 'm3', 'swi', 'swid', 'swb', 'nc', 'ec', 'hf']
METRIC_ROOT_NAMES = {'cc':'Clustering Coefficient', 'ccd':'Clustering Coefficient [d]', 'npl':'Normalized Path Length', 'nl':'Normalized Length', 'cpl': 'Characteristic Path Length', 'ccl':'Capped Characteristic Path Length', 'cl':'Connectivity Length', 'eff':'Efficiency', 'swi':'Small World Index', 'swid':'Small World Index [d]', 'hf':'Heuristic Fitness', 'nm':'Newman Modularity', 'lm':'Louvain Modularity', 'nc':'Node Count', 'ec':'Edge Count', 'swb':'Small World Bias', 'm3':'3-Node Motifs'}
DEFAULT_METRICS = []
METRICS_NO_GRAPH = ['hf']
METRICS_NO_RANDOM = ['swi', 'swid', 'nc', 'ec', 'swb']
PACKAGES = ['nx', 'bct']  # NetworkX or BCT
NEURON_SETS = ['a', 'p']  # all or processing
GRAPH_TYPES = ['bu', 'bd', 'wu', 'wd']  # binary/weighted, undirected/directed
LINK_TYPES = ['w', 'd']  # weight or distance
PRESERVATIONS = ['np', 'wp', 'dp']
LENGTH_TYPES = ['cpl', 'npl', 'cl', 'ccl', 'nl']
RANDOM_PIECE = '_ran_'
FILENAME_AVR = 'AvrMetric.plt'
DEFAULT_NUMBINS = 11

sep = '|'
METRIC_TYPES = {}
for root in METRIC_ROOT_TYPES:
	type = root
	if root not in METRICS_NO_GRAPH:
		type += '_{'
		for neuron_set in NEURON_SETS:
			type += neuron_set + sep
		type = type[:-1] + '}_{'
		for graph_type in GRAPH_TYPES:
			type += graph_type + sep
		type = type[:-1] + '}'
		if root == 'swi' or root == 'swid':
			type += '_{'
			for length_type in LENGTH_TYPES:
				type += length_type + sep
			type = type[:-1] + '}_#_{'
			for preservation in PRESERVATIONS:
				type += preservation + sep
			type = type[:-1] + '}'
		elif root == 'swb':
			type += '_{'
			for length_type in LENGTH_TYPES:
				type += length_type + sep
			type = type[:-1] + '}'
		if root not in METRICS_NO_RANDOM:
			type += '[' + RANDOM_PIECE + '#_{'
			for preservation in PRESERVATIONS:
				type += preservation + sep
			type = type[:-1] + '}]'
	METRIC_TYPES[root] = type


####################################################################################
###
### FUNCTION get_root_type()
###
####################################################################################
def get_root_type(type):
    return type.split('_')[0]

####################################################################################
###
### FUNCTION get_name()
###
####################################################################################
def get_name(type):
    pieces = type.split('_')
    name = METRIC_ROOT_NAMES[pieces[0]]
    if len(pieces) > 1:
        name += ' ('
        for piece in pieces[1:]:
    	    name += piece + ','
    	name = name[:-1] + ')'
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
### FUNCTION normalize_metrics()
###
####################################################################################
def normalize_metrics(data):
    data = map(float, data)
    # ignore 0.0, since it is a critter that was ignored in the complexity
    # calculation due to short lifespan.
    #
    # also, must be sorted
    data = filter(lambda x: x != 0.0, data)
    data.sort()
    return data

####################################################################################
###
### FUNCTION get_random_classifications()
###
####################################################################################
def get_random_classifications(path_run, recent_type = None):
    if recent_type == None:
        classifications = []
        for recent_type in common_functions.RECENT_TYPES:
            classifications += get_random_classifications(path_run, recent_type)

        return classifications

    path = path_avr( path_run, recent_type )
    if os.path.exists( path ):
        digest = datalib.parse_digest( path )

        classifications = set()
        
        for name in digest['tables'].keys():
            index = name.find(RANDOM_PIECE)
            if index > -1:
                classifications.add( name[index + len(RANDOM_PIECE):] )

        return list( classifications )
    else:
        return []

####################################################################################
###
### FUNCTION path_avr()
###
####################################################################################
def path_avr(path_run, recent_type = "Recent"):
    return os.path.join(path_run, 'brain', recent_type, FILENAME_AVR)

####################################################################################
###
### FUNCTION normalize_metrics_names()
###
####################################################################################
def normalize_metrics_names( classification,
                             metrics ):
    if classification.startswith("Random"):
        metric_prefix = classification.replace('Random', 'ran')
        metrics = map(lambda x: x + '_' + metric_prefix,
                      metrics)

    return metrics

####################################################################################
###
### FUNCTION parse_avr()
###
####################################################################################
def parse_avr( run_path,
               recent_type = "Recent",
               metrics = None ): # None gives you all metrics.
    return datalib.parse( path_avr(run_path, recent_type),
			  tablenames = metrics,
			  required = datalib.REQUIRED,
			  keycolname = 'Timestep' )

####################################################################################
###
### FUNCTION read_anatomy()
###
### Reads a brain anatomy file and returns the all-neuron matrix (ga),
### the processing-neuron (non-sensory) matrix (gp), the maximum weight
### allowed in the network during evolution, and the file header.
### Neither ga nor gp contains the bias neuron or bias connections.
###
####################################################################################
def read_anatomy(anatomy_file):
	ga = []
	gp = []

	def __num_input_neurons(header):
		start = header.rfind('-') + 1
		if start > 1:  # new-style anatomy file that has the input neuron info
			return int(header[start:]) + 1  # +1 because the value in the header is a 0-based index, but we need a count
		else:  # old-style anatomy file that doesn't have input neuron info
			# /pwd/run_df8_F30_complexity_0_partial/brain/Recent/0/../../anatomy/brainAnatomy_1_death.txt
			brain_dir = anatomy_file[:anatomy_file.find('Recent')]
			agent_id = anatomy_file.split('_')[-2]
			function_filename = brain_dir + 'function/brainFunction_' + agent_id + '.txt'
			# don't bother with abstractfile since this is an old format
			function_file = open(function_filename, 'r')
			function_header = function_file.readline()
			function_file.close()
			return int(function_header.split(' ')[-2])

	def __max_weight(header):
		start = header.find('maxWeight')
		start += header[start:].find('=') + 1
		stop = start + header[start:].find(' ')
		max_weight = float(header[start:stop])
		return max_weight
	
	file = abstractfile.open(anatomy_file, 'r')
	lines = file.readlines()
	file.close()
	
	header = lines[0].rstrip()  # get rid of the newline
	num_input_neurons = __num_input_neurons(header)
	max_weight = __max_weight(header)

	lines.pop(0) # drop the header line

	for i in range(len(lines)-1):  # -1 to leave out the bias unit
		l = lines[i].split()
		l.remove(';')
		row_a = []
		row_p = []
		for j in range(len(l)-1):  # -1 to leave out the bias links
			if j == i:
				w = float(l[j])
				if w != 0.0:
					print 'self-connection in', anatomy_file, 'i = j =', i, 'w =', w
					w = 0.0
			else:
				w = abs(float(l[j]))
			row_a.append(w)
			if j >= num_input_neurons:
				row_p.append(w)
		ga.append(row_a)  # must agree with neuron-sets and graph-types in common_metric.py
		if i >= num_input_neurons:
			gp.append(row_p)  # must agree with neuron-sets and graph-types in common_metric.py
	
	# Transpose the matrixes so g[i][j] describes a connection from i to j (not j to i, as stored)
	for i in range(len(ga)):
		for j in range(i):
			temp = ga[i][j]
			ga[i][j] = ga[j][i]
			ga[j][i] = temp
	
	for i in range(len(gp)):
		for j in range(i):
			temp = gp[i][j]
			gp[i][j] = gp[j][i]
			gp[j][i] = temp

	return ga, gp, max_weight, header

