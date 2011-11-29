#!/bin/bash

function err()
{
    echo "$*">&2
    exit 1
}

function canonpath()
{
    python -c "import os.path; print os.path.realpath('$1')"
}

function canondirname()
{
    dirname `canonpath "$1"`
}

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

pushd .
cd $tmpdir
zip -r payload.zip .
popd

pwfarm_dir=`canondirname "$0"`

PWHOSTNUMBERS=~/polyworld_pwfarm/etc/pwhostnumbers
USER=`cat ~/polyworld_pwfarm/etc/pwuser`


$pwfarm_dir/pwfarm_dispatcher.sh $passwordopt dispatch $PWHOSTNUMBERS $USER $tmpdir/payload.zip "$cmd" nil nil
rm -rf $tmp_dir
