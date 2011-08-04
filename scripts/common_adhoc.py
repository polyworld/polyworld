import os
import sys
import traceback

import datalib

####################################################################################
###
### FUNCTION parseArgs
###
####################################################################################
def parseArgs( mode, options, args ):
    if len(args) < 5:
        raise Exception( "Missing arguments" )


    try:
        relpath = args.pop(0)
        tablename = args.pop(0)
        xcolname = args.pop(0)
        ycolname = args.pop(0)
    
        mode.func_relpath = lambda x, y: relpath
        mode.func_pathRunFromValue = lambda path, classification, dataset: path[:-len(relpath)].rstrip('/')
        mode.defaultValues = tablename
        mode.colName_timestep = xcolname
        mode.curvetypes = ycolname.split(',')
        mode.defaultCurvetype = mode.curvetypes[0]

        copts = options['curvetypes']
        for curvetype in mode.curvetypes:
            copts.settings[curvetype] = True
        del copts.settings['adhoc_curvetype']
    
        metaopts = options['meta']
        metaopts.settings[ycolname] = metaopts.settings['adhoc_curvetype']
        del metaopts.settings['adhoc_curvetype']
    
        mode.func_parseValues = lambda a, b, c, d, e: parseValues( mode, a, b, c, d, e )
    except:
        print "Unexpected error:", sys.exc_info()
        traceback.print_tb(sys.exc_info()[2])
        raise
    return args

####################################################################################
###
### FUNCTION parseValues
###
####################################################################################
def parseValues(mode, run_paths, classification, dataset, values, run_as_key):
    relpath = mode.relpath(classification, dataset)
    tablename = mode.defaultValues
    keycolname = mode.colName_timestep

    # parse the AVRs for all the runs
    tables = datalib.parse_all( map(lambda x: os.path.join(x, relpath),
                                    run_paths),
                                [tablename],
                                datalib.REQUIRED,
                                keycolname = keycolname )

    if run_as_key:
        # modify the map to use run dir as key, not Avr file
        avrs = dict( [(mode.pathRunFromValue( x[0], classification, dataset ),
                       x[1])
                      for x in tables.items()] )

    return avrs

####################################################################################
###
### FUNCTION getValueNames
###
####################################################################################
def getValueNames( names ):
    return names
