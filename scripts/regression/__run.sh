#!/bin/bash

MAXSTEPS=5001

cat worldfiles/worldfile_nominal | sed "s/[0-9]\+\([[:space:]]\+\)maxSteps/$MAXSTEPS\1maxSteps/" > ./worldfile

function try {
    if ! $*; then
	echo "******************************"
	echo "*** REGRESSION TEST FAILED ***"
	echo "******************************"
	exit 1
    fi
}

try make clean
try make

try ./Polyworld

rm -rf run-regression
mv run run-regression

try scripts/CalcComplexityOverDirectory.sh ./run-regression/brain/function

try scripts/MakeRecentDirectorywithEntirePopulation.sh ./run-regression
try scripts/CalcComplexityOverRecent Recent ./run-regression
try scripts/plotNeuralComplexity Recent ./run-regression

echo "(-: REGRESSION SUCCESSFUL :-)"