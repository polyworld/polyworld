#!/bin/sh

CALC_COMPLEXITY_OVER_DIRECTORY="/polyworld/scripts/CalcComplexityOverDirectory.sh"

# Make sure we're taking an argument
if [ -z "$1" ]
then
	echo "you must specify a Polyworld 'bestRecent' or 'Recent' directory to calculate the average Complexity at each timestep.";
	exit;
fi


# if there is a trailing slash, get rid of it.
directory=$(echo "${1}" | sed -e 's/\/*$//')

# Go into the brainFunction directory (where the timesteps are)
cd "${directory}"
# Are we going to print the header with MATLAB style comments or GNUPLOT style comments?  Default is GNUPLOT style comments.
if [ "$2" == "m" ]
then
	echo "%Time	meanAll 	stderrAll	meanPro 	stderrPro	meanInp 	stderrInp	NumSamples"
else
	echo "#Time	meanAll 	stderrAll	meanPro 	stderrPro	meanInp 	stderrInp	NumSamples"
fi


	# list the timesteps, make sure they are all integers, and sort them by number.
	ls -1 | awk 'int($1) == $1 { print $0 }' | sort -n | while read timestep
	do
#		echo "directory: $timestep"
		# the 1st awk at the end strips any comment (beginning with a '#' or '%') lines, and any blank lines
		# the 2nd awk at the end strips out any values where the Complexity is 0.0.
		data=$(${CALC_COMPLEXITY_OVER_DIRECTORY} ${directory}/${timestep} | awk '$0 !~ /^[\#\%]/ && $0 != "" { print $0 }' | awk '$2 != 0 && $3 != 0 && $4 != 0 { print $0 }')

		if [ ! -z "$data" ]
		then
			NumSamples=$(echo "$data" | wc -l | tr -d '	 ')	# the 'tr' is to remove any tabs/spaces that sneak into NumSamples
		else
			NumSamples=0
		fi


		CplxAll=$(echo "${data}" | cut -f2)
		CplxPro=$(echo "${data}" | cut -f3)
		CplxInp=$(echo "${data}" | cut -f4)

#		echo "---------"
#		echo "ALL:"
#		echo "$CplxAll" | tr '\n' ' ';
#		echo "---------"
#		echo "PRO:"
#		echo "$CplxPro" | tr '\n' ' ';
#		echo "---------"
#		echo "INP:"
#		echo "$CplxInp" | tr '\n' ' ';

		meanAll=$(echo "$CplxAll" | awk 'BEGIN { sum=0; records=0; } NF>1 { print "Must have only 1 column.  You have " NF " columns, line is: " $0; exit -1; } NF==1 && $0 != "" { sum += $1; records += 1; } END { if( records == 0 ) { print "0"; } else { print sum / records } }')
		meanPro=$(echo "$CplxPro" | awk 'BEGIN { sum=0; records=0; } NF>1 { print "Must have only 1 column.  You have " NF " columns, line is: " $0; exit -1; } NF==1 && $0 != "" { sum += $1; records += 1; } END { if( records == 0 ) { print "0"; } else { print sum / records } }')
		meanInp=$(echo "$CplxInp" | awk 'BEGIN { sum=0; records=0; } NF>1 { print "Must have only 1 column.  You have " NF " columns, line is: " $0; exit -1; } NF==1 && $0 != "" { sum += $1; records += 1; } END { if( records == 0 ){ print "0"; } else { print sum / records } }')

		stderrAll=$(echo "$CplxAll" | awk -v mean="$meanAll" 'BEGIN { sumsquares=0; } $0 !="" { sumsquares += ($1 - mean)**2; samples += 1; } END { denominator=((samples - 1) * samples); if( denominator > 0 ) { result=sqrt(sumsquares / denominator); } else { result = 0; } print result }')
		stderrPro=$(echo "$CplxPro" | awk -v mean="$meanPro" 'BEGIN { sumsquares=0; } $0 !="" { sumsquares += ($1 - mean)**2; samples += 1; } END { denominator=((samples - 1) * samples); if( denominator > 0 ) { result=sqrt(sumsquares / denominator); } else { result = 0; } print result }')
		stderrInp=$(echo "$CplxInp" | awk -v mean="$meanInp" 'BEGIN { sumsquares=0; } $0 !="" { sumsquares += ($1 - mean)**2; samples += 1; } END { denominator=((samples - 1) * samples); if( denominator > 0 ) { result=sqrt(sumsquares / denominator); } else { result = 0; } print result }')

		echo "$timestep	$meanAll	$stderrAll	$meanPro	$stderrPro	$meanInp	$stderrInp	$NumSamples" 
	

done | tee AvrComplexity.plt

if [ ! -s 'AvrComplexity.plt' ]
then
	rm AvrComplexity.plt
fi
# if AvrComplexity is empty, we should delete it.
