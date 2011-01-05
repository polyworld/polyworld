#!/usr/bin/env python

from sys import argv, exit
from os import path

import common_functions
import wfutil

if len(argv) != 3:
           ################################################################################
    print "usage:", path.basename(argv[0]), "worldfile propname"
    exit(1)

worldfile = argv[1]
param = argv[2]

value = wfutil.get_parameter( worldfile, param, assume_run_dir = False )

if value != None:
    print value
else:
    common_functions.err( "Cannot find property '%s' in %s" % (param,worldfile) 
)
