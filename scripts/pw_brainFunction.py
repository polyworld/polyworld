#!/usr/bin/env python
from numpy import array, matrix, exp, pi, sqrt, mean, round, random, ones
import numpy
#from pprint import pprint
#from copy import copy
#import numpy

class pw_brainFunction:
	'''holds the data of a polyworld brainFunction file'''

	def __init__(self, input_filename=''):

		self.agent_index = None
		self.num_inputneurons = None
		self.agent_fitness = None
		self.num_neurons = None
		self.num_synapses = None
		self.timesteps_lived = None
		self.timestep_born = None
		self.header = None
		self.acts = None
		self.neurons = {}

		if not input_filename:
			return None
		
		lines = [ x.strip() for x in open( input_filename ).readlines() ]

		# brainFunction 157 15 8 17 0 2-3 4-5 6-7
		# format: 
		self.header = lines.pop(0).split(' ')

		#print 'firstline=', firstline
		assert self.header[0] == 'brainFunction', "was not a brainFunction file"
		assert len(self.header) >= 9, "header line has less than 9 parts"

		assert lines[-1].startswith('end fitness'), "brainFunction file did not finish"

		self.agent_fitness = float(lines.pop().split('=')[-1].strip())
	
		self.num_inputneurons = int(self.header[3])
		self.agent_index = int(self.header[1])
		self.num_neurons = int(self.header[2])
		self.num_synapes = int(self.header[4])
		self.timestep_born = int(self.header[5])

		self.neurons = {}
		Rstart, Rend = [ int(x) for x in self.header[6].split('-') ]
		self.neurons['red'] = range(Rstart,Rend+1)

		Gstart, Gend = [ int(x) for x in self.header[7].split('-') ]
		self.neurons['green'] = range(Gstart,Gend+1)

		Bstart, Bend = [ int(x) for x in self.header[8].split('-') ]
		self.neurons['blue'] = range(Bstart,Bend+1)
	
		self.timesteps_lived = int(len(lines) / self.num_neurons)

		assert len(lines) % self.num_neurons == 0, "Error. number of timesteps lived not divisible by #neurons"
		
		self.acts = [ [] for i in range(self.num_neurons) ]

		for line in lines:
			parts = line.split(' ')
			assert len(parts) == 2, "Error, was not two entries on the line"
			neuron, act = parts
			self.acts[ int(neuron) ].append( float(act) )

		# convert to numpy arrays
		self.acts = array( self.acts )
	
		numrows, numcols = self.acts.shape
		assert numrows == self.num_neurons, "#rows != num_neurons"
		assert numcols == self.timesteps_lived, "#cols != timesteps lived"
	
		# sanity check the activations == they're all within [0,1] ?
		assert 0.0 <= self.acts.all() <= 1.0, "acts had values not within [0.0,1.0]"

		
		##############################################
		# define the neural groups
		##############################################
		
		ALL_NEURONS = range( self.num_neurons )
		INPUT_NEURONS = range( self.num_inputneurons )

		PROCESSING_NEURONS = list(set(ALL_NEURONS) - set(INPUT_NEURONS))
		BEHAVIOR_NEURONS = range( self.num_neurons-7, self.num_neurons )
		INTERNAL_NEURONS = list(set(PROCESSING_NEURONS) - set(BEHAVIOR_NEURONS))
		assert len(BEHAVIOR_NEURONS) == 7, "There wasn't 7 behavior neurons"

		self.neurons['input'] = INPUT_NEURONS
		self.neurons['behavior'] = BEHAVIOR_NEURONS
		self.neurons['processing'] = PROCESSING_NEURONS
		self.neurons['internal'] = INTERNAL_NEURONS

		########################################################
		# NOW WE ACCOUNT FOR THE BIAS NODE.  WE DO THIS BY:
		#   1) Adding a key 'bias' to the self.neurons dictionary
		#	2) numneurons += 1
		#	3) adding a ROW of all 1.0's to end of the activity matrix
		########################################################
		self.neurons['bias'] = max(self.neurons['behavior'])+1
		self.num_neurons += 1
#		print self.acts.shape
		# the bias node is always 1.0
		bias_acts = ones( (1,self.timesteps_lived) )
		self.acts = numpy.vstack( (self.acts, bias_acts) )

#		print self.acts.shape


	def binarize_neurons( self, neurons ):
		'''binarizes the list of neurons'''
		########################################################
		# Binarize the processing neurons?
		########################################################
		for i in sorted(neurons):
			assert 0 <= i < self.acts.shape[0], "i wasn't within range of the neurons"
			self.acts[i] = round( self.acts[i] )

	
	def print_statistics( self ):
		'''print statistics about self.acts'''
		numrows, numcols = self.acts.shape
		assert numrows == self.num_neurons, "#cols wasn't same as num_neurons"

		print "input:", self.neurons['input']
		if self.neurons['internal']:
			print "internal", self.neurons['internal']
		else:
			print "internal: None"
		print "behavior:", self.neurons['behavior']

		means, variances = self.acts.mean(axis=1), self.acts.var(axis=1)
		num_entries = [ len(self.acts[i,:]) for i in range(self.acts.shape[0]) ]

		print "neuron statistics:"
		print ''
		for i, (m, v, n) in enumerate(zip(means,variances,num_entries)):
			
			print "neuron=%s \t mean=%.4f \t var=%.4f \t samples=%s" % ( i, m, v, n )
		
		print "acts.shape=", self.acts.shape
			
	def write_to_Rfile(self, output_filename, labels=None ):		
		assert self.num_neurons == self.acts.shape[0], "num_neurons didn't match the acts matrix!"
		assert self.timesteps_lived == self.acts.shape[1], "timesteps_lived didn't match the acts matrix!"

		temp = self.acts.T
		numrows, numcols = temp.shape

		f = open( output_filename, 'w' )

		if labels:
			assert len(labels) == numcols, "not same number of labels and columns"
			labels_line = ','.join([ str(x) for x in labels])
			f.write( labels_line + '\n' )

		for t in temp:
			line = ','.join( [ '%.5f' % x for x in t.tolist() ] )
	#		print 'line=', line
			f.write( line + '\n' )

		f.close()
		print "wrote to file '%s'" % output_filename
	
