#!/usr/bin/env python

import os
import shutil
import sys

import common_logs
import wfutil

WORLDFILE_TRIAL_INPUT = './trial.wf'
WORLDFILE_TRIAL_EDITED = '/tmp/trial.wf'


argv = sys.argv[1:]
if len(argv) != 3:
    print """\
usage: trial.py input_rundir nagents steps"""
    sys.exit( 1 )

run = argv[0]
nagents = int(argv[1])
maxSteps = int(argv[2])

if not os.path.exists( run ):
    print "Cannot find input run '%s'" % run
    sys.exit( 1 )

if os.path.exists( './trials' ):
    print "Directory ./trials already exists. Please move."
    sys.exit( 1 )

if os.path.exists( './run' ):
    print "Directory ./run already exists. Please move."
    sys.exit( 1 )

if not os.path.exists( WORLDFILE_TRIAL_INPUT ):
    print "Cannot find ./trial.wf"
    sys.exit( 1 )

allAgentIds = list( common_logs.LifeSpans(run).getAll() )
allAgentIds.sort()

agentIds = allAgentIds[-nagents:];

ndomains = wfutil.get_parameter_len( WORLDFILE_TRIAL_INPUT, 'Domains', False )

edits = [
    "MinAgents=0",
    "InitAgents=1",
    "SeedAgents=1",
    "SeedMutationProbability=0",
    "SeedGenomeFromRun=True",
    "SeedPositionFromRun=True",
    "EndOnPopulationCrash=True",
    "MaxSteps=%d" % maxSteps,
]

for i in range(ndomains):
    edits.append( "Domains[%d].MinAgentsFraction=0" % i )
    edits.append( "Domains[%d].InitSeedsFraction=0" % i )
    edits.append( "Domains[%d].ProbabilityOfMutatingSeeds=0" % i )

wfutil.edit( WORLDFILE_TRIAL_INPUT, edits, WORLDFILE_TRIAL_EDITED )

f = open( "./seedPositions.txt", "w" )
f.write( "r0.5 r-1 r-0.5\n" )
f.close()

os.mkdir( "./trials" )

for agentId in agentIds:
    f = open( "./genomeSeeds.txt", "w" )
    f.write( os.path.join( run, 'genome/agents/genome_' + str(agentId) + '.txt\n' ) )
    f.close()

    if 0 != os.system( "./Polyworld %s" % WORLDFILE_TRIAL_EDITED ):
        print "Failed execting Polyworld"
        sys.exit( 1 )

    shutil.move( "./run", "./trials/run_%d" % agentId )
