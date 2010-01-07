import os
import re
import sys

import datalib
import glob

STAT_TYPES = ['agents', 'food', 'born', 'created', 'died', 'LifeSpan', 'CurNeurons', 'NeurGroups', 'CurNeurGroups']
FILENAME_DATALIB = 'datalib.txt'

####################################################################################
###
### FUNCTION get_names()
###
####################################################################################
def get_names(types):
    return types

####################################################################################
###
### FUNCTION relpath_stats()
###
####################################################################################
def relpath_stats():
    return os.path.join('stats', FILENAME_DATALIB)

####################################################################################
###
### FUNCTION path_stats()
###
####################################################################################
def path_stats(path_run):
    return os.path.join(path_run, relpath_stats())

####################################################################################
###
### FUNCTION path_run_from_complexity()
###
####################################################################################
def path_run_from_stats(path_stats):
    suffix = relpath_stats()

    return path_stats[:-(len(suffix) + 1)]

####################################################################################
###
### FUNCTION parse_complexity
###
####################################################################################
def parse_stats(run_paths, types, run_as_key = False):
    # make sure the datalib files exist
    for path in run_paths:
        __get_stats( path )

    # parse the stats for all the runs
    tables = datalib.parse_all( map(lambda x: path_stats( x ),
                                    run_paths),
                                types,
                                datalib.REQUIRED,
                                keycolname = 'step' )

    if run_as_key:
        # modify the map to use run dir as key, not Avr file
        tables = dict( [(path_run_from_stats( x[0] ),
                         x[1])
                        for x in tables.items()] )

    return tables

def __get_stats( path_run ):
    path_datalib = os.path.join( path_run, 'stats', FILENAME_DATALIB )
    if os.path.exists( path_datalib ):
        return datalib.parse( path_datalib,
                              keycolname = 'step' )

    tables = __create_tables( path_run )

    paths = glob.glob( os.path.join(path_run, 'stats', 'stat.*') )
    paths.sort( lambda x, y: __path2step(x) - __path2step(y) )

    for path in paths:
        __add_row( tables, path )

        datalib.write( path_datalib,
                       tables )

    return tables

def __parse_file( path ):
    regex_label = r'-?[a-zA-Z]+'
    regex_number = r'-?[0-9]+(?:\.[0-9]+)?'
    regex = r'\s*(%s)\s*=\s*(%s)(\s*$|\s+[^0-9])' % (regex_label, regex_number)

    major = None

    for line in open( path ):
        line = line.strip()

        result = re.match( regex, line )
        if result:
            label = result.group(1)
            number = result.group(2)

            if '.' in number:
                type = 'float'
            else:
                type = 'int'

            if label[0] == '-':
                label = major + label
            else:
                major = label

            yield label, type, number

def __create_tables( path_run ):
    path = os.path.join( path_run, 'stats', 'stat.1' )

    tables = {}

    for label, type, value in __parse_file( path ):
        if label == 'step':
            continue

        colnames = ['step', 'value']
        coltypes = ['int', type]

        tables[label] = datalib.Table( label,
                                       colnames,
                                       coltypes,
                                       keycolname = 'step' )

    return tables

def __add_row( tables, path ):
    step = __path2step( path )

    for label, type, value in __parse_file( path ):
        if label == 'step':
            assert( int(value) == step )
            continue

        table = tables[label]
        row = table.createRow()
        row['step'] = step
        row['value'] = value

def __path2step( path ):
    return int(path[ path.rfind('.') + 1 : ])
