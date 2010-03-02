import glob
import os

import common_functions
import datalib

METRIC_ROOT_TYPES = ['cc', 'nsp', 'cpl', 'swi', 'hf', 'nm']
METRIC_ROOT_NAMES = {'cc':'Clustering Coefficient', 'nsp':'Normalized Shortest Path', 'cpl': 'Characteristic Path Length', 'swi': 'Small World Index', 'hf':'Heuristic Fitness', 'nm':'Newman Modularity'}
METRIC_TYPES = ['cc_a_bu', 'nsp_a_bu', 'cpl_a_bu', 'swi_nsp_a_bu', 'swi_cpl_a_bu', 'hf', 'nm_a_wd']
DEFAULT_METRICS = ['cc_a_bu', 'nsp_a_bu', 'cpl_a_bu', 'swi_nsp_a_bu', 'swi_cpl_a_bu', 'hf', 'nm_a_wd']
NETWORKX_METRICS = ['cc_a_bu', 'nsp_a_bu', 'cpl_a_bu']
BCT_METRICS = ['nm_a_wd']
METRICS_NON_RANDOM = ['swi', 'swi', 'hf']
FILENAME_AVR = 'AvrMetric.plt'
RANDOM_FILENAME_AVR = 'AvrMetricRandom.plt'
DEFAULT_NUMBINS = 11


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
### FUNCTION has_random()
###
####################################################################################
def has_random(path_run, recent_type = None):
    if recent_type == None:
        for recent_type in common_functions.RECENT_TYPES:
            if has_random(path_run, recent_type):
                return True
        return False

    return os.path.exists( path_avr(path_run,
                                    'Random',
                                    recent_type) )

####################################################################################
###
### FUNCTION relpath_avr()
###
####################################################################################
def relpath_avr(classification,
                recent_type):
    if classification == 'Random':
        return os.path.join('brain', recent_type, RANDOM_FILENAME_AVR)
    else:
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
    
