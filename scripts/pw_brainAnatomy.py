#!/usr/bin/env python
from pprint import pprint
from numpy import array, matrix, zeros
import os.path

class pw_brainAnatomy:
	
	def __init__( self, input_filename ):
		
		assert os.path.isfile( input_filename ), "passed input_filename was not a valid file"
		
		lines = [ x.strip('\n; ') for x in open(input_filename).readlines() ]

		self.header = lines.pop(0).split(' ')
		assert( self.header[0] == 'brain' )

		assert len(self.header) >= 9, "header should have at least 9 parts."

		self.max_bias = float([ x.split('=')[1] for x in self.header if x.startswith('maxBias') ][0])
		self.max_weight = float([ x.split('=')[1] for x in self.header if x.startswith('maxWeight') ][0])
		self.agent_index = int(self.header[1])
		self.num_neurons = int([ x.split('=')[1] for x in self.header if x.startswith('numneurons+1') ][0])

		# if the blueinput=6-7, then the input neurons are [0...7], thus there are 8 input neurons.
		self.num_inputneurons = int(max([ x.split('=')[1].split('-')[1] for x in self.header if 'input' in x ] )) + 1

		assert len(lines) == self.num_neurons, "#lines != num_neurons"

		# read in the cells in one glorious line
		cells = [ float(cell) for row in lines for cell in row.split(' ') ]

		assert len(cells) == self.num_neurons*self.num_neurons, "Did not find a square matrix in anatomy file"

		self.cxnmatrix = array(cells).reshape(self.num_neurons, self.num_neurons) # reshape(self.num_neurons, self.num_neurons)


		assert -self.max_weight <= self.cxnmatrix.all() <= self.max_weight, "anatomy matrix wasn't within [-max_weight,max_weight]"

#		print self.cxnmatrix
		
	
	def trace( self, start_nodes, max_distance, threshold=0.0 ):
		'''Returns all nodes within max_distance connections from start_nodes -- going fowards.
		Ignores all connections less than threshold'''		
		return self.k_distance_from( start_nodes, max_distance, threshold, backtrace=False )

	def trace_back( self, start_nodes, max_distance, threshold=0.0 ):
		'''Returns all nodes within max_distance connections from start_nodes -- going backwards.
		Ignores all connections less than threshold'''
		return self.k_distance_from( start_nodes, max_distance, threshold, backtrace=True )

	
	def k_distance_from( self, start_nodes, max_distance, threshold, backtrace ):
		'''ignore all connections less than threshold'''
		assert max_distance >= 1, "max_distance must be >= 1"
		
		# if we passed a single int, make a list of it.
		if type(start_nodes) is int:
			start_nodes = [ start_nodes ]

		assert 0 <= min(start_nodes) <= max(start_nodes) <= self.num_neurons, "start_nodes weren't within [0,num_neurons]"
		
		

#		print self.cxnmatrix
		
		m = matrix( abs(self.cxnmatrix) > threshold, dtype=int )

		# if going backwards, invert the matrix and then look forward
		if backtrace:
			m = m.T

		v = zeros( (m.shape[0],1), dtype=int ); v[start_nodes] = 1
		connected = []

		for i in range(max_distance):
#			print "depth %s START:\t %s" % (i+1, v.T )
			v = m*v
			indices = v.nonzero()[0].tolist()[0]

#			print "depth %s END:  \t %s -- adding indices=%s" % (i+1, v.T, indices)

			# append to our list of connected_nodes, uniqueify
			connected = list(set( connected+indices ))

		
#		print "returning: ", connected 
		return sorted(connected)
		
if __name__ == '__main__':
	f = pw_brainAnatomy('brainAnatomy_62_incept.txt')

	max_dist = 3
	start_nodes = [2,3]
	connected = f.trace( [2,3], 2 )
	connected_back = f.trace_back( [2,3], 2 )	

	print "nodes %(connected)s are connected within %(max_dist)s steps from nodes %(start_nodes)s" % locals()
	print "nodes %(connected_back)s are BACKconnected within %(max_dist)s steps from nodes %(start_nodes)s" % locals()
