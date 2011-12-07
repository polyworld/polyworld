#!/bin/bash

source $( dirname $BASH_SOURCE )/__lib.sh || exit 1


if [ $# == "0" ]; then
    echo "\
usage: $( basename $0 ) (clear|recover|disconnect)" >&2
    exit 1
fi

case "$1" in
    "clear")
	$PWFARM_SCRIPTS_DIR/__pwfarm_dispatcher.sh clear
	;;
    "recover")
	$PWFARM_SCRIPTS_DIR/__pwfarm_dispatcher.sh recover
	;;
    "disconnect")
	$PWFARM_SCRIPTS_DIR/__pwfarm_dispatcher.sh disconnect
	;;
    *)
	echo "Invalid mode: $1"
	exit 1
esac