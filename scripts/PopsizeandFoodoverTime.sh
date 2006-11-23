#!/bin/sh

# if there is a trailing slash, get rid of it.
directory=$(echo "${1}" | sed -e 's/\/*$//')

if [ ! -r "$directory/stat.1" ]
then
        echo "you must specify a Polyworld 'run/stats' directory to print the population size at each timestep.";
        exit;
fi

cd "${directory}"

# Are we going to print the header with MATLAB style comments or GNUPLOT style comments?  Default is GNUPLOT style comments.
if [ "$2" == "m" ]
then
        echo "%Time	NumCritters	Food	CurNeurGroups	CurNeurGroupsStdErr"
else
        echo "#Time	NumCritters	Food	CurNeurGroups	CurNeurGroupsStdErr"
fi

ls -1 | sort -t '.' -k2 -n | while read statfile
do
	timestep=$(echo "$statfile" | cut -d'.' -f2)
	numcritters=$(grep '^critters = ' "$statfile" | awk -F' ' '{ print int($3) }')
	amountfood=$(grep '^food = ' "$statfile" | awk -F' ' '{ print int($3) }')
	CurNeurGroups=$(grep '^CurNeurGroups = ' "$statfile" | awk -F' ' '{ print $3 }')
	CurNeurGroupsStdDev=$(grep '^CurNeurGroups = ' "$statfile" | awk -F' ' '{ print $5 }')

	LifeSpan=$(grep '^LifeSpan = ' "$statfile" | awk -F' ' '{ print $3 }')
	LifeSpanStdDev=$(grep '^LifeSpan = ' "$statfile" | awk -F' ' '{ print $5 }')

	# StdDev tells us about the variance within a population.  StdErr tells us how representative the mean is.
	CurNeurGroupsStdErr=$(echo "$CurNeurGroupsStdDev" | awk -v samplesize="$numcritters" '{ print $1 / sqrt(samplesize) }')

	echo "$timestep	$numcritters	$amountfood	$CurNeurGroups	$CurNeurGroupsStdErr	$LifeSpan	$LifeSpanStdDev"
done
