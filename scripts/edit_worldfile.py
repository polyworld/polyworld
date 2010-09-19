#!/usr/bin/env python

from sys import argv, exit
from os import path
import re

if len(argv) == 1:
           ################################################################################
    print "usage:", path.basename(argv[0]), "input_file output_file (propname=value)*"
    print
    print "      Propname may include splat wildcard -- useful for arrays. If using splat,"
    print "  be sure to put quotes around the argument so the shell doesn't screw it up."
    exit(1)

orig = argv[1]
new = argv[2]

edits = {}

for edit in argv[3:]:
    pattern, value = edit.strip().split('=')
    pattern = '^' + pattern.replace('*', '.*').replace('[', '\[').replace(']', '\]') + '$'
    edits[pattern] = value

edits_notFound = list( edits.keys() )

forig = open( orig )
fnew = open( new, 'w' )

for line in forig:
    fields = line.split()
    newline = line

    if( len(fields) >= 2 ):
        label = fields[1]

        for pattern, value in edits.items():
            if re.match( pattern, label ):
                newline = value + '\t' + label + '\n'
                try:
                    edits_notFound.remove( pattern )
                except:
                    pass # edits can have multiple matches with patterns
    
    fnew.write( newline )

forig.close()
fnew.close()

if len( edits_notFound ):
    print "Couldn't find labels:", ', '.join( edits.keys() )
    exit( 1 )

exit( 0 )
