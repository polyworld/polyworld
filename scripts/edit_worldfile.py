#!/usr/bin/python

from sys import argv, exit
from os import path

if len(argv) == 1:
    print "usage:", path.basename(argv[0]), "input_file output_file (propname=value)*"
    exit(1)

orig = argv[1]
new = argv[2]

edits = {}

for edit in argv[3:]:
    edit = edit.strip().split('=')
    edits[edit[0]] = edit[1]

forig = open( orig )
fnew = open( new, 'w' )

for line in forig:
    fields = line.split()
    newline = line

    if( len(fields) >= 2 ):
        try:
            label = fields[1]
            newval = edits[label]
            newline = newval + '\t' + label + '\n'
            del edits[label]
        except KeyError:
            # no edit for this label
            pass
    
    fnew.write( newline )

forig.close()
fnew.close()

if len( edits ):
    print "Couldn't find labels:", ', '.join( edits.keys() )
    exit( 1 )

exit( 0 )
