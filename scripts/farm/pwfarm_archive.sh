#!/bin/bash

echo "incompatible with current pwfarm implementation"
exit 1

if [ -z "$1" ]; then
    echo "usage: $0 run_id"
    echo
    echo "  run_id: unique id of run, which should use '/' for a hierarchy of classification."
    echo

    exit 1
fi

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

if [ "$1" == "--field" ]; then
    payload_dir=`pwd`
    RUNID=`cat $payload_dir/runid`

    cd ~/polyworld_pwfarm/runs/good || exit
    
    if [ ! -z $RUNID ]; then
	cd $RUNID || exit 1
    fi

    rundirs=`find . -name "worldfile" -exec dirname {} \;`
    
    for rundir in $rundirs ; do
	rundir=`canonpath $rundir`
	echo $rundir
	pushd .
	cd $rundir
	tarname=../`basename $rundir`.run.tar.gz
	tar czf $tarname . || exit 1
	popd
	rm -rf $rundir
    done
else
    pwfarm_dir=`canondirname "$0"`
    poly_dir=`canonpath "$pwfarm_dir/../.."`

    PWHOSTNUMBERS=~/polyworld_pwfarm/etc/pwhostnumbers
    USER=`cat ~/polyworld_pwfarm/etc/pwuser`
    RUNID="$1"

    tmp_dir=`mktemp -d /tmp/poly_tar.XXXXXXXX` || exit 1
    echo $tmp_dir

    cp $0 $tmp_dir/pwfarm_tar.sh || exit
    echo "$RUNID" > $tmp_dir/runid

    cd $tmp_dir

    zip -r payload.zip .

    $pwfarm_dir/__pwfarm_dispatcher.sh dispatch $PWHOSTNUMBERS $USER payload.zip './pwfarm_tar.sh --field'

    rm -rf $tmp_dir
fi