#!/bin/sh
############################################################
# This script takes 1 or 2 arguments, a BirthsDeaths logfile is taken as input.  The 2nd argument specifies the output filename.
############################################################
# CONFIGURABLE PARAMETERS
############################################################
DOT="/usr/local/graphviz-2.12/bin/dot"
SCCMAP="/usr/local/graphviz-2.12/bin/sccmap"

#DOT_FILLCOLORS="whitesmoke deeppink1 firebrick1 hotpink1 indianred1 lightsalmon maroon orangered1 palevioletred1 pink1 salmon1 red1 forestgreen"
DOT_FILLCOLORS="whitesmoke deeppink1 orange cyan1 forestgreen"
PLOT_FORMAT="pdf"	# pdf, png, etc.
DOTFILE_PARAMS="
	nodesep=0.2;				// minimum node separation (in inches).
//	height=0.7;
//	edge [color=Blue, style=dashed, arrowhead=normal];
	concentrate=true;			// handy!  Collapse 2 edges into a single edge.
	ranksep=equally;			// ranks should be equally distant from one another
	center=true;				// center the graph on the page
	node [color=Red,fontname=Helvetica,shape=invtrapezium];		//shape=point seems like a good idea, but it just made for edgehell.
	edge [color=Blue, style=dashed,arrowhead=normal];		//sametail seems like it would be nice, but it puts arrows going inside the trapezoids -- lame.
//	node [color=Red,fontname=Helvetica,shape=point];		//shape=point seems like a good idea, but it just made for edgehell.
//	edge [color=Blue, style=dashed,arrowhead=none];		//sametail seems like it would be nice, but it puts arrows going inside the trapezoids -- lame.


//	a[shape=polygon,sides=5,peripheries=3,color=lightblue,style=filled];
	rankdir=TB;			// Print from top to bottom (the default)
	"
# DON'T EDIT ANYTHING PAST THIS UNLESS YOU KNOW WHAT YOU'RE DOING.
############################################################
############################################################
BDfile=$(echo "${1}" | sed -e 's/\/$//')       # removing any trailing slash
PLOTfile="$2"

if [ ! -x "$DOT" ]
then
	echo "Could not execute file '$DOT'.  It's either unexecutable or nonexistant.  Fix it."
	exit;
fi

if [ -d "$BDfile" ]	# user probably specified a run/ directory instead of the BirthsDeaths.log.  That's okay though, it'll be accounted for.
then
	if [ -f "$BDfile/BirthsDeaths.log" ]
	then
		# update the BDfile to include the BirthsDeaths.log
		BDfile="${BDfile}/BirthsDeaths.log"
	else
		# nope, user didn't specify a run/ directory.  Exit.
		echo "* Do not recognize '$BDfile' as a BirthDeaths.log or a run/ directory.  Exiting."
		exit;
	fi
	
fi

if [ ! -f "$BDfile" ]
then
	echo "This script takes 1 or 2 arguments.
	1) The filename of the polyworld BirthsDeaths.log file
	2) The output filename of the pedigree plot.  If no 2nd argument is specified, the default takes the input filename and appends '.${PLOT_FORMAT}'"
        exit;
fi

#######################

childcolorcap=$(echo "${DOT_FILLCOLORS}" | tr ' ' '\n' | wc -l | tr -d ' 	' | awk '{ print $0 - 1 }')
echo "- Distinguishing from 0...${childcolorcap} children.";

#######################


# If no second argument is specified
if [ -z "$PLOTfile" ]
then
	PLOTfile="${BDfile}.${PLOT_FORMAT}"
fi

#firstline looks at the first hundred lines of the BirthsDeaths.log.  It removes all lines beginning with '#' and '%' and then gets the first line (the first line of data)
firstline=$(head -n 100 "$BDfile" | grep -v '^[%#]' | head -n 1)
FL_firstchild=$(echo "$firstline" | cut -d' ' -f3)
numseedcritters=$(echo "$FL_firstchild - 1" | bc)

echo "- Num Seed Critters = $numseedcritters"


#past -> 1978 -> 1980 -> 1982 -> 1983 -> 1985 -> 1986 -> 
# 1987 -> 1988 -> 1989 -> 1990 -> "future"; 



# Only look at lines that are BIRTH events, and then only get columns 3, 4, and 5.
{
echo "digraph hierarchy {"
echo "${DOTFILE_PARAMS}"
grep ' BIRTH ' ${BDfile} | cut -d' ' -f3,4,5 | awk -v DOT_FILLCOLORS="${DOT_FILLCOLORS}" -v firstchild="${FL_firstchild}" -F' ' '
BEGIN { 
	DEPTHS[0]="";		# (index=0...MAXDEPTHS) -- which critters are at each depth
	CRITTER[0]="";		# (index=critternum) -- depth of a particular critter
	NUMCHILDREN[0]="";	# (index=critternum) -- number of children a particular critter had
	MAXDEPTH=0;		# Holds the highest critterdepth that we encounter.
	MAXCRITTERNUM=0;	# Holds the highest critternum that we encounter.
	for( i=1; i<firstchild; i++ ) { DEPTHS[0] = DEPTHS[0] " " i; CRITTER[i]=0; }
	}

# Main program
{
	critternum=$1;

	# critterdepth is the average of the depth of the parents, rounded, plus one.
	critterdepth= int( ((CRITTER[$2] + CRITTER[$3]) / 2) + 0.5) + 1;

	# update MAXCRITTERNUM to the new highest critternum
	if( critternum > MAXCRITTERNUM ) {
		MAXCRITTERNUM=critternum;
	}

	# update MAXDEPTH to the new max, make the first node (for the timeline)
	if( critterdepth > MAXDEPTH ) {
		MAXDEPTH=critterdepth;
	}

	NUMCHILDREN[$2]++;
	NUMCHILDREN[$3]++;

	DEPTHS[critterdepth]=DEPTHS[critterdepth] " " critternum;
	CRITTER[critternum]=critterdepth

#	print "critterdepth= " critterdepth
	print "{ " $2 " " $3 " }->" critternum

}

END {
	# Now print our depth information
	print "{rank=source;" DEPTHS[0] " }";	# print the SEEDs, they have a special rank.
	for( i=1; i<MAXDEPTH; i++ ) {
#		print "DEPTHS[" i "]: " DEPTHS[i];
		print "{rank=same;" DEPTHS[i] " }"
		}
	# The final children also have a special rank.
	print "{rank=max;" DEPTHS[MAXDEPTH] " }";	# print the SEEDs, they have a special rank.  These could also be "rank=sink", but rank=max is more robustly aesthetic.

	####################################################################################
	####################################################################################
	####################################################################################

	# Get out ColorsArray in order so we can colorize agents based on how many children they had.
	numcolors = split( DOT_FILLCOLORS, ColorsArray, " ");

	for( i=0; i<=MAXCRITTERNUM; i++ ) {
		numchildren=NUMCHILDREN[i];
	
		# if the critter has more children than there are colors, set this critter to the highest color
		if( numchildren == "" ) { numchildren = 0; }
		colorindex=numchildren + 1;
		if( colorindex > numcolors ) { colorindex = numcolors; }

		print i "[ color=\"" ColorsArray[colorindex] "\", style=filled, label=" numchildren "]; // numchildren=" numchildren;
		print i "[ color=\"" ColorsArray[colorindex] "\", style=filled ]; // numchildren=" numchildren;
#		124[ color=\"firebrick1\", style=filled]
#		print "# CRITTER_" i "	" NUMCHILDREN[i];
#		label="makea\nstring"
	}
}'
echo "}"
} > ',temp'
echo "- Made temp .dot file ',temp'"
echo "- Generating '${PLOTfile}'..."
${DOT} ',temp' -T${PLOT_FORMAT} -o ${PLOTfile}
echo "- Made ${PLOTfile}.  Bringing up the plot.."
open ${PLOTfile}
################################################################
#echo "- Making the SCCMapp'ed .dot file..."
#${SCCMAP} -d ',temp' > ',sccmap'
#echo "Plotting the ,sccmap..."
#${DOT} ',sccmap' -T${PLOT_FORMAT} -o sccmap.pdf
#echo "- Made sccmap.pdf.  Bringing up the plot.."
#open sccmap.pdf


# rm ,temp
# echo "- Deleted ',temp'"
echo "Done!"
