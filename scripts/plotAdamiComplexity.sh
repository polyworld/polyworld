#!/bin/sh
########################################################################################################
# Usage: ./plotNeuralComplexity.sh <run_directory> [IPA]
# I = plot input neural complexity
# P = plot processing neural complexity
# A = plot All neural complexity
# if no options are specified, it plots all three.
########################################################################################################
# CONFIGURABLE OPTIONS
GNUPLOT="/sw/bin/gnuplot"
FILENAME="genome/AdamiComplexity-summary.txt"
GNUPLOT_ylabel="Adami Complexity"
GNUPLOT_labelparams='font "Times,20"'
GNUPLOT_titleparams='font "Times,20"'
# Don't modify anything beneath here
########################################################################################################

FLAG_plotOnebit=0
FLAG_plotTwobit=0
FLAG_plotFourbit=0

RUN_DIR=$(echo "${1}" | sed -e 's/\/$//')	# removing any trailing slash
TO_PLOT="$2"

if [ ! -x "${GNUPLOT}" ]
then
	echo "Gnuplot at '${GNUPLOT}' isn't executable.  Change the variable or make it so.";
	exit;
fi

if [ ! -d "$RUN_DIR" ]
then
	echo "You must specify a polyworld run/ directory."
	echo "Usage: ./plotAdamiComplexity.sh <run_directory> [124]"
	echo "Where [124] specifies the width of the bit-window you want to plot.  If no options are specified the default is to plot 1,2, and 4 bits."
	echo "Ex: ./plotAdamiComplexity <run_directory> 12		-- plots AdamiComplexity with 1 and 2 bit windows"
	echo "Ex: ./plotAdamiComplexity <run_directory> 14		-- plots AdamiComplexity with 1 and 4 bit windows"
	exit;
fi

# Does the AdamiComplexity summary file exist?
if [ -f "$RUN_DIR/$FILENAME" ]
then
	echo "- Plotting '$RUN_DIR/$FILENAME'..."
else
	echo "* Error: Didn't find file '$RUN_DIR/$FILENAME'"
	exit;
fi

if [ -z "$TO_PLOT" ]		# if no options specified, plot them all.
then
	FLAG_plotOnebit=1
	FLAG_plotTwobit=1
	FLAG_plotFourbit=1
else
	temp=$(echo "$TO_PLOT" | grep -o '1')
	if [ -n "$temp" ]
	then
		FLAG_plotOnebit=1
	fi
	temp=$(echo "$TO_PLOT" | grep -o '2')
	if [ -n "$temp" ]
	then
		FLAG_plotTwobit=1
	fi
	temp=$(echo "$TO_PLOT" | grep -o '4')
	if [ -n "$temp" ]
	then
		FLAG_plotFourbit=1
	fi
fi

echo "- Plotting 1-bit = $FLAG_plotOnebit"
echo "- Plotting 2-bit = $FLAG_plotTwobit"
echo "- Plotting 4-bit = $FLAG_plotFourbit"


#################### Finished with our parameters, now lets plot!

GNUPLOT_plotstring="plot"

GNUPLOT_filename="$RUN_DIR/$FILENAME"
GNUPLOT_title="${GNUPLOT_filename}\n"
if [ "$FLAG_plotOnebit" = 1 ]		# Plotting Onebit?
then
	GNUPLOT_plotstring="$GNUPLOT_plotstring '$GNUPLOT_filename' using 1:2 w l lw 2 lt 1 title '1 bit',"
	GNUPLOT_title="${GNUPLOT_title}1bit,"
fi
 
if [ "$FLAG_plotTwobit" = 1 ]          # Plotting Twobit?
then
	GNUPLOT_plotstring="$GNUPLOT_plotstring '$GNUPLOT_filename' using 1:3 w l lw 2 lt 2 title '2 bit',"
	GNUPLOT_title="${GNUPLOT_title}2bit,"
fi
 
if [ "$FLAG_plotFourbit" = 1 ]          # Plotting Fourbit?
then
	GNUPLOT_plotstring="$GNUPLOT_plotstring '$GNUPLOT_filename' using 1:4 w l lw 2 lt 3 title '4 bit',"
	GNUPLOT_title="${GNUPLOT_title}4bit,"
fi

	# take off the final comas
GNUPLOT_plotstring=$(echo "$GNUPLOT_plotstring" | sed -e 's/,$//')
GNUPLOT_title=$(echo "$GNUPLOT_title" | sed -e 's/,$//')
#	echo "plotstring = '$GNUPLOT_plotstring'"
#	echo "title = '$GNUPLOT_title'"

	# Okay, lets plot!
${GNUPLOT} << EOF
set xlabel 'Timestep' ${GNUPLOT_labelparams}
set ylabel '${GNUPLOT_ylabel}' ${GNUPLOT_labelparams}
set grid
set mxtics 5
set title "${GNUPLOT_title}" ${GNUPLOT_titleparams}
${GNUPLOT_plotstring}
EOF

echo "Done!"
