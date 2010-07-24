#!/bin/bash

./scripts/CalcEvents c run

cd scripts
./MakeRecentDirectorywithEntirePopulation.sh ../run
./CalcComplexityOverRecent Recent ../run


