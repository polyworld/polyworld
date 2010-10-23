import os
import re
import sys

import datalib
import glob

STAT_TYPES = ['agents', 'food', 'born', 'created', 'died', 'LifeSpan', 'CurNeurons', 'NeurGroups', 'CurNeurGroups', 'Domain[domain_index]FP[foodpatch_index][stat_index]']
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
### FUNCTION path_run_from_stats()
###
####################################################################################
def path_run_from_stats(path_stats, classification, dataset):
    suffix = relpath_stats()

    return path_stats[:-(len(suffix) + 1)]

####################################################################################
###
### FUNCTION parse_complexity
###
####################################################################################
def parse_stats(run_paths, classification = None, dataset = None, types = None, run_as_key = False, quiet = False):
    # make sure the datalib files exist
    for path in run_paths:
        __get_stats( path, quiet )

    # parse the stats for all the runs
    tables = datalib.parse_all( map(lambda x: path_stats( x ),
                                    run_paths),
                                types,
                                datalib.REQUIRED,
                                keycolname = 'step' )

    if run_as_key:
        # modify the map to use run dir as key, not Avr file
        tables = dict( [(path_run_from_stats( x[0],
                                              classification,
                                              dataset),
                         x[1])
                        for x in tables.items()] )

    return tables

def __get_stats( path_run, quiet = False ):
    path_datalib = os.path.join( path_run, 'stats', FILENAME_DATALIB )
    if os.path.exists( path_datalib ):
        return datalib.parse( path_datalib,
                              keycolname = 'step' )

    if not quiet:
        # This can take a long time, so let the user we're not hung
        print 'Converting stats files into datalib format for run', path_run

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
    regex_equals = r'\s*(%s)\s*=\s*(%s)(\s*$|\s+[^0-9])' % (regex_label, regex_number)

    major = None

    def __type(number):
        if '.' in number:
            return 'float'
        else:
            return 'int'

    for line in open( path ):
        line = line.strip()

        result = re.match( regex_equals, line )
        if result:
            label = result.group(1)
            number = result.group(2)
            type = __type(number)

            if label[0] == '-':
                label = major + label
            else:
                major = label

            yield label, type, number
        else:
            fields = line.split()

            if fields[0] == 'Domain':
                domain_id = fields[1]
            else:
                result = re.match( r'FP([0-9]+|\*)', fields[0] )
                if result:
                    fp_id = result.group(1)

                    for i in range(1, len(fields)):
                        label = 'Domain[%s]FP[%s][%s]' % (domain_id, fp_id, i-1)
                        number = fields[i]
                        type = __type( number )

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

        if not label in tables:
            print 'Warning! Found label "%s" in %s, but not in stat.1' % (label,path)
        else:
            table = tables[label]
            row = table.createRow()
            row['step'] = step
            row['value'] = value

def __path2step( path ):
    return int(path[ path.rfind('.') + 1 : ])

def test():
    #for x in __parse_file( '../run_tau60k_from18k_ws200/stats/stat.1' ):
    #    print x

    for tablename, table in datalib.parse( '../run_tau60k_from18k_ws200/stats/datalib.txt' ).items():
        print '---', tablename, table.name, '---'
        for row in table.rows():
            print row['value']

