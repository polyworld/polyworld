#!/bin/sh
############################################################
# This script takes 1 or 2 arguments.  If no second argument is specified, the default is to use the input filename and appending '.nwb'
# 1) The filename of the polyworld brainAnatomy file
# 2) The filename of the desired NetworkWorkbench file
############################################################

BAfile="$1"
NWBfile="$2"


if [ ! -f "$BAfile" ]
then
	echo "This script takes 1 or 2 arguments.
	1) The filename of the polyworld brainAnatomy file
	2) The output filename after converting to NetworkWorkbench.  If no 2nd argument is specified, the default is to use the input filename and append '.nwb'"
        exit;
fi

# If no second argument is specified
if [ -z "$NWBfile" ]
then
	NWBfile="${BAfile}.nwb"
fi

firstline=$(head -n 1 "$BAfile")
FL_critterid=$(echo "$firstline" | cut -d' ' -f2)
FL_fitness=$(echo "$firstline" | cut -d' ' -f3 | cut -d'=' -f2)
FL_numneurons=$(echo "$firstline" | cut -d' ' -f4 | cut -d'=' -f2)

# This is a little nuts.  But here's the idea.  1) cat file. 2) space->newline. 3) strip lines not beginning with +/-. 4) remove zeros. 5) get count. 6) strip whitespace from result.
numedges=$(cat ${BAfile} | tr ' ' '\n' | grep '^[\+\-]' | grep -v '^[\+\-]0.0000[0]*$' | wc -l | sed -e 's/[^0-9]//g')

if [ "$numedges" -le 0 ]
then
	echo "Error: I found '$numedges' edges!  There is probably something terribly wrong.  Exiting."
	exit;
fi

echo "// firstline = $firstline" > $NWBfile
# echo "// id = $FL_critterid" >> $NWBfile
# echo "// fitness = $FL_fitness" >> $NWBfile
# echo "// numneu = $FL_numneurons" >> $NWBfile
# echo "// numedges = $numedges" >> $NWBfile

echo "*Nodes $FL_numneurons" >> $NWBfile
echo "*DirectedEdges $numedges" >> $NWBfile

grep '^[\+\-]' ${BAfile} | awk -F' ' '
{
	arraysize = split( $0, EDGES, " ");
	for( FROMneuron=1; FROMneuron < arraysize; FROMneuron++ )
	{
#		print "Cxn " FROMneuron ": " EDGES[FROMneuron]
		if( EDGES[FROMneuron] != 0.0 ) 
		{
			# Inhibitory or Excitatory cxn?
			if( EDGES[FROMneuron] > 0.0 ) { cxntype="E" } else { cxntype="I" }

			# remove the plus or minus 
			EDGES[FROMneuron] = substr( EDGES[FROMneuron], 2, length( EDGES[FROMneuron] ) - 1);

#			print "To: " NR "_____From: " FROMneuron "________" EDGES[FROMneuron];
			print FROMneuron " " NR " " EDGES[FROMneuron] " " cxntype 
		}
	}


}' >> $NWBfile

