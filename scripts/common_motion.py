import os

import datalib

COMPLEXITY_TYPES = ['Olaf']
COMPLEXITY_NAMES = {'Olaf':'Olaf'}
FILENAME_COMPLEXITY = 'Complexity.txt'

####################################################################################
###
### FUNCTION get_names()
###
####################################################################################
def get_names(types):
    return types

####################################################################################
###
### FUNCTION relpath_complexity()
###
####################################################################################
def relpath_complexity():
    return os.path.join('motion', FILENAME_COMPLEXITY)

####################################################################################
###
### FUNCTION path_complexity()
###
####################################################################################
def path_complexity(path_run):
    return os.path.join(path_run, relpath_complexity())

####################################################################################
###
### FUNCTION path_run_from_complexity()
###
####################################################################################
def path_run_from_complexity(path_complexity):
    suffix = relpath_complexity()

    return path_complexity[:-(len(suffix) + 1)]

####################################################################################
###
### FUNCTION parse_complexity
###
####################################################################################
def parse_complexity(run_paths, complexities, run_as_key = False):
    # parse the AVRs for all the runs
    tables = datalib.parse_all( map(lambda x: path_complexity( x ),
                                    run_paths),
                                complexities,
                                datalib.REQUIRED,
                                keycolname = 'EPOCH-START' )

    if run_as_key:
        # modify the map to use run dir as key, not Avr file
        tables = dict( [(path_run_from_complexity( x[0] ),
                         x[1])
                        for x in tables.items()] )

    return tables
