import os
import re
import sys

import datalib
import glob

STAT_TYPES = ['agents', 'food', 'born', 'bornv', 'created', 'died', 'EatRate', 'MateRate', 'LifeSpan', 'CurNeurons', 'NeurGroups', 'CurNeurGroups', 'Domain[domain_index]FP[foodpatch_index][stat_index]']
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
### FUNCTION path_stats()
###
####################################################################################
def path_stats(path_run):
    return os.path.join(path_run, 'stats', FILENAME_DATALIB)

####################################################################################
###
### FUNCTION parse_stats
###
####################################################################################
def parse_stats(run_path, types = None, quiet = False):
    __ensure_datalib( run_path, quiet )

    return datalib.parse( path_stats(run_path),
                          tablenames = types,
                          required = datalib.REQUIRED,
                          keycolname = 'step' )

####################################################################################
###
### FUNCTION __ensure_datalib
###
####################################################################################
def __ensure_datalib( path_run, quiet = False ):
    path_datalib = path_stats( path_run )
    if os.path.exists( path_datalib ):
        return

    if not quiet:
        # This can take a long time, so let the user we're not hung
        print 'Converting stats files into datalib format for run', path_run

    tables = __create_tables( path_run, quiet )

    paths = glob.glob( os.path.join(path_run, 'stats', 'stat.*') )
    paths.sort( lambda x, y: __path2step(x) - __path2step(y) )

    for path in paths:
        __add_row( tables, path, quiet )

    if not quiet:
        print '\nwriting %s' % path_datalib

    datalib.write( path_datalib,
                   tables,
                   randomAccess = False )

####################################################################################
###
### FUNCTION __parse_file
###
####################################################################################
def __parse_file( path, quiet ):
    if not quiet:
        print '\rparsing %s' % path,

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


####################################################################################
###
### FUNCTION __create_tables
###
####################################################################################
def __create_tables( path_run, quiet ):
    path = os.path.join( path_run, 'stats', 'stat.1' )

    tables = {}

    for label, type, value in __parse_file( path, quiet ):
        if label == 'step':
            continue

        colnames = ['step', 'value']
        coltypes = ['int', type]

        tables[label] = datalib.Table( label,
                                       colnames,
                                       coltypes,
                                       keycolname = 'step' )

    return tables

####################################################################################
###
### FUNCTION __add_row
###
####################################################################################
def __add_row( tables, path, quiet ):
    step = __path2step( path )

    for label, type, value in __parse_file( path, quiet ):
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

####################################################################################
###
### FUNCTION __path2step
###
####################################################################################
def __path2step( path ):
    return int(path[ path.rfind('.') + 1 : ])
