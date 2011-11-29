#!/bin/bash

# bomb out of script on errors
set -e

# calculate complexity
CalcComplexity.py run

# make some plots
mkdir -p run/plots
plotopts=--noview
plot neuralComplexity -o run/plots/complexity.eps $plotopts run
plot stats -o run/plots/neurons.eps -v CurNeurons $plotopts run

# specify which files should be pulled back to workstation
echo 'plots/*.eps' >> $PWFARM_RUNZIP
