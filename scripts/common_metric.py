import os

import datalib

COMPLEXITY_TYPES = ['CC', 'SP']
COMPLEXITY_NAMES = {'CC':'Clustering Coefficient', 'SP':'Shortest Path'}
DEFAULT_METRICS = ['CC', 'SP']
FILENAME_AVR = 'AvrMetric.plt'
DEFAULT_NUMBINS = 11

####################################################################################
###
### FUNCTION get_name()
###
####################################################################################
def get_name(type):
    result = []

##    for c in type:
##        result.append(COMPLEXITY_NAMES[c])
    result.append(COMPLEXITY_NAMES[type])
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
### FUNCTION normalize_complexities()
###
####################################################################################
def normalize_complexities(data):
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
def relpath_avr(recent_type):
    return os.path.join('brain', recent_type, FILENAME_AVR)

####################################################################################
###
### FUNCTION path_avr()
###
####################################################################################
def path_avr(path_run, recent_type):
    return os.path.join(path_run, relpath_avr(recent_type))

####################################################################################
###
### FUNCTION path_run_from_avr()
###
####################################################################################
def path_run_from_avr(path_avr, recent_type):
    suffix = relpath_avr(recent_type)

    return path_avr[:-(len(suffix) + 1)]

####################################################################################
###
### FUNCTION parse_avrs
###
####################################################################################
def parse_avrs(run_paths, recent_type, complexities, run_as_key = False):
    # parse the AVRs for all the runs
    avrs = datalib.parse_all( map(lambda x: path_avr( x, recent_type ),
                                  run_paths),
                              complexities,
                              datalib.REQUIRED,
                              keycolname = 'Timestep' )

    if run_as_key:
        # modify the map to use run dir as key, not Avr file
        avrs = dict( [(path_run_from_avr( x[0], recent_type ),
                       x[1])
                      for x in avrs.items()] )

    return avrs
    
