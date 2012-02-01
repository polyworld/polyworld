#!/bin/bash

set -e

if [ $# == 0 ]; then
    name=`basename $0`
    
    echo "usage: $name src <run_dir> <failed> [<script>]"
    exit 1
fi

function err()
{
    echo $* >&2; exit 1
}

mode="$1"
if [ "$mode" != "src" ]; then
    err "Unsupported mode: $mode"
fi

run_dir="$2"
failed="3"
script="$4"

if [ ! -d "$run_dir" ]; then
    err "Invalid run dir: $run_dir"
fi

if [ -z "$failed" ]; then
    err "Missing 'failed' parm"
fi

if [ ! -z "$script" ] && [ ! -f "$script" ]; then
    err "Invalid script: $script"
fi


src_dir="$run_dir/.history/src"
mkdir -p $src_dir
nentries=$(echo `ls $src_dir | wc -l`) # use echo in order to work around stupid \t insertion
src_dir="$src_dir/$nentries"
mkdir $src_dir


if [ "$failed" == "true" ]; then
    touch "$src_dir/FAILED"
fi


SCRIPTS=`dirname $0`

$SCRIPTS/package_source.sh $src_dir/src.tbz

if [ ! -z "$script" ]; then
    cp "$script" $src_dir/script
fi

