#!/bin/bash

SRCDIRS="agent\
         app\
         brain\
         complexity\
         debugger\
         environment\
         genome\
         graphics\
         logs\
         proplib\
         main\
         monitor\
         tools\
         ui\
         utils"

MISCDIRS="scripts\
          objects\
          etc"

MISCFILES="Makefile"

if [ $# != 1 ]; then
    name=`basename $0`
    echo "usage: $name output.zip"
    echo
    echo "Packages all the source code for the Polyworld source tree containing the script into output.zip"
    echo
    echo "Note: Can be executed from any working directory; will always package the tree containing it."
    
    exit 1
fi

function err()
{
    echo $* >&2; exit 1
}
function canonpath
{
    python -c "import os.path; print os.path.realpath('$1')"
}
function canondirname()
{
    dirname `canonpath "$1"`
}
SCRIPTS_DIR=$( canondirname $BASH_SOURCE )
function archive()
{
    $SCRIPTS_DIR/archive.sh "$@"
}


if [ -z "$1" ]; then
    err "Must specify output zip file"
fi

homedir=$(canonpath `dirname "$0"`/..)
output=$(canonpath "$1")

if [ -f "$output" ]; then
    rm "$output" || exit 1
fi

cd "$homedir"

archive pack \
    -x "*.o" -x "*.so" -x "*.pyc" \
    -x "*.eps" -x "*.pdf" -x "*.plt" \
    -x "scripts/bct_*.py" \
    -x "*/CVS/*" -x "*~" \
	$output $SRCDIRS $MISCDIRS $MISCFILES
