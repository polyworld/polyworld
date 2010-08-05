#!/usr/bin/env python

# Calculates neuron and synapse count min, max, and average
# Invoke this from within a "Recent" or "bestRecent" or "bestSoFar" directory
# Prints to screen and writes to a comma-separated-values file, "neuronStats.csv"

import sys, getopt, os.path, re, os

c = ","
n = "\n"

csv = open("neuronStats.csv", "w")
csv.write("Time"+c+"numNeuronsMin"+c+"numNeuronsAvg"+c+"numNeuronsMax"
				+c+"numSynapsesMin"+c+"numSynapsesAvg"+c+"numSynapsesMax"+n)

numNeuronsMin = 10000
numNeuronsMax = 0
numNeuronsAvg = 0
numSynapsesMin = 10000
numSynapsesMax = 0
numSynapsesAvg = 0
numDirs = 0
for root, dirs, files in os.walk("."):
	dirnums = []
	for dir in dirs:
		dirnums.append(int(dir))
	dirnums.sort()
	dirs = []
	for num in dirnums:
		dirs.append(str(num))
	for dir in dirs:
		print "**** " + dir + " ****"
		numNeuronsDirMin = 10000
		numNeuronsDirMax = 0
		numNeuronsDirAvg = 0
		numSynapsesDirMin = 10000
		numSynapsesDirMax = 0
		numSynapsesDirAvg = 0
		numBrainFiles = 0
		for root2, dirs2, files2 in os.walk(dir):
			for name in files2:
				if name[0:13] == "brainFunction":
					numBrainFiles += 1
					file = open(dir+"/"+name, "r")
					line = file.readline()
					words = line.split()
					numNeurons = int(words[2])
					if numNeurons > numNeuronsDirMax:
						numNeuronsDirMax = numNeurons
					if numNeurons < numNeuronsDirMin:
						numNeuronsDirMin = numNeurons
					numNeuronsDirAvg += numNeurons
					numSynapses = int(words[4])
					if numSynapses > numSynapsesDirMax:
						numSynapsesDirMax = numSynapses
					if numSynapses < numSynapsesDirMin:
						numSynapsesDirMin = numSynapses
					numSynapsesDirAvg += numSynapses
			break
		numNeuronsDirAvg /= numBrainFiles
		numSynapsesDirAvg /= numBrainFiles
		print "    numNeuronsDir Min =", numNeuronsDirMin, "Avg =", numNeuronsDirAvg, "Max =", numNeuronsDirMax
		print "    numSynapsesDir Min =", numSynapsesDirMin, "Avg =", numSynapsesDirAvg, "Max =", numSynapsesDirMax
		csv.write(dir+c+str(numNeuronsDirMin)+c+str(numNeuronsDirAvg)+c+str(numNeuronsDirMax)
					 +c+str(numSynapsesDirMin)+c+str(numSynapsesDirAvg)+c+str(numSynapsesDirMax)+n)
		if numNeuronsDirMin < numNeuronsMin:
			numNeuronsMin = numNeuronsDirMin
		if numNeuronsDirMax > numNeuronsMax:
			numNeuronsMax = numNeuronsDirMax
		numNeuronsAvg += numNeuronsDirAvg
		if numSynapsesDirMin < numSynapsesMin:
			numSynapsesMin = numSynapsesDirMin
		if numSynapsesDirMax > numSynapsesMax:
			numSynapsesMax = numSynapsesDirMax
		numSynapsesAvg += numSynapsesDirAvg
		if numBrainFiles > 0:
			numDirs += 1
numNeuronsAvg /= numDirs
numSynapsesAvg /= numDirs
print "numNeurons Min =", numNeuronsMin, "Avg =", numNeuronsAvg, "Max =", numNeuronsMax
print "numSynapses Min =", numSynapsesMin, "Avg =", numSynapsesAvg, "Max =", numSynapsesMax
csv.write("Total"+c+str(numNeuronsMin)+c+str(numNeuronsAvg)+c+str(numNeuronsMax)
				 +c+str(numSynapsesMin)+c+str(numSynapsesAvg)+c+str(numSynapsesMax)+n)
