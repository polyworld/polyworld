#!/bin/bash

if [ -z "$PWFARM_SCRIPTS_DIR" ]; then
    source $( dirname $BASH_SOURCE )/__pwfarm_runutil.sh || exit 1
else
    source $PWFARM_SCRIPTS_DIR/__pwfarm_runutil.sh || exit 1
fi

########################################
###
### USAGE
###
########################################
function usage()
{    
    cat <<EOF
usage: $( basename $0 ) [-cf:bh]

    Build Polyworld on farm.

OPTIONS:

   -c             Clean build.

   -f fields
                  Specify fields on which this should run. Must be a single argument,
                so use quotes. e.g. -f "0 1" or -f "{0..3}"

   -b             Don't build BCT.

   -h             Show this message.
EOF
    exit 1
}

########################################
###
### DETERMINE MACHINE ROLE
###
########################################
if [ "$1" == "--field" ]; then
    field=true
    shift
else
    field=false
fi

########################################
###
### PROCESS OPTIONS
###
########################################
clean=false
bct=true

while getopts "cf:bh" opt; do
    case $opt in
	c)
	    clean=true
	    ;;
	f)
	    if ! $field; then
		__pwfarm_config env set fieldnumbers "$OPTARG"
		validate_farm_env
	    fi
	    ;;
	b)
	    bct=false
	    ;;
        h)
	    usage
	    ;;
	*)
	    exit 1
	    ;;
    esac
done

args=$( encode_args "$@" )

set -e

########################################
###
### BCT BUILD ROUTINE
###
########################################
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

    if [ "$(svn info -r HEAD | grep 'Revision:')" == "$(cat .pwfarm_build.revision)" ]; then
	popd_quiet
	return 0
    fi

    PWFARM_STATUS "Building BCT"

    local install_dir=$( canonpath ../bct-install )
    mkdir -p $install_dir/include
    mkdir -p $install_dir/lib


    # Note: If you change the following contents of Makefile.vars, be sure to escape $ (\$)
    echo "\
# Arguments to be sent to the C++ compiler
# Some arguments may already be specified in Makefile
CXXFLAGS                += -m32 -fopenmp -DGSL_DOUBLE

# Installation directory
install_dir              = $install_dir

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
    make install
    make swig

    cp bct_py.py "$POLYWORLD_PWFARM_APP_DIR/scripts"
    cp bct_gsl.py "$POLYWORLD_PWFARM_APP_DIR/scripts"
    cp _bct_py.so "$POLYWORLD_PWFARM_APP_DIR/scripts"
    cp _bct_gsl.so "$POLYWORLD_PWFARM_APP_DIR/scripts"

    svn info | grep "Revision:" > .pwfarm_build.revision

    popd_quiet
}

if $field; then
    ########################################
    ###
    ### BUILD LOGIC ON FIELD MACHINE
    ###
    ########################################
    lock_app || exit 1

    store_orphan_run "$POLYWORLD_PWFARM_APP_DIR/run"

    # Generate list of files we just transferred
    find . > .pwfarm_build.payload

    # Check if any files have been deleted.
    if ! $clean && [ -e "$POLYWORLD_PWFARM_APP_DIR/.pwfarm_build.payload" ]; then
	function files_deleted()
	{
	    old="$1"
	    new="$2"
	    (
		cat "$old" | awk '{ print "old\t"$0 }'
		cat "$new" | awk '{ print "new\t"$0 }'
	    ) |
	    sort -k 2 |
	    uniq -u -f 1 |
	    cut -f 1 |
	    grep old > /dev/null
	}

	if files_deleted "$POLYWORLD_PWFARM_APP_DIR/.pwfarm_build.payload" .pwfarm_build.payload; then
	    echo "DETECTED DELETED FILES. FORCING CLEAN."
	    clean=true
	fi
    else
	echo "DIDN'T FIND PAYLOAD LIST. FORCING CLEAN."
	clean=true
    fi

    if $clean && [ -e "$POLYWORLD_PWFARM_APP_DIR" ]; then
	mkdir -p /tmp/polyworld_pwfarm
	bak=/tmp/polyworld_pwfarm/`date | sed -e "s/ /_/g" -e "s/:/./g"`
	echo "$POLYWORLD_PWFARM_APP_DIR exists! Moving to $bak"
	
	mv "$POLYWORLD_PWFARM_APP_DIR" "$bak"
    fi

    mkdir -p "$POLYWORLD_PWFARM_APP_DIR"
    cp -r . "$POLYWORLD_PWFARM_APP_DIR"
    cd "$POLYWORLD_PWFARM_APP_DIR"

    if $bct; then
	build_bct
    fi

    if $clean; then
	PWFARM_STATUS "Clean Build"
    else
	PWFARM_STATUS "Incremental Build"
    fi

    make

    unlock_app
else
    ########################################
    ###
    ### LOCAL PREPARATION
    ###
    ########################################
    tmp_dir=$( create_tmpdir ) || exit 1

    cd $POLYWORLD_DIR

    payload=$tmp_dir/src.zip
    scripts/package_source.sh $payload

    tasks=$( taskmeta create_field_tasks $tmp_dir "./scripts/farm/pwfarm_build.sh --field $args" nil )
    
    dispatcher dispatch $payload $tasks

    rm -rf $tmp_dir
fi
