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
FILENAME="AvrComplexity.plt"
GNUPLOT_ylabel="Average Neural Complexity"
GNUPLOT_labelparams='font "Times,20"'
GNUPLOT_titleparams='font "Times,20"'
# Don't modify anything beneath here
########################################################################################################

FLAG_plotInp=0
FLAG_plotPro=0
FLAG_plotAll=0

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
        echo "Usage: ./plotNeuralComplexity.sh <run_directory> [API]"
        echo "Where [API] specifies which groups of neurons (All, Processing, Input) you want to plot.  If no options are specified the default is to plot them all."
        echo "Ex: ./plotNeuralComplexity <run_directory> AP              -- plots NeuralComplexity of All neurons and Processing neurons"
        echo "Ex: ./plotNeuralComplexity <run_directory> PI              -- plots NeuralComplexity of Processing neurons and Input neurons"
	exit;
fi

if [ -z "$TO_PLOT" ]		# if no options specified, plot them all.
then
	FLAG_plotInp=1
	FLAG_plotPro=1
	FLAG_plotAll=1
else
	temp=$(echo "$TO_PLOT" | grep -o -i '[Ii]')
	if [ -n "$temp" ]
	then
		FLAG_plotInp=1
	fi
	temp=$(echo "$TO_PLOT" | grep -o -i '[Pp]')
	if [ -n "$temp" ]
	then
		FLAG_plotPro=1
	fi
	temp=$(echo "$TO_PLOT" | grep -o -i '[Aa]')
	if [ -n "$temp" ]
	then
		FLAG_plotAll=1
	fi
fi

echo "- Plotting Input = $FLAG_plotInp"
echo "- Plotting Processing = $FLAG_plotPro"
echo "- Plotting All = $FLAG_plotAll"


# Does the bestRecent complexity plot exist?
PLOT_BESTRECENT=0
if [ -f "$RUN_DIR/brain/bestRecent/$FILENAME" ]
then
	echo "- Plotting bestRecent..."
	PLOT_BESTRECENT=1
else
	echo "* Didn't find file '$RUN_DIR/brain/bestRecent/$FILENAME'"
fi

# Does the Recent complexity plot exist?
PLOT_RECENT=0
if [ -f "$RUN_DIR/brain/Recent/$FILENAME" ]
then
	echo "- Plotting Recent..."
	PLOT_RECENT=1
else
	echo "* Didn't find file '$RUN_DIR/brain/Recent/$FILENAME'"
fi


#################### Finished with our parameters, now lets plot!

# Plotting BestRecent
GNUPLOT_plotstring="plot"

if [ "$PLOT_BESTRECENT" ]
then
	GNUPLOT_filename="$RUN_DIR/brain/bestRecent/$FILENAME"
	GNUPLOT_title="${GNUPLOT_filename}\n"
	if [ "$FLAG_plotAll" = 1 ]		# Plotting All?
	then
		GNUPLOT_plotstring="$GNUPLOT_plotstring '$GNUPLOT_filename' using 1:2 w l lw 2 lt 1 title 'All', '$GNUPLOT_filename' using 1:2:3 w yerrorbars lt 1 notitle,"
		GNUPLOT_title="${GNUPLOT_title}All,"
	fi
 
        if [ "$FLAG_plotPro" = 1 ]          # Plotting Pro?
        then
                GNUPLOT_plotstring="$GNUPLOT_plotstring '$GNUPLOT_filename' using 1:4 w l lw 2 lt 2 title 'Processing', '$GNUPLOT_filename' using 1:4:5 w yerrorbars lt 2 notitle,"
                GNUPLOT_title="${GNUPLOT_title}Processing,"
        fi
 
        if [ "$FLAG_plotInp" = 1 ]          # Plotting Inp?
        then
                GNUPLOT_plotstring="$GNUPLOT_plotstring '$GNUPLOT_filename' using 1:6 w l lw 2 lt 3 title 'Input', '$GNUPLOT_filename' using 1:6:7 w yerrorbars lt 3 notitle,"
                GNUPLOT_title="${GNUPLOT_title}Input,"
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
set key outside
set bar 0
set mxtics 5
set title "${GNUPLOT_title}" ${GNUPLOT_titleparams}
#set title "Neural Complexity over time - entire population"
${GNUPLOT_plotstring}
EOF
fi


# Plotting BestRecent
GNUPLOT_plotstring="plot"

if [ "$PLOT_RECENT" ]
then
	GNUPLOT_filename="$RUN_DIR/brain/Recent/$FILENAME"
	GNUPLOT_title="${GNUPLOT_filename}\n"
	if [ "$FLAG_plotAll" = 1 ]		# Plotting All?
	then
		GNUPLOT_plotstring="$GNUPLOT_plotstring '$GNUPLOT_filename' using 1:2 w l lw 2 lt 1 title 'All', '$GNUPLOT_filename' using 1:2:3 w yerrorbars lt 1 notitle,"
		GNUPLOT_title="${GNUPLOT_title}All,"
	fi
 
        if [ "$FLAG_plotPro" = 1 ]          # Plotting Pro?
        then
                GNUPLOT_plotstring="$GNUPLOT_plotstring '$GNUPLOT_filename' using 1:4 w l lw 2 lt 2 title 'Processing', '$GNUPLOT_filename' using 1:4:5 w yerrorbars lt 2 notitle,"
                GNUPLOT_title="${GNUPLOT_title}Processing,"
        fi
 
        if [ "$FLAG_plotInp" = 1 ]          # Plotting Inp?
        then
                GNUPLOT_plotstring="$GNUPLOT_plotstring '$GNUPLOT_filename' using 1:6 w l lw 2 lt 3 title 'Input', '$GNUPLOT_filename' using 1:6:7 w yerrorbars lt 3 notitle,"
                GNUPLOT_title="${GNUPLOT_title}Input,"
        fi

	# take off the final comas
	GNUPLOT_plotstring=$(echo "$GNUPLOT_plotstring" | sed -e 's/,$//')
	GNUPLOT_title=$(echo "$GNUPLOT_title" | sed -e 's/,$//')

	# Okay, lets plot!
${GNUPLOT} << EOF
set xlabel 'Timestep' ${GNUPLOT_labelparams}
set ylabel '${GNUPLOT_ylabel}' ${GNUPLOT_labelparams}
set grid
set key outside
set bar 0
set mxtics 5
#set title "${GNUPLOT_title}" ${GNUPLOT_titleparams}
set title "Neural Complexity over time - entire population" ${GNUPLOT_titleparams}
${GNUPLOT_plotstring}
EOF
fi







#echo "- Deleted ,temp file."
#rm ,temp

echo "Done!"
