#!/bin/sh
# This program will generate a timestep '0' in the bestRecent, Recent, and bestSoFar directory.


# remove any trailing slashes from the run directory
directory=$(echo "${1}" | sed -e 's/\/*$//')
numseedcritters=$(echo "${2}" | awk '{ print int($0) }')
if [ "$numseedcritters" -gt 0 ]
then
	echo "You have force-specified the number of seed critters to '$numseedcritters'"
fi

if [ ! -d "$directory" ]
then
        echo "You must specify a polyworld 'run/' directory to generate timestep '0' in brain/bestRecent, brain/bestSoFar, and brain/Recent.";
        exit;
fi

if [ ! -f "$directory/BirthsDeaths.log" ]
then
	# Welp, no BirthsDeaths, can we get it from the worldfile?
	if [ -f "$directory/worldfile" ]
	then
		if [ ! -f "$directory/LOCKSTEP-BirthsDeaths.log" ]
		then
			if [ "$numseedcritters" -le 0 ] # if we haven't forced-specified a number of seed critters, exit.
			then
       	 			echo "* Error: Need a BirthsDeaths.log or worldfile, and I don't see any file '$directory/BirthDeaths.log'."; 
				echo "Exiting."
				exit;
			fi
		fi
	fi
fi


if [ ! -d "$directory/brain/function" ]
then
        echo "Need a brain/function directory that has the brainFunction files, and I don't see any directory '$directory/brain/function'."; 
        exit;
fi

if [ ! -d "$directory/brain/bestRecent" ]
then
        echo "I need a brain/bestRecent directory to determine what time interval to bin, and I don't see any directory '$directory/brain/bestRecent'.";
        exit;
else
	if [ -d "${directory}/brain/bestRecent/0" ]
	then
		rm -rf "${directory}/brain/bestRecent/0"
	fi
	
	mkdir "${directory}/brain/bestRecent/0"
fi

if [ ! -d "$directory/brain/bestSoFar" ]
then
        echo "Warning: no ${directory}/brain/bestSoFar directory.  This isn't required, but it should probably be here. "
else
	if [ -d "${directory}/brain/bestSoFar/0" ]
	then
		rm -rf "${directory}/brain/bestSoFar/0"
	fi

	mkdir "${directory}/brain/bestSoFar/0"
fi

if [ ! -d "$directory/brain/Recent" ]
then
        echo "Warning: no ${directory}/brain/Recent directory.  This isn't required, but it should probably be here. "
else
	if [ -d "${directory}/brain/Recent/0" ]
	then
		rm -rf "${directory}/brain/Recent/0"
	fi

	mkdir "${directory}/brain/Recent/0"
fi


# if we didn't force specify a number of seed critters determine it now
if [ "${numseedcritters}" -le 0 -a -d "$directory/brain/seeds/function" ] 
then
	echo "Determining number of seed critters dyanmically from the $directory/brain/seeds/function/..."
	numseedcritters=$(ls -1 "$directory/brain/seeds/function/" | wc -l | tr -d ' ')
fi
if [ "${numseedcritters}" -le 0 -a -f "$directory/worldfile" ]
then
	echo "Determining number of seed critters dyanmically from the worldfile..."
	numseedcritters=$(grep 'initnumcritters' "$directory/worldfile" | awk -F' ' '{ print $1 }')
fi
if [ "${numseedcritters}" -le 0 -a -f "$directory/BirthsDeaths.log" ]
then
	echo "Determining number of seed critters dyanmically from the BirthsDeaths.log..."
	numseedcrittersplusone=$(grep ' BIRTH ' ${directory}/BirthsDeaths.log | head -n 1 | cut -d' ' -f3)
	numseedcritters=$(echo "$numseedcrittersplusone - 1" | bc)
elif [ "${numseedcritters}" -le 0 -a -f "$directory/LOCKSTEP-BirthsDeaths.log" ]	# is it STILL <= 0 ?
then
	echo "Determining number of seed critters dyanmically from the LOCKSTEP-BirthsDeaths.log..."
	numseedcrittersplusone=$(grep ' BIRTH ' ${directory}/LOCKSTEP-BirthsDeaths.log | head -n 1 | cut -d' ' -f3)
	numseedcritters=$(echo "$numseedcrittersplusone - 1" | bc)
fi


echo "Number of Seed Critters = $numseedcritters"

# First, populate the brain/Recent with the entire seed population.
if [ -d "$directory/brain/Recent/0" ]
then
	echo "Populating ${directory}/brain/Recent/0 with $numseedcritters brainFunction files..."
	critter=1
	while [ $critter -le $numseedcritters ]
	do
		filename="brainFunction_$critter.txt"
		abstractfile ln "${directory}/brain/function/$filename" "${directory}/brain/Recent/0/$filename"
		critter=$(echo "$critter + 1" | bc)
	done
fi

# Second, determine the N best fit.
# Get the average number of *brainFunction* files in each brain/bestRecent/{timestep} ...
numberofbins=$(for timestep in ${directory}/brain/bestRecent/[1-9]*			# [1-9] excludes timestep zero.
do
	echo $timestep
done | wc -l)

numberofbrainfunctionfiles=$(for brainfunctionfile in `ls -1 ${directory}/brain/bestRecent/[1-9]*/*brainFunction*`
do
	echo $brainfunctionfile
done | wc -l)
# echo "numberofbins = $numberofbins"
# echo "numberofbrainfunctionfiles = $numberofbrainfunctionfiles"

AvrNumBrainFuncFiles=$(echo "" | awk -v sum="$numberofbrainfunctionfiles" -v samples="$numberofbins" '{ mean= sum / samples; if( mean != int(mean) ) { mean = int(mean+1); } print mean; }')

echo "Getting the '$AvrNumBrainFuncFiles' most fit seed critters..."

critter=1
data=$(while [ $critter -le $numseedcritters ]
        do
		fitness=$(abstractfile tail -1 "${directory}/brain/function/brainFunction_${critter}.txt" | cut -d' ' -f4)
		echo "$critter	$fitness"
		critter=$(echo "$critter + 1" | bc)
done | sort -t'	' -k2 -rn | head -n "${AvrNumBrainFuncFiles}")

NumsOfMostFitCritters=$(echo "$data" | cut -f1)
# CritterRank=$(echo "$AvrNumBrainFuncFiles - 1" | bc)
CritterRank=0

echo "Most Fit Seed Critters: ${NumsOfMostFitCritters}" | tr '\n' ' '; echo "";

NumMoved=0
for critternum in ${NumsOfMostFitCritters}
do
	filename="brainFunction_$critternum.txt"
	bestRecentfilename="${CritterRank}_${filename}"
	if abstractfile ln "${directory}/brain/function/$filename" "${directory}/brain/bestRecent/0/$bestRecentfilename"
	then
		NumMoved=$(echo "$NumMoved + 1" | bc)
	fi
	CritterRank=$(echo "$CritterRank + 1" | bc)
done

echo "Linked $NumMoved critters into \'${directory}/brain/bestRecent/0\'"
