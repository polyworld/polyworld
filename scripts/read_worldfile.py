#!/usr/bin/env python

from sys import argv, exit
from os import path

import common_functions

if len(argv) != 3:
           ################################################################################
    print "usage:", path.basename(argv[0]), "worldfile propname"
    exit(1)

worldfile = argv[1]
param = argv[2]

value = common_functions.read_worldfile_parameter( worldfile, param )

if value != None:
    print value
else:
    common_functions.err( "Cannot find property '%s' in %s" % (param,worldfile) 
)
