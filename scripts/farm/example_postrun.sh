#!/bin/bash

# bomb out of script on errors
set -e

# calculate complexity
CalcComplexity.py run

if ! which gnuplot; then
# gnuplot not installed
    exit
fi

# make some plots
mkdir -p run/plots
plotopts=--noview
plot neuralComplexity -o run/plots/complexity.eps $plotopts run
plot stats -o run/plots/neurons.eps -v CurNeurons $plotopts run

# specify which files should be pulled back to workstation
echo 'plots/*.eps' >> $POLYWORLD_PWFARM_RUN_PACKAGE
