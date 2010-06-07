import glob
import os

import common_functions
import datalib

METRIC_ROOT_TYPES = ['cc', 'ccd', 'cpl', 'npl', 'cl', 'nm', 'lm', 'm3', 'swi', 'swid', 'swb', 'nc', 'ec', 'hf']
METRIC_ROOT_NAMES = {'cc':'Clustering Coefficient', 'ccd':'Clustering Coefficient [d]', 'npl':'Normalized Path Length', 'cpl': 'Characteristic Path Length', 'cl':'Connectivity Length', 'swi':'Small World Index', 'swid':'Small World Index [d]', 'hf':'Heuristic Fitness', 'nm':'Newman Modularity', 'lm':'Louvain Modularity', 'nc':'Node Count', 'ec':'Edge Count', 'swb':'Small World Bias', 'm3':'3-Node Motifs'}
DEFAULT_METRICS = []
METRICS_NO_GRAPH = ['hf']
METRICS_NO_RANDOM = ['swi', 'swid', 'nc', 'ec', 'swb']
PACKAGES = ['nx', 'bct']  # NetworkX or BCT
NEURON_SETS = ['a', 'p']  # all or processing
GRAPH_TYPES = ['bu', 'bd', 'wu', 'wd']  # binary/weighted, undirected/directed
LINK_TYPES = ['w', 'd']  # weight or distance
PRESERVATIONS = ['np', 'wp', 'dp']
LENGTH_TYPES = ['cpl', 'npl', 'cl']
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

    path = path_avr( path_run, None, recent_type )
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
### FUNCTION relpath_avr()
###
####################################################################################
def relpath_avr(classification,
                recent_type):
    return os.path.join('brain', recent_type, FILENAME_AVR)

####################################################################################
###
### FUNCTION path_avr()
###
####################################################################################
def path_avr(path_run,
             classification,
             recent_type):
    return os.path.join(path_run, relpath_avr(classification, recent_type))

####################################################################################
###
### FUNCTION path_run_from_avr()
###
####################################################################################
def path_run_from_avr(path_avr,
                      classification,
                      recent_type):
    suffix = relpath_avr(classification,
                         recent_type)

    return path_avr[:-(len(suffix) + 1)]

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
### FUNCTION parse_avrs()
###
####################################################################################
def parse_avrs(run_paths,
               classification,
               recent_type,
               metrics,
               run_as_key = False):

    # parse the AVRs for all the runs

    avrs = datalib.parse_all( map(lambda x: path_avr( x, classification, recent_type ),
                                  run_paths),
                              metrics,
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
### FUNCTION read_anatomy()
###
### Reads a brain anatomy file and returns the all-neuron matrix (ga),
### the processing-neuron (non-sensory) matrix (gp), and the file header.
### Neither ga nor gp contains the bias neuron or bias inputs.
###
####################################################################################
def read_anatomy(anatomy_file):
	ga = []
	gp = []

	def __num_input_neurons(header):
		start = header.rfind('-') + 1
		return int(header[start:]) + 1  # +1 because the value in the header is the last 0-based index, but we need a count
	
	file = open(anatomy_file, 'r')
	lines = file.readlines()
	header = lines[0].rstrip()  # get rid of the newline
	num_input_neurons = __num_input_neurons(header)

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

	return ga, gp, header

