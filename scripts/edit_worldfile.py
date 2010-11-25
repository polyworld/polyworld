#!/usr/bin/env python

###
### This module can be used as a script from the shell or as a library.
### The entry point for the shell is main(), the entry point as a library
### is edit()
###

from sys import argv, exit
from os import path
import re

from common_functions import err

################################################################################
###
### CLASS LabelNotFoundException
###
################################################################################
class LabelNotFoundException(Exception):
    pass

################################################################################
###
### FUNCTION edit()
###
### orig = path of original worldfile
### new = path of new worldfile to create
### edit_specs = array of edits in form "label=value", where label supports splat wildcard
###
################################################################################
def edit( orig, new, edit_specs ):
    edits = {}
    
    for edit in edit_specs:
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
        raise LabelNotFoundException( "Couldn't find labels: " +  ', '.join(edits.keys()) )

################################################################################
###
### FUNCTION main()
###
### entry point when called from shell    
###
################################################################################
def main():
    if len(argv) < 3:
        print "usage:", path.basename(argv[0]), "input_file output_file (propname=value)+"
        print
        print "      Propname may include splat wildcard -- useful for arrays. If using splat,"
        print "  be sure to put quotes around the argument so the shell doesn't screw it up."
        exit(1)

    try:
        edit( argv[1], argv[2], argv[3:] )
    except Exception, x:
        err( x )
    
    exit( 0 )

if __name__ == "__main__":
    main()
