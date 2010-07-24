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

if [ "$1" == "--field" ]; then
    if [ -e ~/polyworld_pwfarm/app ]; then
	mkdir -p /tmp/polyworld_pwfarm
	bak=/tmp/polyworld_pwfarm/`date | sed -e "s/ /_/g" -e "s/:/./g"`
	echo "~/polyworld_pwfarm/app already exists! Moving to $bak"
	
	mv ~/polyworld_pwfarm/app $bak
    fi

    mkdir -p ~/polyworld_pwfarm/app
    mv * ~/polyworld_pwfarm/app
    cd ~/polyworld_pwfarm/app

    make
else
    pwfarm_dir=`canondirname "$0"`
    poly_dir=`canonpath "$pwfarm_dir/../.."`
    PWHOSTNUMBERS=~/polyworld_pwfarm/etc/pwhostnumbers
    USER=`cat ~/polyworld_pwfarm/etc/pwuser`

    tmp_dir=`mktemp -d /tmp/poly_build.XXXXXXXX` || exit 1
    echo $tmp_dir

    cd $poly_dir

    make || exit 1

    zip -r $tmp_dir/src.zip Makefile src scripts objects

    $pwfarm_dir/pwfarm_dispatcher.sh clear $PWHOSTNUMBERS
    $pwfarm_dir/pwfarm_dispatcher.sh dispatch $PWHOSTNUMBERS $USER $tmp_dir/src.zip './scripts/farm/poly_build.sh --field'

    rm -rf $tmp_dir
fi
