#!/bin/bash

if [ $# == 0 ]; then
    op=steps
else
    op="$1"
fi

PWHOSTNUMBERS=`cat ~/polyworld_pwfarm/etc/pwhostnumbers`
USER=`cat ~/polyworld_pwfarm/etc/pwuser`

for x in $PWHOSTNUMBERS; do 
    id=`printf "%02d" $x`
    pwhostname=pw$id
    eval pwhost=\$$pwhostname

    case $op in
	'steps')
	    echo -n "$pwhostname = "

	    ssh -l $USER $pwhost ls -tr '~/polyworld_pwfarm/app/run/stats' | tail -n 1 | sed s/stat\.//
	    ;;
	'df')
	    echo "--- $pwhostname ---"
	    ssh -l $USER $pwhost df -H
	    ;;
	'*')
	    echo "Invalid op: $op">&2
	    exit 1
	    ;;
    esac
done