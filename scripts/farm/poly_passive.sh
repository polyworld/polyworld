#!/bin/bash

function usage()
{
    echo "usage: $( basename $0 ) run_id_driven run_id_passive (\"nil\"|postrun_script) dst_dir"
    exit 1
}

function err()
{
    echo "$*">&2
    exit 1
}

self=`basename $0`

if [ "$self" == "prerun.sh" ]; then
    # we're running on the field

    run_id_driven=`cat $POLYWORLD_PWFARM_INPUT/run_id_driven`
    run_dir=~/polyworld_pwfarm/runs/good/$run_id_driven

    if [ ! -d $run_dir ]; then
	err "Cannot locate run at $run_dir"
    fi
    

    if ! cp $run_dir/BirthsDeaths.log ./LOCKSTEP-BirthsDeaths.log ; then
	exit 1
    fi

    if ! cp $run_dir/normalized.wf $POLYWORLD_PWFARM_WORLDFILE; then
	err "Failed copying driven worldfile"
    fi

    if ! ./scripts/wfutil edit $POLYWORLD_PWFARM_WORLDFILE PassiveLockstep=True; then
	err "Failed setting PassiveLockstep property"
    fi
else
    if [ -z "$1" ]; then
	usage
    fi

    if [ $# != 4 ]; then
	usage "Invalid number of parms"
    fi

    run_id_driven=$1
    run_id_passive=$2
    postrun_script=$3
    dst_dir=$4

    tmp_dir=`mktemp -d /tmp/poly_passive.XXXXXXXX` || exit 1
    echo $run_id_driven>$tmp_dir/run_id_driven

    pushd .
    cd $tmp_dir
    zip -qr input.zip .
    popd

    `dirname $0`/poly_run.sh nil $run_id_passive $0 $postrun_script $tmp_dir/input.zip $dst_dir
fi