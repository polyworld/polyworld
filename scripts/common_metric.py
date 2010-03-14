import glob
import os

import common_functions
import datalib

METRIC_ROOT_TYPES = ['cc', 'npl', 'cpl', 'cl', 'nm', 'swi', 'hf']
METRIC_ROOT_NAMES = {'cc':'Clustering Coefficient', 'npl':'Normalized Path Length', 'cpl': 'Characteristic Path Length', 'cl':'Connectivity Length', 'swi':'Small World Index', 'hf':'Heuristic Fitness', 'nm':'Newman Modularity'}
DEFAULT_METRICS = []
METRICS_NO_GRAPH = ['hf']
PACKAGES = ['nx', 'bct']  # NetworkX or BCT
NEURON_SETS = ['a', 'p']  # all or processing
GRAPH_TYPES = ['bu', 'bd', 'wu', 'wd']  # binary/weighted, undirected/directed
LINK_TYPES = ['w', 'd']  # weight or distance
PRESERVATIONS = ['np', 'wp', 'dp']
RANDOM_PIECE = '_ran_'
FILENAME_AVR = 'AvrMetric.plt'
DEFAULT_NUMBINS = 11

METRIC_TYPES = []
for root in METRIC_ROOT_TYPES:
	for neuron_set in NEURON_SETS:
		for graph_type in GRAPH_TYPES:
			type = root + '_' + neuron_set + '_' + graph_type
			METRIC_TYPES.append(type)


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
    	    name += piece.upper() + ','
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
