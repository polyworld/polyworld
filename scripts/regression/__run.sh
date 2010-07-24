#!/bin/bash

if [ -z "$1" ]; then
    TESTS="clean determinism complexity"
else
    TESTS="$*"
fi

function istest {
    echo $TESTS | grep "\<$1\>" > /dev/null
}

function worldfile {
    if [[ "$1" == *=* ]]; then
	in=./worldfiles/worldfile_nominal
    else
	in="$1"
	shift
    fi

    try ./scripts/edit_worldfile.py $in ./worldfile $*
}

function try {
    if ! $*; then
	fail "Failing command = $*"
    fi
}

function fail {
    echo "******************************"
    echo "*** REGRESSION TEST FAILED ***"
    echo "******************************"

    echo
    echo $1

    exit 1
}

function run {
    run_output=$1

    try ./Polyworld > out

    mkdir -p `dirname $run_output`
    mv run $run_output
    mv out $run_output
}

#
# BUILD
#
if istest clean; then
    try make clean
fi

try make

#
# DETERMINISM
#
if istest determinism; then
    NSTEPS=101

    function determinism {
	case "$1" in
	    "static=true")
		static=1
		type="static"
		;;
	    "static=false")
		static=0
		type="dynamic"
		;;
	    *)
		echo "BAD REGRESSION SCRIPT!"
		exit 1
		;;
	esac

	worldfile recordPerformanceStats=0 maxSteps=$NSTEPS staticTimestepGeometry=$static

	dir=regression/determinism/$type

	run $dir/run-a
	run $dir/run-b

	if ! diff -r $dir/run-{a,b} > $dir/diff.out; then
	    fail "Determinism failed with type $type"
	fi
    }

    echo "--- Testing Dynamic Geometry Determinism"
    determinism static=false

    echo "--- Testing Static Geometry Determinism"
    determinism static=true
fi

#
# COMPLEXITY
#
if istest complexity; then
    NSTEPS=301
    FREQUENCY=100

    echo "--- Testing Complexity"

    dir=regression/complexity/run
    worldfile maxSteps=$NSTEPS bestRecentBrainAnatonomyRecordFrequency=$FREQUENCY bestRecentBrainFunctionRecordFrequency=$FREQUENCY
    run $dir

    try scripts/CalcComplexityOverDirectory.sh $dir/brain/function

    try scripts/MakeRecentDirectorywithEntirePopulation.sh $dir
    try scripts/CalcComplexityOverRecent Recent $dir
    try scripts/plotNeuralComplexity Recent $dir
fi

echo "(-: REGRESSION SUCCESSFUL :-)"
exit 0