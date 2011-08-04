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
        path_log = os.path.join(path_run, 'genome/separations.txt')

        class state:
            tables = {}

        def __beginTable( tablename, colnames, coltypes, path, table_index, keycolname ):
            agentNumber = int( tablename )
            state.currTable = {}
            state.tables[ agentNumber ] = state.currTable

        def __row( row ):
            state.currTable[ row['Agent'] ] = row[ 'Separation' ]


        datalib.parse( path_log, stream_beginTable = __beginTable, stream_row = __row )

        self.tables = state.tables

    def separation( self, agentNumber1, agentNumber2 ):
        if agentNumber1 < agentNumber2:
            x = agentNumber1
            y = agentNumber2
        else:
            x = agentNumber2
            y = agentNumber1

        return self.tables[x][y]

    # returns min_separation, max_separation
    def getBounds( self ):
        lo = sys.maxint
        hi = -lo

        for table in self.tables.values():
            for separation in table.values():
                lo = min(lo, separation)
                hi = max(hi, separation)

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
        genomepath = f.readline()
        if '/genome/agents/' in genomepath:
            path_curr = os.path.dirname( os.path.dirname(os.path.dirname( genomepath )) )
        else:
            path_curr = os.path.dirname( os.path.dirname(genomepath) )
        
        f.close()

    return paths
        
