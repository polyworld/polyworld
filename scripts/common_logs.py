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

    def getBirth( self, agentNumber ):
        return self.table[agentNumber]['BirthStep']

    def getBounds( self ):
        lo = sys.maxint
        hi = -lo

        for row in self.table.rows():
            age = row['DeathStep'] - row['BirthStep']
            lo = min(lo, age)
            hi = max(hi, age)

        return lo, hi

    def getAll( self ):
        for row in self.table.rows():
            yield row['Agent']

    def getAllWithDeathReason( self, deathReason, includeNilLifeSpan = False ):
        for row in self.table.rows():
            if row['DeathReason'] == deathReason:
                if includeNilLifeSpan or row['BirthStep'] != row['DeathStep']:
                    yield row['Agent']

    def getAllAliveAtTime( self, time ):
        for row in self.table.rows():
            if row['BirthStep'] <= time and row['DeathStep'] > time:
                yield row['Agent']

####################################################################################
###
### CLASS BirthsDeaths
###
####################################################################################
class BirthsDeaths:
    def __init__( self, path_run ):
        self.entries = {}

        f = open( os.path.join(path_run, 'BirthsDeaths.log') )

        header = f.readline()
        assert( header.strip() == '% Timestep Event Agent# Parent1 Parent2' )

        for line in f:
            fields = line.split()

            timestep = int( fields[0] )
            event = fields[1]
            agent = int( fields[2] )

            if event == 'BIRTH':
                parent1 = int( fields[3] )
                parent2 = int( fields[4] )

                entry = BirthsDeathsEntry( event, timestep, parent1, parent2 )
                self.entries[agent] = entry
            elif event == 'CREATION':
                entry = BirthsDeathsEntry( event, timestep, None, None )
                self.entries[agent] = entry
                
            elif event == 'DEATH':
                try:
                    entry = self.entries[agent]
                except:
                    entry = BirthsDeathsEntry( 'UNKNOWN', -1, None, None )

                entry.death( timestep )
                
            else:
                print event
                assert( False )


    def getEntry( self, agentNumber ):
        return self.entries[ agentNumber ]

class BirthsDeathsEntry:
    def __init__( self, birthType, timestep, parent1, parent2 ):
        self.birthType = birthType
        self.birthTimestep = timestep
        self.parent1 = parent1
        self.parent2 = parent2
        self.deathTimestep = None

    def death( self, timestep ):
        self.deathTimestep = timestep
