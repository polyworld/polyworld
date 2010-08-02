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
time ./MakeRecent ../$RUN
time ./CalcComplexity -C P Recent ../$RUN

