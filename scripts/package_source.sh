#!/bin/bash

SRCDIRS="agent\
         app\
         brain\
         complexity\
         debugger\
         environment\
         genome\
         graphics\
         tools\
         ui\
         utils"

MISCDIRS="scripts\
          objects"

MISCFILES="Makefile\
           default.wfs"

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

if [ -z "$1" ]; then
    err "Must specify output zip file"
fi

homedir=$(canonpath `dirname "$0"`/..)
output=$(canonpath "$1")

if [ "${output##*.}" != "zip" ]; then
    err "Not a path with zip extension: $output"
fi

if [ -f "$output" ]; then
    rm "$output" || exit 1
fi

cd "$homedir"

zip -qr "$output" $SRCDIRS $MISCDIRS $MISCFILES -x "*.pyc" -x "*/CVS/*" -x "*~"