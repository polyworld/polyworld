#!/bin/bash

SUFFIX=$1
if [ -z "$SUFFIX" ]; then
    echo Must provide suffix
    exit 1
fi

RUN=run$SUFFIX

time ./Polyworld
mv ./run $RUN

cd scripts
time ./MakeRecentDirectorywithEntirePopulation.sh ../$RUN
time ./CalcComplexityOverRecent -C P Recent ../$RUN