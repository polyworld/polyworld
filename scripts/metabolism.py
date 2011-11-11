#!/usr/bin/env python

import os
import sys

import common_genome
import common_logs
import datalib


def main():
    epochLen = 10000

    argv = sys.argv[1:]

    if len(argv) != 1:
        print "usage: metabolism.py rundir"
        sys.exit( 1 )

    run = argv[0]
    if not os.path.exists( run ):
        print "Cannot locate dir '%s'" % run
        sys.exit( 1 )

    dir_metabolism = os.path.join( run, 'metabolism' )
    if not os.path.exists( dir_metabolism ):
        os.mkdir( dir_metabolism )

    metabolismCache = createMetabolismCache( run )

    computeEvents( metabolismCache, epochLen, run, os.path.join(dir_metabolism, 'events.txt') )

def createMetabolismCache( path_run ):
    metabolismCache = {}

    genomeSchema = common_genome.GenomeSchema( path_run )
    agents = common_logs.LifeSpans( path_run ).getAll()

    for agent in agents:
        genome = common_genome.Genome( genomeSchema, agent )
        metabolism = genome.getGeneValue( 'MetabolismIndex' )
        metabolismCache[agent] = metabolism

    return metabolismCache

def fdiv( x, y ):
    try:
        return float(x) / y
    except:
        return float('nan')

def computeEvents( metabolismCache, epochLen, path_run, path_output ):
    birthsDeaths = common_logs.BirthsDeaths( path_run )

    class Epoch:
        def __init__( self, timestep ):
            self.timestep = timestep
            self.mate_same = 0
            self.mate_diff = 0
            self.give_same = 0
            self.give_diff = 0
            self.contact_same = 0
            self.contact_diff = 0
    
    epochs = {}
    
    for agent in birthsDeaths.entries.keys():
        entry = birthsDeaths.getEntry( agent )
        
        if (entry.parent1 != None) and (entry.deathTimestep != None):
            epochIndex = entry.birthTimestep / epochLen
            try:
                epoch = epochs[epochIndex]
            except:
                epoch = Epoch( epochIndex * epochLen )
                epochs[epochIndex] = epoch
    
            parent1Metabolism = metabolismCache[ entry.parent1 ]
            parent2Metabolism = metabolismCache[ entry.parent2 ]
    
            if parent1Metabolism == parent2Metabolism:
                epoch.mate_same += 1
            else:
                epoch.mate_diff += 1
    
    def __process_contact_row( row ):
        step = row['Timestep']
        agent1 = row['Agent1']
        agent2 = row['Agent2']
        events = row['Events']

        epochIndex = step / epochLen
        try:
            epoch = epochs[epochIndex]
        except:
            epoch = Epoch( epochIndex * epochLen )
            epochs[epochIndex] = epoch

        agent1Metabolism = metabolismCache[ agent1 ]
        agent2Metabolism = metabolismCache[ agent2 ]

        ngive = events.count( 'G' )

        if agent1Metabolism == agent2Metabolism:
            epoch.contact_same += 1
            epoch.give_same += ngive
        else:
            epoch.contact_diff += 1
            epoch.give_diff += ngive

    datalib.parse( os.path.join(path_run, 'events/contacts.log'),
                   tablenames = ['Contacts'],
                   stream_row = __process_contact_row )

    # HACK: DROP LAST EPOCH SINCE IT'S USUALLY JUST ONE STEP
    epochIndexes = list(epochs.keys())
    epochIndexes.sort()
    del epochs[ epochIndexes[-1] ]
    
    colnames = ['Timestep', 'Ms', 'Md', 'PercentSame']
    coltypes = ['int', 'int', 'int', 'float']
    table = datalib.Table( 'AssortativeMating', colnames, coltypes )
    
    for epoch in epochs.values():
        row = table.createRow()
        row['Timestep'] = epoch.timestep
        row['Ms'] = epoch.mate_same
        row['Md'] = epoch.mate_diff
        row['PercentSame'] = 100 * fdiv(epoch.mate_same, epoch.mate_same + epoch.mate_diff)

    colnames = ['Timestep', 'Ms', 'Md', 'Cs', 'Cd', 'Ab']
    coltypes = ['int', 'int', 'int', 'int', 'int', 'float']
    table_contactNorm = datalib.Table( 'AssortativeMating_ContactNormalized', colnames, coltypes )
    
    for epoch in epochs.values():
        row = table_contactNorm.createRow()
        row['Timestep'] = epoch.timestep
        row['Ms'] = epoch.mate_same
        row['Md'] = epoch.mate_diff
        row['Cs'] = epoch.contact_same
        row['Cd'] = epoch.contact_diff
        row['Ab'] = fdiv( fdiv(epoch.mate_same, epoch.contact_same),
                          fdiv(epoch.mate_diff, epoch.contact_diff) );

    colnames = ['Timestep', 'Gs', 'Gd', 'Cs', 'Cd', 'Ps', 'Pd', 'Bias']
    coltypes = ['int', 'int', 'int', 'int', 'int', 'float', 'float', 'float']
    table_give = datalib.Table( 'Give', colnames, coltypes )
    
    for epoch in epochs.values():
        row = table_give.createRow()
        row['Timestep'] = epoch.timestep
        row['Gs'] = epoch.give_same
        row['Gd'] = epoch.give_diff
        row['Cs'] = epoch.contact_same
        row['Cd'] = epoch.contact_diff
        row['Ps'] = fdiv( epoch.give_same, epoch.contact_same * 2 )
        row['Pd'] = fdiv( epoch.give_diff, epoch.contact_diff * 2 )
        row['Bias'] = fdiv( fdiv(epoch.give_same, epoch.contact_same),
                            fdiv(epoch.give_diff, epoch.contact_diff) );

    colnames = ['Timestep', 'Cs', 'Cd', 'PercentSame']
    coltypes = ['int', 'int', 'int', 'float']
    table_contact = datalib.Table( 'Contact', colnames, coltypes )
    
    for epoch in epochs.values():
        row = table_contact.createRow()
        row['Timestep'] = epoch.timestep
        row['Cs'] = epoch.contact_same
        row['Cd'] = epoch.contact_diff
        row['PercentSame'] = 100 * fdiv( epoch.contact_same, epoch.contact_same + epoch.contact_diff )
    
    datalib.write( path_output, [table, table_contactNorm, table_give, table_contact] )


main()
