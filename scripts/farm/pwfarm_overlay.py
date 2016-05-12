#!/usr/bin/env python

import numpy
import sys

def usage():
    print """\
usage: pwfarm_overlay.py N (propname minval maxval)+

  Generates an overlay file, where results are written to stdout.

  Example:

    pwfarm_overlay.py 10 MinMutationRate 0.001 0.01 MaxMutationRate 0.005 0.05 > mutate.wfo"""
    sys.exit(1)

class PropRange:
    def __init__( self, name, minval, maxval ):
        self.name = name
        self.minval = float(minval)
        self.maxval = float(maxval)

    def getValue( self, normalizedValue ):
        return (normalizedValue * (self.maxval - self.minval)) + self.minval

args = sys.argv[1:]

if len(args) == 0:
    usage()

N = int( args[0] )

args = args[1:]
assert( len(args) % 3 == 0 )

props=[]

for iprop in range(0, len(args), 3):
    props.append( PropRange(args[iprop], args[iprop+1], args[iprop+2]) )

print "overlays ["

for normalizedValue in numpy.linspace( 0, 1, N ):
    for prop in props:
        print "  %-20s %f" % ( prop.name, prop.getValue(normalizedValue) )

    if normalizedValue != 1.0:
        print ","
    
print "]"
