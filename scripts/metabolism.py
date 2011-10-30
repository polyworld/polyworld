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
            self.same = 0
            self.diff = 0
    
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
                epoch.same += 1
            else:
                epoch.diff += 1
    
    epochIndexes = list(epochs.keys())
    epochIndexes.sort()
    
    colnames = ['Timestep', 'Ratio']
    coltypes = ['int', 'float']
    table = datalib.Table( 'AssortativeMating', colnames, coltypes )
    
    for epoch in epochs.values():
        row = table.createRow()
        row['Timestep'] = epoch.timestep
        row['Ratio'] = float(epoch.same) / (epoch.same + epoch.diff)
    
    datalib.write( path_output, [table] )
    
    """
    
    for i in range(1,2):
        genome = common_genome.Genome( schema, i )
    
        name = 'Red'
        print genome.getRawValue( name )
        print genome.getGeneValue( name )
    """



main()
