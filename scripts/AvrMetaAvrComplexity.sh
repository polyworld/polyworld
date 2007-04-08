#!/bin/sh
# This script takes a raw cat of multiple AvrComplexity.plt's put together, and averages them all together.

if [ -z "$1" ]
then
	echo "You must specify a metaAvrComplexity.plt file"
	exit
fi

metaplotfile="$1"
column1=$(cat "$metaplotfile" | cut -f1 | sort -n | uniq)

timesteps=$(echo "$column1" | awk '{ if( $0 == int($0)) { print $0 } }')	# only consider the integers, no strings, no decimals.

echo "$timesteps" | tr '\n' ' ' | awk '{ print "Timesteps: " $0 }'

for timestep in $timesteps
do
#	echo "grep "^$timestep" "$metaplotfile""
	data=$(grep "^$timestep	" "$metaplotfile")

	echo "$data" | cut -f2,4,6,8 | awk -v timestep="$timestep" -F'	' 'BEGIN { sumAll=0; sumPro=0; sumInp=0; TotalSamples=0; } { if(NF != 4) { print "There *must* be four fields, but I see " NF; exit; } TotalSamples+=$4; sumAll+=($1*$4); sumPro+=($2*$4); sumInp+=($3*$4); } END { print timestep "	" sumAll/TotalSamples "	" sumPro/TotalSamples "	" sumInp/TotalSamples "	" TotalSamples; }'

done
