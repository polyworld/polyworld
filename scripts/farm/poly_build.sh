#!/bin/bash

set -e

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

    # Note: If you change the following contents of Makefile.vars, be sure to escape $ (\$)
    echo "\
# Arguments to be sent to the C++ compiler
# Some arguments may already be specified in Makefile
CXXFLAGS                += -m32 -fopenmp

# Installation directory
install_dir              = /usr/local

# The following variables are only needed for SWIG
# If you aren't generating Python bindings, you don't need to worry about them

# A typical value for the Python header file directory is
python_dir_apple         = /Library/Frameworks/Python.framework/Versions/Current/include/python2.6

# Python header file directory
# This points to the C header files required to compile the SWIG bindings
# You may be able to use one of the previously defined variables
# E.g., python_dir = \$(python_dir_apple)
python_dir               = \$(python_dir_apple)

# Typical values for swig_lib_flags in different environments
# You probably don't need to change these
swig_lib_flags_apple     = -bundle -flat_namespace -undefined suppress
swig_lib_flags_linux     = -shared

# Arguments for generating a shared library from SWIG wrappers
# You can probably use one of the previously defined variables
# E.g., swig_lib_flags = \$(swig_lib_flags_apple)
swig_lib_flags          = \$(swig_lib_flags_apple)" \
    > Makefile.vars

    make
    PWFARM_SUDO make install
    make swig

    cp bct_py.py ~/polyworld_pwfarm/app/scripts
    cp bct_gsl.py ~/polyworld_pwfarm/app/scripts
    cp _bct_py.so ~/polyworld_pwfarm/app/scripts
    cp _bct_gsl.so ~/polyworld_pwfarm/app/scripts

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
    
    $pwfarm_dir/pwfarm_dispatcher.sh --password dispatch $PWHOSTNUMBERS $USER $tmp_dir/src.zip './scripts/farm/poly_build.sh --field' nil nil

    rm -rf $tmp_dir
fi
