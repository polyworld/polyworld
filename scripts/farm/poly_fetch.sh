#!/bin/bash

if [ $# != 2 ]; then
    echo "usage: $( basename $0 ) run_id dst_dir"
    echo
    echo "A convenience script for pulling run files to workstation."
    exit 1
fi

runid=$1
dstdir=$2

$( dirname $0 )/poly_run.sh nil $runid nil nil nil $dstdir