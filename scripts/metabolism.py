#!/usr/bin/env python

import os
import sys

import common_genome
import common_logs
import datalib


def main():
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

    computeAssortativeMating( run, os.path.join(dir_metabolism, 'assortativeMating.txt') )
    #createClusterFile( RUN, 'foo' )

def createClusterFile( path_run, path_output ):
    for agent in common_logs.LifeSpans:
        print agent

def computeAssortativeMating( path_run, path_output ):
    birthsDeaths = common_logs.BirthsDeaths( path_run )
    genomeSchema = common_genome.GenomeSchema( path_run )
    
    epochLen = 1000
    
    class Epoch:
        def __init__( self, timestep ):
            self.timestep = timestep
            self.mate_same = 0
            self.mate_diff = 0
            self.contact_same = 0
            self.contact_diff = 0
    
    epochs = {}
    
    metabolismCache = {}
    
    def __getMetabolism( agent ):
        try:
            metabolism = metabolismCache[agent]
        except:
            genome = common_genome.Genome( genomeSchema, agent )
            metabolism = genome.getGeneValue( 'MetabolismIndex' )
            metabolismCache[agent] = metabolism
    
        return metabolism
    
    for agent in birthsDeaths.entries.keys():
        entry = birthsDeaths.getEntry( agent )
        
        if (entry.parent1 != None) and (entry.deathTimestep != None):
            epochIndex = entry.birthTimestep / epochLen
            try:
                epoch = epochs[epochIndex]
            except:
                epoch = Epoch( epochIndex * epochLen )
                epochs[epochIndex] = epoch
    
            parent1Metabolism = __getMetabolism( entry.parent1 )
            parent2Metabolism = __getMetabolism( entry.parent2 )
    
            if parent1Metabolism == parent2Metabolism:
                epoch.mate_same += 1
            else:
                epoch.mate_diff += 1
    
    def __process_contact_row( row ):
        step = row['Timestep']
        agent1 = row['Agent1']
        agent2 = row['Agent2']

        epochIndex = step / epochLen
        try:
            epoch = epochs[epochIndex]
        except:
            epoch = Epoch( epochIndex * epochLen )
            epochs[epochIndex] = epoch

        agent1Metabolism = __getMetabolism( agent1 )
        agent2Metabolism = __getMetabolism( agent2 )

        if agent1Metabolism == agent2Metabolism:
            epoch.contact_same += 1
        else:
            epoch.contact_diff += 1

    datalib.parse( os.path.join(path_run, 'events/contacts.log'),
                   tablenames = ['Contacts'],
                   stream_row = __process_contact_row )

    epochIndexes = list(epochs.keys())
    epochIndexes.sort()
    
    colnames = ['Timestep', 'Ms', 'Md', 'PercentSame']
    coltypes = ['int', 'int', 'int', 'float']
    table = datalib.Table( 'AssortativeMating', colnames, coltypes )
    
    for epoch in epochs.values():
        row = table.createRow()
        row['Timestep'] = epoch.timestep
        row['Ms'] = epoch.mate_same
        row['Md'] = epoch.mate_diff
        try:
            row['PercentSame'] = 100 * float(epoch.mate_same) / (epoch.mate_same + epoch.mate_diff)
        except ZeroDivisionError:
            row['PercentSame'] = float('nan')

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
        try:
            row['Ab'] = (float(epoch.mate_same)/epoch.contact_same) / (float(epoch.mate_diff)/epoch.contact_diff);
        except ZeroDivisionError:
            row['Ab'] = float('nan')
    
    datalib.write( path_output, [table, table_contactNorm] )
    
    """
    
    for i in range(1,2):
        genome = common_genome.Genome( schema, i )
    
        name = 'Red'
        print genome.getRawValue( name )
        print genome.getGeneValue( name )
    """



main()
