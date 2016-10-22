#!/bin/bash

if [ $# == 0 ]; then
    echo "usage: $( basename $0 ) gccbin.tar.gz"
    exit 1
fi

if [ "$1" != "--field" ]; then
    if [ ! -e "$1" ]; then
	echo "Cannot locate $1" >&2
	exit 1
    fi

    __pwfarm_script.sh --password --input $1 $0 --field $*
else
    shift

    archive=$( basename $1 )

    PWFARM_SUDO tar -xvf $archive -C /
fi