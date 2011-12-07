#!/bin/bash

source $( dirname $BASH_SOURCE )/__lib.sh || exit 1

if [ "$1" == "--password" ]; then
    shift
    passwordopt="--password"
else
    passwordopt=""
fi

if [ "$1" == "--input" ]; then
    shift
    input="$1"
    shift
fi

if [ "$1" == "--output" ]; then
    shift
    output_basename="$1"
    shift
    output_dir="$1"
    shift
else
    output_basename="nil"
    output_dir="nil"
fi

script="$1"
shift
args="$*"
cmd="./$( basename $script ) $args"

if [ ! -e "$script" ]; then
    err "Cannot locate $script"
fi

tmpdir=`mktemp -d /tmp/poly_run.XXXXXXXX` || exit 1
cp $script $tmpdir || exit
if [ ! -z "$input" ]; then
    cp $input $tmpdir || exit
fi

pushd_quiet .
cd $tmpdir
zip -qr payload.zip .
popd_quiet

pwfarm_dir=`canondirname "$0"`

$pwfarm_dir/__pwfarm_dispatcher.sh $passwordopt dispatch $tmpdir/payload.zip "$cmd" "$output_basename" "$output_dir"
rm -rf $tmp_dir
