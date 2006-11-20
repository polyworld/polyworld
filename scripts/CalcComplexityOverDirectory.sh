#!/bin/sh
CALC_COMPLEXITY="$pol/utils/CalcComplexity"

# If the CalcComplexity command-line tool isn't compiled, compile it.
if [ ! -x "$CALC_COMPLEXITY" ]
then
	g++ -lgsl -lgslcblas -lm -O3 '/polyworld/utils/CalcComplexity.cpp' -o ${CALC_COMPLEXITY}
fi

# Make sure we're taking an argument
if [ -z "$1" ]
then
	echo "you must specify a directory containing brainFunction files to calculate the Complexity of";
	exit;
fi

# if there is a trailing slash, get rid of it.
directory=$(echo "${1}" | sed -e 's/\/*$//')

# Go into the brainFunction directory (where the timesteps are)
cd "$directory"

echo "#CritterNum	All	Processing	Input"
ls -1 | awk '$1 ~ /brainFunction/ && $1 !~ /incomplete_/ { print $0 }' | sort -n -t '_' -k2,4 | while read funcfile
do
	critternum=$(echo "$funcfile" | sed -e 's/^[0-9]*_//' | cut -f2 -d'_'  | cut -f1 -d'.')	# get the critter number out

	# now get the complexties.  They are in Columns: All / Processing / Input

	line=$(${CALC_COMPLEXITY} $funcfile | awk -F'	' 'NR==1{  all=$2 } NR==2{ processing=$2 } NR==3{ input=$2 } END { print all "	" processing "	" input }')

	echo "$critternum	$line	${directory}/${funcfile}"	# now echo the line

done


