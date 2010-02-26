import glob
import os

import common_functions
import datalib

METRIC_TYPES = ['CC', 'SP', 'CP', 'SW', 'SC']
METRIC_NAMES = {'CC':'Clustering Coefficient', 'SP':'Normalized Shortest Path', 'CP': 'Characteristic Path Length', 'SW': 'Small World Index (NSP)', 'SC': 'Small World Index (CPL)'}
DEFAULT_METRICS = ['CC', 'SP', 'CP', 'SW', 'SC']
METRIC_FILENAME_AVR = 'AvrMetric.plt'
RANDOM_METRIC_FILENAME_AVR = 'AvrMetricRandom.plt'
DEFAULT_NUMBINS = 11

####################################################################################
###
### FUNCTION get_name()
###
####################################################################################
def get_name(type):
    result = []

##    for c in type:
##        result.append(METRIC_NAMES[c])
    result.append(METRIC_NAMES[type])
    return '+'.join(result)

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
        return os.path.join('brain', recent_type, RANDOM_METRIC_FILENAME_AVR)
    else:
        return os.path.join('brain', recent_type, METRIC_FILENAME_AVR)

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
    
