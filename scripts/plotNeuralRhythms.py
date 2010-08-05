#!/usr/bin/env python
# plotNeuralRhythms
# Read file

import os
import pylab
import matplotlib
from matplotlib.collections import LineCollection
from matplotlib.colors import colorConverter
import getopt
import sys

ALT_COLOR_MAX = 0.75

def choose_file():
#	direc=os.listdir(os.getcwd())
#	directory=[]
#	print "The available neural rhythm files are:"
#	for text in direc:
#		if ".txt" in text:
#			directory.append(text)
#	for line in directory:
#		print line
	return raw_input("Please type the name of the brainFunction file you would like to plot: ")

def open_file(file_name, mode):
	"""Open a file."""
	try:			
		the_file = open(file_name, mode)
	except(IOError), e:
		print "Unable to open the file", file_name, "Ending program.\n", e
		raw_input("\n\nPress the enter key to exit.")
		sys.exit()
	except(NameError):
		print "Unable to open the file", file_name, "because it does not exist.	 \nEnding program.\n"
	else:
		return the_file

def next_line(the_file):
	"""Return next line from the brainFunction file, formatted."""
	line = the_file.readline()
	line = line.replace("/", "\n")
	return line

def interpret(line):
	 """Returns interpreted first line"""
	 args = []
	 args1 = line.split()
	 for string in args1:
		for item in string.split('-'):
			args.append(item)
	 return args


def plotter(choice, numNeurons, numTimeSteps, maxNeurons, maxTimeSteps, reverse, numInputNeurons, colorNeurons, redStart, redEnd, greenStart, greenEnd, blueStart, blueEnd, eatNeuron, mateNeuron, fightNeuron, moveNeuron, yawNeuron, label, behaviorLabels, inputLabels):
	rhythm_file = open_file(choice, "r")
	line = next_line(rhythm_file)
	line = next_line(rhythm_file)
	figwidth = 12.0
	figheight = 8.0

	fig = pylab.figure(figsize=(figwidth,figheight))

	ax = fig.add_subplot(111)
	ax.set_xlim(0.5, maxTimeSteps+0.5)
	ax.set_ylim(maxNeurons-0.5, -0.5)
	pylab.title(label)
	pylab.ylabel('Neuron Index')
	pylab.xlabel('Time Step')

	linewidth = 0.715 * fig.get_figwidth() * fig.get_dpi() / maxTimeSteps

	for time in range(numTimeSteps):
		x = []
		y = []
		colors = []
		for neuron in range(numNeurons):
			activ = line.split()
			x.append(time+1)			  
			y.append(neuron-0.5)
			activation = float(activ[1])
			if reverse:
				activation = 1.0 - activation
			if colorNeurons:
				if neuron in range(redStart, redEnd+1):
					colors.append((activation, activation*ALT_COLOR_MAX, activation*ALT_COLOR_MAX, 1.0))
				elif neuron in range(greenStart, greenEnd+1):
					colors.append((activation*ALT_COLOR_MAX, activation, activation*ALT_COLOR_MAX, 1.0))
				elif neuron in range(blueStart, blueEnd+1):
					colors.append((activation*ALT_COLOR_MAX, activation*ALT_COLOR_MAX, activation, 1.0))
				elif neuron == eatNeuron:
					colors.append((activation*ALT_COLOR_MAX, activation, activation*ALT_COLOR_MAX, 1.0))
				elif neuron == mateNeuron:
					colors.append((activation*ALT_COLOR_MAX, activation*ALT_COLOR_MAX, activation, 1.0))
				elif neuron == fightNeuron:
					colors.append((activation, activation*ALT_COLOR_MAX, activation*ALT_COLOR_MAX, 1.0))
				elif neuron == yawNeuron:
					colors.append((activation, activation, activation*ALT_COLOR_MAX, 1.0))
				else:
					colors.append((activation, activation, activation, 1.0))
			else:
				colors.append((activation, activation, activation, 1.0))
			line = next_line(rhythm_file)
		x.append(time+1)
		y.append(neuron+0.5)
		colors.append((activation, activation, activation, 1.0))
		points = zip(x, y)
		segments = zip(points[:-1], points[1:])
		lc = LineCollection(segments, colors=colors)
		lc.set_alpha(1.0)
		lc.set_linewidth(linewidth)
		lc.set_antialiased(False)
		ax.add_collection(lc)

	rhythm_file.close()

	if behaviorLabels:
		matplotlib.pyplot.text(maxTimeSteps+1.5, eatNeuron, "Eat", weight="ultralight", size="small", va="center")
		matplotlib.pyplot.text(maxTimeSteps+1.5, mateNeuron, "Mate", weight="ultralight", size="small", va="center")
		matplotlib.pyplot.text(maxTimeSteps+1.5, fightNeuron, "Fight", weight="ultralight", size="small", va="center")
		matplotlib.pyplot.text(maxTimeSteps+1.5, moveNeuron, "Move", weight="ultralight", size="small", va="center")
		matplotlib.pyplot.text(maxTimeSteps+1.5, yawNeuron, "Turn", weight="ultralight", size="small", va="center")
		matplotlib.pyplot.text(maxTimeSteps+1.5, yawNeuron+1, "Light", weight="ultralight", size="small", va="center")
		matplotlib.pyplot.text(maxTimeSteps+1.5, yawNeuron+2, "Focus", weight="ultralight", size="small", va="center")
	
	if inputLabels:
		matplotlib.pyplot.text(maxTimeSteps+1.5, 0, "Random", weight="ultralight", size="small", va="center")
		matplotlib.pyplot.text(maxTimeSteps+1.5, 1, "Health", weight="ultralight", size="small", va="center")
		for neuron in range(redStart, redEnd+1):
			matplotlib.pyplot.text(maxTimeSteps+1.5, neuron, "R", weight="ultralight", size="small", va="center")
		for neuron in range(greenStart, greenEnd+1):
			matplotlib.pyplot.text(maxTimeSteps+1.5, neuron, "G", weight="ultralight", size="small", va="center")
		for neuron in range(blueStart, blueEnd+1):
			matplotlib.pyplot.text(maxTimeSteps+1.5, neuron, "B", weight="ultralight", size="small", va="center")


def usage():
	print "Usage: plotNeuralRhythms [-i -g -f -b -n numNeurons -t numTimeSteps -l label] fileName"
	print "    -r will reverse brightness so 1.0 is black (normally 1.0 is white)"
	print "    -g will inhibit drawing of the graph (normally the graph is drawn)"
	print "    -f will save the plot in a file (normally a file is not saved)"
	print "    -b will inhibit behavior labels (normally they are drawn)"
	print "    -c will inhibit the use of colors to identify neurons (normally color is used)"
	print "    -i will toggle input labels (off by default, they are turned on if color is inhibited)"
	print "    -n numNeurons manually determines the scale of the y axis (normally it is automatic)"
	print "    -t numTimeSteps manually determines the scale of the x axis (normally it is automatic)"
	print "    -l label allows specification of a plot label"
	print "    fileName is the name of the brain function file to plot"
 
def main():

	reverse = False
	saveGraph = False
	showGraph = True
	maxNeurons = 0
	maxTimeSteps = 0
	label = ""
	behaviorLabels = True
	inputLabels = False
	toggleInputLabels = False
	colorNeurons = True

	try:
		opts, args = getopt.getopt(sys.argv[1:], "rn:t:fgl:bci", \
					 ["reverse", "numNeurons", "numTimeSteps", "savegraph", "nograph", "label", "behaviorLabels", "colorNeurons", "inputLabels"] )
	except getopt.GetoptError, err:
		# print help information and exit:
		print str(err) # will print something like "option -a not recognized"
		usage()
		sys.exit(2)

	if len(args) == 1:
		file_name = args[0]
	elif len(args) > 1:
		print "Error - unrecognized arguments: ",
		for a in args[1:]:
			print a, "",
		print
		usage()
		exit(2)
	else:
		usage()
		exit(0)
		#file_name = choose_file()
	
	for o, a in opts:
		if o in ("-r", "--reverse"):
			reverse = True
		elif o in ("-n", "--numNeurons"):
			maxNeurons = int(a)
		elif o in ("-t", "--numTimeSteps"):
			maxTimeSteps = int(a)
		elif o in ("-g", "--nograph"):
			showGraph = False
		elif o in ("-f", "--savegraph"):
			saveGraph = True
		elif o in ("-l", "--label"):
			label = a
		elif o in ("-b", "--behaviorLabels"):
			behaviorLabels = False
		elif o in ("-c", "--colorNeurons"):
			colorNeurons = False
			inputLabels = True
		elif o in ("-i", "--inputLabels"):
			toggleInputLabels = True
		else:
			assert False, "unhandled option"
	
	if toggleInputLabels:
		inputLabels = not inputLabels
	
	rhythm_file = open_file(file_name, "r")
	line = next_line(rhythm_file)
	line1_args = interpret(line)
	if label == "":
		label = "Brain Function "+ line1_args[1]
	numNeurons = int(line1_args[2])
	if maxNeurons == 0:
		maxNeurons = numNeurons
	numInputNeurons = int(line1_args[3])
	redStart = int(line1_args[6])
	redEnd = int(line1_args[7])
	greenStart = int(line1_args[8])
	greenEnd = int(line1_args[9])
	blueStart = int(line1_args[10])
	blueEnd = int(line1_args[11])
	eatNeuron = numInputNeurons
	mateNeuron = eatNeuron + 1
	fightNeuron = mateNeuron + 1
	moveNeuron = fightNeuron + 1
	yawNeuron = moveNeuron + 1
	noOfLines = 0
	while next_line(rhythm_file) != "":
		noOfLines+=1
	numTimeSteps = (noOfLines-2)/numNeurons # -2 for top brainFunction line and bottom end fitness line
	rhythm_file.close()
	if maxTimeSteps == 0:
		maxTimeSteps = numTimeSteps

	plotter(file_name, numNeurons, numTimeSteps, maxNeurons, maxTimeSteps, reverse, numInputNeurons, colorNeurons, redStart, redEnd, greenStart, greenEnd, blueStart, blueEnd, eatNeuron, mateNeuron, fightNeuron, moveNeuron, yawNeuron, label, behaviorLabels, inputLabels)
	
	if saveGraph == True:
		pylab.savefig(label+".png", dpi=72)
	if showGraph == True:
		pylab.show()

try:
	main()
except KeyboardInterrupt:
	print "\nExiting."


