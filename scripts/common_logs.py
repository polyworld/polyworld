import os
import sys

import datalib

####################################################################################
###
### CLASS LifeSpans
###
####################################################################################
class LifeSpans:
    def __init__( self, path_run ):
        self.table = datalib.parse( os.path.join(path_run, 'lifespans.txt'),
                                    keycolname = 'Agent' )['LifeSpans']

    def age( self, agentNumber, step ):
        return step - self.table[agentNumber]['BirthStep']

    def getBounds( self ):
        lo = sys.maxint
        hi = -lo

        for row in self.table.rows():
            age = row['DeathStep'] - row['BirthStep']
            lo = min(lo, age)
            hi = max(hi, age)

        return lo, hi

    def getAllWithDeathReason( self, deathReason, includeNilLifeSpan = False ):
        for row in self.table.rows():
            if row['DeathReason'] == deathReason:
                if includeNilLifeSpan or row['BirthStep'] != row['DeathStep']:
                    yield row['Agent']

    def getAllAliveAtTime( self, time ):
        for row in self.table.rows():
            if row['BirthStep'] <= time and row['DeathStep'] > time:
                yield row['Agent']
