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

function build_bct()
{
    pushd .

    if [ -d bct-cpp ]; then
	cd bct-cpp
	svn update
    else
	svn checkout http://bct-cpp.googlecode.com/svn/trunk/ bct-cpp
	cd bct-cpp
    fi

    cat  Makefile | sed -e "s/CXXFLAGS[[:space:]]*=/CXXFLAGS = -arch i386 /" > Makefile.pw

    make -f Makefile.pw
    make -f Makefile.pw swig

    cp bct.py ~/polyworld_pwfarm/app/scripts
    cp _bct.so ~/polyworld_pwfarm/app/scripts

    popd
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

    build_bct

    make
else
    pwfarm_dir=`canondirname "$0"`
    poly_dir=`canonpath "$pwfarm_dir/../.."`
    scripts_dir=`canonpath "$poly_dir/scripts"`
    PWHOSTNUMBERS=~/polyworld_pwfarm/etc/pwhostnumbers
    USER=`cat ~/polyworld_pwfarm/etc/pwuser`

    tmp_dir=`mktemp -d /tmp/poly_build.XXXXXXXX` || exit 1
    echo $tmp_dir

    cd $poly_dir

    scripts/package_source.sh $tmp_dir/src.zip
    

    $pwfarm_dir/pwfarm_dispatcher.sh clear $PWHOSTNUMBERS
    $pwfarm_dir/pwfarm_dispatcher.sh dispatch $PWHOSTNUMBERS $USER $tmp_dir/src.zip './scripts/farm/poly_build.sh --field'

    rm -rf $tmp_dir
fi
