import os
import sys

import datalib

####################################################################################
###
### CLASS SeparationCache
###
####################################################################################
class SeparationCache:
    def __init__( self, path_run ):
        self.tables = datalib.parse( os.path.join(path_run, 'genome/separations.txt'),
                                     keycolname = 'Agent',
                                     tablename2key = int )

    def separation( self, agentNumber1, agentNumber2 ):
        if agentNumber1 < agentNumber2:
            x = agentNumber1
            y = agentNumber2
        else:
            x = agentNumber2
            y = agentNumber1

        return self.tables[x][y]['Separation']

    # returns min_separation, max_separation
    def getBounds( self ):
        lo = sys.maxint
        hi = -lo

        for table in self.tables.values():
            for row in table.rows():
                sep = row['Separation']
                lo = min(lo, sep)
                hi = max(hi, sep)

        return lo, hi

####################################################################################
###
### FUNCTION get_seed_run_chain()
###
### Using information from genomeSeeds.txt in a run directory, construct a list
### of runs from oldest to newest.
###
####################################################################################
def get_seed_run_chain( path_newest ):
    path_curr = path_newest
    paths = []

    while True:
        paths.insert( 0, path_curr )

        path_seeds = os.path.join( path_curr, 'genome/genomeSeeds.txt' )
        if not os.path.exists( path_seeds ):
            break;

        f = open( path_seeds )
        line = f.readline()
        path_curr = os.path.dirname( os.path.dirname(line) )
        f.close()

    return paths
        
