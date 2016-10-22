#!/bin/bash

source $( dirname $BASH_SOURCE )/__pwfarm_runutil.sh || exit 1

validate_farm_env

function usage()
{
    cat >&2 <<EOF
USAGE:

    $( basename $0 ) [dir]

        With no args, dir is your current directory. The dir must be within your local farm run
      data directory. If the dir is under a run directory (e.g. .../run_0/brain/) then information
      will be printed for that run. If the dir is not under a run, then information will be printed
      for any runs under dir.

    $( basename $0 ) run_id [nid]

        Report information for all runs with a Run ID starting with <run_id>. If <nid> is specified,
      then only runs of that nid will be reported.
EOF

    exit 1
}

if [ $# -gt 0 ] && [ "$1" == "-h" ] ; then
    usage
fi

function report_run()
{
    local dir=$1

    canonpath $dir
    echo "fid: $( cat $dir/.pwfarm/fieldnumber )"
    echo "nid: $( cat $dir/.pwfarm/nid )"
}

if [ $# == 0 ]; then
    DIR=$( canonpath $PWD )
elif [ -d $1 ]; then
    DIR=$( canonpath $1 )
else
    RUNID=$1
    validate_runid $RUNID
    if [ ! -z "$2" ]; then
	NID="$2"
	if ! is_integer "$NID"; then
	    err "nid argument must be integer"
	fi
    fi
fi

if [ ! -z "$DIR" ]; then
    ###
    ### First, check if we're under the local farm runs dir
    ###
    FARMDIR=$(pwenv runresults_dir)
    if ! echo $DIR | grep "^$FARMDIR" > /dev/null; then
	err "Not under local farm data dir: $DIR. Please specify Run ID. For help, use -h."
    fi

    ###
    ### Try to find a run directory above us in filesystem
    ###
    dir=$DIR
    found_run=false

    while true; do
	if is_run $dir; then
	    found_run=true
	    break
	fi
	
	prev=$dir
	dir=$( dirname $dir )
	if [ $prev == $dir ]; then
	    break
	fi
    done

    if $found_run; then
	report_run $dir
	###
	### DONE
	###
	exit 0
    fi

    ###
    ### Try to find run directories below us
    ###
    export -f is_run
    rundirs=$(
	find $DIR -exec bash -c "is_run {}" \; -print -prune |
	sort
    )

    if [ -z "$rundirs" ]; then
	err "No run directories found under current directory. For help, use -h"
    fi

    for dir in $rundirs; do
	report_run $dir
    done
else
    OWNER=$( pwenv pwuser )

    rundirs=$(
	find_runs_local $OWNER $RUNID |
	sort )

    if [ -z "$rundirs" ]; then
	err "No runs found of Run ID $RUNID"
    fi

    if [ -z "$NID" ]; then
	for dir in $rundirs; do
	    report_run $dir
	done
    else
	niddirs=$(
	    for dir in $rundirs; do
		if [ $NID == $(parse_stored_run_path_local --nid $dir) ]; then
		    echo $dir
		fi
	    done
	)

	if [ -z "$niddirs" ]; then
	    err "No runs found of Run ID $RUNID with nid $NID"
	fi

	for dir in $niddirs; do
	    report_run $dir
	done
    fi
fi