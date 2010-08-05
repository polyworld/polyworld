#!/usr/bin/env python

import pw_agent

# Provide a valid run directory and an index for an agent that lived and died
RUN_DIR = "../build/Development/run_new_static"
AGENT_INDEX = "2"

func_filename = RUN_DIR + "/brain/function/brainFunction_" + AGENT_INDEX + ".txt"
anat_filename = RUN_DIR + "/brain/anatomy/brainAnatomy_" + AGENT_INDEX + "_death.txt"

# test just specifying a function filename
print "**************************************************************************************"
print "Specifying function filename:", func_filename
print "**************************************************************************************"
agent = pw_agent.pw_agent(func_filename)
agent.func.print_statistics()
print agent.anat.cxnmatrix

# test just specifying an anatomy filename
print "**************************************************************************************"
print "Specifying anatomy filename:", anat_filename
print "**************************************************************************************"
agent = pw_agent.pw_agent(anat_filename)
agent.func.print_statistics()
print agent.anat.cxnmatrix

# test specifying both function and anatomy filenames
print "**************************************************************************************"
print "Specifying both filenames:", func_filename
print "                          ", anat_filename
print "**************************************************************************************"
agent = pw_agent.pw_agent(func_filename, anat_filename)
agent.func.print_statistics()
print agent.anat.cxnmatrix
