#!/usr/bin/env python

import argparse
import os
import re
import pw_agent
import sys

def parseArgs():
    parser = argparse.ArgumentParser(description = "Converts brain function files to time series.")
    parser.add_argument("run", metavar = "RUN", help = "run directory")
    group = parser.add_mutually_exclusive_group()
    group.add_argument("--steps", metavar = "STEPS", type = int, help = "number of timesteps")
    group.add_argument("--max-steps", metavar = "MAX_STEPS", type = int, help = "maximum number of timesteps")
    return parser.parse_args()

def writeStep(agent, step):
    for neuron in range(agent.func.num_neurons - 1):
        if neuron > 0:
            sys.stdout.write(" ")
        sys.stdout.write("{0}".format(agent.func.acts[neuron, step]))
    sys.stdout.write("\n")

args = parseArgs()
sys.stdout.write("# BEGIN ARGUMENTS\n")
if args.steps is not None:
    sys.stdout.write("steps = {0}\n".format(args.steps))
elif args.max_steps is not None:
    sys.stdout.write("max-steps = {0}\n".format(args.max_steps))
sys.stdout.write("# END ARGUMENTS\n")
funcPath = os.path.join(args.run, "brain", "function")
anatPath = os.path.join(args.run, "brain", "anatomy")
for funcFileName in os.listdir(funcPath):
    match = re.search(r"^(?:incomplete_)?brainFunction_(\d+)(\.txt(?:\.gz)?)$", funcFileName)
    if match is None:
        continue
    anatFileName = "brainAnatomy_{0}_incept{1}".format(match.group(1), match.group(2))
    try:
        agent = pw_agent.pw_agent(os.path.join(funcPath, funcFileName), os.path.join(anatPath, anatFileName))
    except:
        continue
    if agent.func.timesteps_lived == 0:
        continue
    sys.stdout.write("# AGENT {0}\n".format(agent.func.agent_index))
    sys.stdout.write("# DIMENSIONS {0} {1} {2}\n".format(agent.func.num_neurons - 1, agent.func.num_inputneurons, len(agent.func.neurons["behavior"])))
    sys.stdout.write("# BEGIN ENSEMBLE\n")
    if args.max_steps is None:
        if args.steps is None:
            stepCountMax = agent.func.timesteps_lived
        else:
            stepCountMax = args.steps
        stepCount = 0
        while stepCount < stepCountMax:
            sys.stdout.write("# BEGIN TIME SERIES\n")
            for step in range(agent.func.timesteps_lived):
                writeStep(agent, step)
                stepCount += 1
                if stepCount == stepCountMax:
                    break
            sys.stdout.write("# END TIME SERIES\n")
    else:
        if agent.func.timesteps_lived <= args.max_steps:
            steps = range(agent.func.timesteps_lived)
        else:
            steps = range(agent.func.timesteps_lived - args.max_steps, agent.func.timesteps_lived)
        sys.stdout.write("# BEGIN TIME SERIES\n")
        for step in steps:
            writeStep(agent, step)
        sys.stdout.write("# END TIME SERIES\n")
    sys.stdout.write("# END ENSEMBLE\n")
