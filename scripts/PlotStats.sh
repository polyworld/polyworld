#!/bin/sh
########################################################################################################
# CONFIGURABLE OPTIONS
GNUPLOT="/sw/bin/gnuplot"
MAX_NUMBER_OF_PARAMS_TO_PLOT=5
# Don't modify anything beneath here
########################################################################################################

STATS_DIR=$(echo "${1}" | sed -e 's/\/$//')	# removing any trailing slash
TO_GET="$2"

if [ ! -x "${GNUPLOT}" ]
then
	echo "Gnuplot at '${GNUPLOT}' isn't executable.  Change the variable or make it so.";
	exit;
fi

if [ ! -d "$STATS_DIR" ]
then
	echo "You must specify a polyworld run/stats/ directory."
	exit;
fi

if [ -z "$TO_GET" ]
then
	echo "You must specify one or more attributes to be retrieved from the stats.#### files."
	exit;
fi

# If user is being silly and specifies a run/ directory instead of a run/stats directory, fix it instead of spewing an error.
if [ -d "$STATS_DIR/stats" ]
then
	STATS_DIR="$STATS_DIR/stats"
fi


run_dir=$(echo "$STATS_DIR" | sed -e 's/\/stats//')				# we use this for the title on the plot
LAST_TIMESTEP=$(ls -1 $STATS_DIR | sed -e 's/stat\.//' | sort -n | tail -n 1)
secondtolast_timestep=$(ls -1 $STATS_DIR | sed -e 's/stat\.//' | sort -n | tail -n 2 | head -n 1)
TIMESTEP_INTERVAL=$(echo "$LAST_TIMESTEP - $secondtolast_timestep" | bc)
echo "- Stats Directory=$STATS_DIR"
echo "- Last Timestep=$LAST_TIMESTEP"
echo "- Timestep Interval=$TIMESTEP_INTERVAL"

#param1,param2,param3

# Check to make sure all of the specified parameters are actually there.
# the sed here is to properly escape any hyphens in the input before they goto grep
params=$(echo "$TO_GET" | sed -e 's/-/\\-/g' | tr -s ',' '\n')		# the -s here is important to collapse multiple comas. ',,'->','

# if there is NOT a '#' on the end of each param, put them in here.
params=$(for param in ${params}	
do
	has_crunch=$(echo "$param" | grep -o '#[0-9]*$')
	if [ -z "$has_crunch" ]
	then
		with_crunch=$(echo "${param}#1")
	else
		with_crunch=$param
	fi

	echo "$with_crunch"
done)

# Make a whole new param array that DOES NOT have the crunches
params_withoutcrunch=$(for param in ${params}
do
	echo "$param" | sed -e 's/#[0-9]*$//'
done)


# Check to make sure all of the specified parameters exist within the stat.#### files, and that they only specify a single line.
exit_error=0
number_of_parameters=0
for param in ${params}
do
	# Take off the crunch
	param_withoutcrunch=$(echo "$param" | sed -e 's/#[0-9]*$//')

	number_of_parameters=$(echo "$number_of_parameters + 1" | bc)
	number=$(grep -c "^[ ]*$param_withoutcrunch[^A-Za-z]*" ${STATS_DIR}/stat.1)

	echo "- param${number_of_parameters}=$param"

	if [ "$number" -le "0" ]		# I know techincally it should be 'eq' not 'le', but if there's a negative we shouldn't continue then either.
	then
		echo "Error: I did not see a value '$param_withoutcrunch' in '${STATS_DIR}/stat.1"
		exit_error=1

	elif [ "$number" -ge "2" ]
	then
		echo "The parameter '$param_withoutcrunch' is ambiguous.  Possibilities: "
		grep "^[ ]*$param_withoutcrunch[^A-Za-z]*" $STATS_DIR/stat.1
		exit_error=1
	fi

done

# If one of the parameters was invalid, exit.
if [ "$exit_error" -eq "1" ]
then
	exit;
fi

if [ "$number_of_parameters" -gt "$MAX_NUMBER_OF_PARAMS_TO_PLOT" ]
then
	echo "Error: You're trying to plot '$number_of_parameters' parameters, but the maximum allowed is '$MAX_NUMBER_OF_PARAMS_TO_PLOT'"
	exit;
fi

# Okay, all of our parameters are valid, lets process them!

# This part is a little strange.  Here's how it works.
# 1) the awk script is given a list of the parameters.  It builds an array 'DATA[t<timestep>_<index>]'   -- where the indexes are 1...numparameters
# 2) It gets the values to put into the DATA array via a '<cmd> | getline' statement.  It just so happens that here the <cmd> has lots of quotes and nesting.
# 3) After it creates the DATA array in the main part, it prints DATA within the END { }.
# 4) In both the Main { } and End { } part it handles the timestep=1 case separately from the rest.
echo "- Grepping through $STATS_DIR..."
echo "${params}" | awk -v TIMESTEP_INTERVAL="$TIMESTEP_INTERVAL" -v LAST_TIMESTEP="$LAST_TIMESTEP" -v STATS_DIR="$STATS_DIR" '
{
	split($0,temp,"#");
	param=temp[1];
	column=temp[2];

	# HACK ALERT!! -- If param has a number [0-9], in it, we must increment the column by 1.  This is to prevent us accidentially from printing the number within the param.
	if( match(param, "[0-9]") ) { column = column + 1; }

	# do the first timestep
	# Steps: 1) Grep for the param; 2) strip everything except for numbers and spaces; 3) collapse multiple spaces; 4) remove any leading spaces; 5) get the intended column; 6) store into variable
	"grep \"^[ ]*" param "[^A-Za-z]*\" " STATS_DIR "/stat.1 | sed -e \"s/[^0-9\. ]//g\" | tr -s \" \" | sed -e \"s/^[ ]*//\" | cut -d\" \" -f" column | getline DATA["t1_" NR];
	close("grep \"^[ ]*" param "[^A-Za-z]*\" " STATS_DIR "/stat.1 | sed -e \"s/[^0-9\. ]//g\" | tr -s \" \" | sed -e \"s/^[ ]*//\" | cut -d\" \" -f" column);

#	print "t1______" param "=" DATA["t1_" NR];

	# do all the other timesteps
	for( current_timestep=TIMESTEP_INTERVAL; current_timestep <= LAST_TIMESTEP; current_timestep += TIMESTEP_INTERVAL )
	{

		"grep \"^[ ]*" param "[^A-Za-z]*\" " STATS_DIR "/stat." current_timestep " | sed -e \"s/[^0-9\. ]//g\" | tr -s \" \" | sed -e \"s/^[ ]*//\" | cut -d\" \" -f" column | getline DATA["t" current_timestep "_" NR];
		close("grep \"^[ ]*" param "[^A-Za-z]*\" " STATS_DIR "/stat." current_timestep " | sed -e \"s/[^0-9\. ]//g\" | tr -s \" \" | sed -e \"s/^[ ]*//\" | cut -d\" \" -f" column);

#		print "t" current_timestep "______" param "=" DATA["t" current_timestep "_" NR]

	}


} END {

	# print the first timestep
	printf "1\t";
	for( i=1; i<=NR; i++ ) { printf "%s\t", DATA["t1_" i]; }
	printf "\n";

	# print the rest of the timesteps
	for( current_timestep=TIMESTEP_INTERVAL; current_timestep <= LAST_TIMESTEP; current_timestep += TIMESTEP_INTERVAL )
	{
		printf "%s\t", current_timestep;
		for( i=1; i<=NR; i++ ) { printf "%s\t", DATA["t" current_timestep "_" i]; }
		printf "\n"
	}
}' > ,temp

echo "Humancheck -- beginning of plotfile:"
head -n 5 ,temp

#### Okay, we've now put all of our data into ,temp.  Time to plot it!
echo "- Plotting..."

# Because we have a variable number of things that we could be plotting, we have to dynamically generate the plot string in shellscript before passing it to GNUplot.
GNUPLOT_plotstring="plot"
GNUPLOT_ylabel=""
column=2;

for param in ${params}
do
	GNUPLOT_ylabel="$GNUPLOT_ylabel $param,"
	GNUPLOT_plotstring="$GNUPLOT_plotstring ',temp' using 1:$column w lp lw 2 title '$param',"
	column=$(echo "$column + 1 " | bc)
done

# take off the final comas
GNUPLOT_plotstring=$(echo "$GNUPLOT_plotstring" | sed -e 's/,$//')
GNUPLOT_ylabel=$(echo "$GNUPLOT_ylabel" | sed -e 's/,$//')

# echo "plotstring = '$GNUPLOT_plotstring'"

# Okay, lets plot!
${GNUPLOT} << EOF

set xlabel 'Timestep'
# set ylabel '${GNUPLOT_ylabel}'
set grid
set mxtics 5
set xrange [0:${LAST_TIMESTEP}]
set title "${run_dir}\n${GNUPLOT_ylabel}"
${GNUPLOT_plotstring}

EOF

echo "- Deleted ,temp file."
rm ,temp

echo "Done!"
