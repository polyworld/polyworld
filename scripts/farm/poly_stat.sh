#!/bin/bash

PWHOSTNUMBERS=`cat ~/polyworld_pwfarm/etc/pwhostnumbers`
USER=`cat ~/polyworld_pwfarm/etc/pwuser`

for x in $PWHOSTNUMBERS; do 
    id=`printf "%02d" $x`
    pwhostname=pw$id
    eval pwhost=\$$pwhostname

    echo -n "$pwhostname = "

    ssh -l $USER $pwhost ls -tr '~/polyworld_pwfarm/app/run/stats' | tail -n 1 | sed s/stat\.//
done