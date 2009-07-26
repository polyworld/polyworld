#!/bin/bash

#
# make sure environment is ok and fetch PW_* environment variables
#
. `dirname $0`/env_check.sh $0 complexity gnuplot

BEGIN_AT_TIMESTEP=100
SAMPLE_EVERY=25

BF_FILE="$1"

if [ ! -f "$BF_FILE" ]
then
	echo "You must specify a single argument for the brainfunction file."
	exit;
fi
# brainFunction 11111 61 24
numneurons_in_agent=$(head -n 1 $BF_FILE | cut -d' ' -f3)
timesteps_in_lifetime=$(cat $BF_FILE | wc -l | sed -e 's/[ ]*//g' | awk -v numneur=${numneurons_in_agent} '{ print int( $1 / numneur) }')

if [ $timesteps_in_lifetime -lt 100 ]
then
	exit;
fi

echo "numneurons=$numneurons_in_agent"
echo "timestepsinlifetime=$timesteps_in_lifetime"
echo "Plotting..."

{
	current_timestep=${BEGIN_AT_TIMESTEP}

	while [ "$current_timestep" -le "$timesteps_in_lifetime" ]
	do
		data=$(${PW_CALC_COMPLEXITY} ${BF_FILE} ${current_timestep})
		Cplx_All=$(echo "$data" | grep '(All)' | awk -F'	' '{ print $2 }')
		Cplx_Pro=$(echo "$data" | grep '(Processing)' | awk -F'	' '{ print $2 }')
		Cplx_Inp=$(echo "$data" | grep '(Input)' | awk -F'	' '{ print $2 }')
		echo "$current_timestep	$Cplx_All	$Cplx_Pro	$Cplx_Inp"
		current_timestep=$(echo "${current_timestep} + ${SAMPLE_EVERY}" | bc)
	done

# Now one more time...
	data=$(${PW_CALC_COMPLEXITY} ${BF_FILE})
	Cplx_All=$(echo "$data" | grep '(All)' | awk -F'	' '{ print $2 }')
	Cplx_Pro=$(echo "$data" | grep '(Processing)' | awk -F'	' '{ print $2 }')
	Cplx_Inp=$(echo "$data" | grep '(Input)' | awk -F'	' '{ print $2 }')
	echo "$timesteps_in_lifetime	$Cplx_All	$Cplx_Pro	$Cplx_Inp"
} > ',temp'


${PW_GNUPLOT} << EOF

set xlabel 'Timestep'
set ylabel 'Neural Complexity'
set grid

set title "${BF_FILE} \n numneurons=${numneurons_in_agent}"
plot ",temp" using 1:2 w lp lw 2 title "All", ",temp" using 1:3 w lp lw 2 title "Processing", ",temp" using 1:4 w lp lw 2 title "Input"

EOF

rm ,temp
