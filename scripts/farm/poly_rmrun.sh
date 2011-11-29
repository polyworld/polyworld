#!/bin/bash

if [ $# == 0 ]; then
    echo "usage: $( basename $0 ) [--test] [--orphan] runid|/"
    echo
    echo "Deletes run data from farm. Either specify run id or '/' for all."
    echo
    echo "  --test: Don't delete anything, just check show info."
    echo "  --oprhan: Operate on runs in ophan directory."
    echo
    echo "  WARNING! Option parsing is order-dependent. Follow order in usage."

    exit 1
fi

if [ "$1" != "--field" ]; then
    for x in $*; do
	if [ "$x" == "/" ]; then
	    read -p "You are about to delete all runs! Are you sure? [y/n]: " response
	    if [ "$response" != "y" ]; then
		exit 1
	    fi
	fi
    done

    poly_script.sh $0 --field $*
else
    shift

    if [ "$1" == "--test" ]; then
	shift
	testing=true
    else
	testing=false
    fi

    if [ "$1" == "--orphan" ]; then
	shift
	orphan=true
    else
	orhpan=false
    fi

    runid=$1
    if [ "$runid" == "/" ]; then
	runid=""
    fi

    if [ "$orphan" == "true" ]; then
	rundir=~/polyworld_pwfarm/runs/orphaned
	if [ "$testing" == "true" ]; then
	    file $rundir
	    ls $rundir
	    read -p "press enter..."
	else
	    rm -rf $rundir
	fi
    else
	good=~/polyworld_pwfarm/runs/good/$runid
	failed=~/polyworld_pwfarm/runs/failed/$runid

	if [ "$testing" == "true" ]; then
	    file $good
	    ls $good
	    file $failed
	    ls $failed
	    read -p "press enter..."
	else
	    rm -rf $good
	    rm -rf $failed
	fi
    fi
fi