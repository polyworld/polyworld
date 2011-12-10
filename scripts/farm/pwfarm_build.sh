#!/bin/bash

if [ -z "$PWFARM_SCRIPTS_DIR" ]; then
    source $( dirname $BASH_SOURCE )/__pwfarm_runutil.sh || exit 1
else
    source $PWFARM_SCRIPTS_DIR/__pwfarm_runutil.sh || exit 1
fi

function usage()
{    
    cat <<EOF
usage: $( basename $0 ) [--nobct] [-f:h]

    Build Polyworld on farm.

OPTIONS:

   --nobct        Don't build BCT.

   -f fields
                  Specify fields on which this should run. Must be a single argument,
                so use quotes. e.g. -f "0 1" or -f "{0..3}"

   -h             Show this message.
EOF
    exit 1
}

set -e

function build_bct()
{
    pushd_quiet .

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

    cp bct_py.py "$POLYWORLD_PWFARM_APP_DIR/scripts"
    cp bct_gsl.py "$POLYWORLD_PWFARM_APP_DIR/scripts"
    cp _bct_py.so "$POLYWORLD_PWFARM_APP_DIR/scripts"
    cp _bct_gsl.so "$POLYWORLD_PWFARM_APP_DIR/scripts"

    popd_quiet
}

if [ "$1" == "--field" ]; then
    shift
    lock_app || exit 1

    if [ "$1" == "--nobct" ]; then
	bct=false
	shift
    else
	bct=true
    fi

    store_orphan_run "$POLYWORLD_PWFARM_APP_DIR/run"

    if [ -e "$POLYWORLD_PWFARM_APP_DIR" ]; then
	mkdir -p /tmp/polyworld_pwfarm
	bak=/tmp/polyworld_pwfarm/`date | sed -e "s/ /_/g" -e "s/:/./g"`
	echo "$POLYWORLD_PWFARM_APP_DIR exists! Moving to $bak"
	
	mv "$POLYWORLD_PWFARM_APP_DIR" "$bak"
    fi

    mkdir -p "$POLYWORLD_PWFARM_APP_DIR"
    mv * "$POLYWORLD_PWFARM_APP_DIR"
    cd "$POLYWORLD_PWFARM_APP_DIR"

    if $bct; then
	build_bct
    fi

    make

    unlock_app
else
    opts=""
    if [ "$1" == "--nobct" ]; then
	opts="$1"
	shift
    fi

    while getopts "f:h" opt; do
	case $opt in
	    f)
		__pwfarm_config env set fieldnumbers "$OPTARG"
		validate_farm_env
		;;
            h)
		usage
		;;
	    *)
		exit 1
		;;
	esac
    done
    shift $(( $OPTIND - 1 ))

    pwfarm_dir=$( canondirname "$0" )
    poly_dir=$( canonpath "$pwfarm_dir/../.." )
    scripts_dir=$( canonpath "$poly_dir/scripts" )

    tmp_dir=`mktemp -d /tmp/poly_build.XXXXXXXX` || exit 1

    cd $poly_dir

    scripts/package_source.sh $tmp_dir/src.zip
    
    $pwfarm_dir/__pwfarm_dispatcher.sh --password dispatch $tmp_dir/src.zip "./scripts/farm/pwfarm_build.sh --field $opts $*" nil nil

    rm -rf $tmp_dir
fi
