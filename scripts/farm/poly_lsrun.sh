#!/bin/bash

if [ $# == 0 ]; then
    echo "usage: $( basename $0 ) runid"
    echo
    echo "Checks if runid exists in either good or failed directories"

    exit 1
fi

runid=$1

PWHOSTNUMBERS=`cat ~/polyworld_pwfarm/etc/pwhostnumbers`
USER=`cat ~/polyworld_pwfarm/etc/pwuser`

for x in $PWHOSTNUMBERS; do 
    id=`printf "%02d" $x`
    pwhostname=pw$id
    eval pwhost=\$$pwhostname

    echo --- $pwhostname ---
    ssh -l $USER $pwhost "\
        cd
        if [ -e \"polyworld_pwfarm/runs/good/$runid\" ]; then
            file \"polyworld_pwfarm/runs/good/$runid\"
        fi
        if [ -e \"polyworld_pwfarm/runs/failed/$runid\" ]; then
            file \"polyworld_pwfarm/runs/failed/$runid\"
        fi"

done