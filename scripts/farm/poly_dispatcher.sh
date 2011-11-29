#!/bin/bash

function canonpath()
{
    python -c "import os.path; print os.path.realpath('$1')"
}

function canondirname()
{
    dirname `canonpath "$1"`
}

pwfarm_dir=`canondirname "$0"`


PWHOSTNUMBERS=~/polyworld_pwfarm/etc/pwhostnumbers
USER=`cat ~/polyworld_pwfarm/etc/pwuser`

case "$1" in
    "clear")
	$pwfarm_dir/pwfarm_dispatcher.sh clear $PWHOSTNUMBERS $USER
	;;
    "recover")
	$pwfarm_dir/pwfarm_dispatcher.sh recover
	;;
    *)
	echo "Invalid mode: $1"
	exit 1
esac