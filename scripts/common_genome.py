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
