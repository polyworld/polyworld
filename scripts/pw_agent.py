#!/usr/bin/env python
import os.path
import pw_brainAnatomy, pw_brainFunction
from numpy import sum

class pw_agent:
	'''contains methods and data of polyworld agent anatomy and brainFunction files'''
	
	
	def __init__(self, filename1, filename2=None ):
		''' you may pass in filename1 and filename2 in any order'''

		self.func_filename, self.func = None, None
		self.anat_filename, self.anat = None, None

		assert os.path.isfile( filename1 ), "filename1 wasn't a file"
		assert filename2 is None or os.path.isfile( filename2), "filename2 was specified but wasn't a valid file"
		
		
		f1 = open(filename1)
		header1 = f1.readline()
		if header1.startswith('brainFunction'):
			self.func_filename = filename1
		elif header1.startswith('brain'):
			self.anat_filename = filename1
		else:
			raise ValueError, "filename1 was not a polyworld file"
		
		f1.close()
		
		if filename2:
			f2 = open(filename2)
			header2 = f2.readline()
			if header2.startswith('brainFunction'):
				assert self.func_filename is None, "func_filename already defined!"
				self.func_filename = filename2
			elif header2.startswith('brain'):
				assert self.anat_filename is None, "anat_filename already defined!"				
				self.anat_filename = filename2
			else:
				raise ValueError, "filename2 was not a polyworld file"

			f2.close()
		else:	# only one filename provided
			if self.func_filename:	# func filename provided, so generate anat filename
				self.anat_filename = self.func_filename.replace("/function/","/anatomy/").replace("brainFunction","brainAnatomy").replace(".txt","_death.txt")
				if not os.path.isfile( self.anat_filename ):
					self.anat_filename = None
			elif self.anat_filename:	# anat filename provided, so generate func filename
				bdi = self.anat_filename.rfind("_")	# index of start of _birth/death/incept
				self.func_filename = self.anat_filename[:bdi].replace("/anatomy/","/function/").replace("brainAnatomy","brainFunction") + ".txt"
				if not os.path.isfile( self.func_filename ):
					self.func_filename = None
		
		if self.anat_filename:
			#print "anat_filename=", self.anat_filename
			self.anat = pw_brainAnatomy.pw_brainAnatomy( self.anat_filename )
		
		if self.func_filename:
			#print "func_filename=", self.func_filename
			self.func = pw_brainFunction.pw_brainFunction( self.func_filename )
			
		# sanity chceks to ensure the anatomy and function are about the same agent
		if self.anat and self.func:
			assert self.anat.agent_index == self.func.agent_index, "agent indices didn't match"
			assert self.anat.num_neurons == self.func.num_neurons, "number of neurons didn't match"
			
			# Now do sanity check that the bias node is connected to every processing node...
			for node in self.func.neurons['processing']:
				assert self.func.neurons['bias'] in self.anat.trace_back(node, 1) , "* Error! a processing neuron wasn't connected to the bias node!"

	def context_network( self, behavior_node, numsteps ):
		'''returns all nodes connected within numsteps of a given behavior node'''

		assert behavior_node in self.func.neurons['behavior'], "passed node wasn't in behavior nodes!"

		context_nodes = self.anat.trace_back( behavior_node, numsteps )
		print "CN nodes=%s" % context_nodes
		
		context_network = []
		for n in context_nodes:
			this_cn = [ (i,n) for i in context_nodes if self.anat.cxnmatrix[n][i] ]
#			print "Found connections", this_cn 
			context_network.extend( this_cn )

		# return all of the unique pairs of the context_network
		return sorted(list(set(context_network)))
	
	def reference_time( self, behavior_node, window_size, start_at ):
		'''returns the time start_at <= t <= numsteps lived that begins the most active time of length window_size
		breaking ties by going to the end.
		'''
		
		assert window_size >= 1, "window size must be >= 1"
		assert start_at >= 20, "You want start_at to be at least 20 for decent sampling"
		assert behavior_node in self.func.neurons['behavior'], "behavior_node wasn't in the behvior nodes!"
		
		if start_at + window_size > self.func.timesteps_lived:
			return None
		
#		print self.func.acts.shape
#		print self.func.acts[behavior_node,:]
#		print len(self.func.acts[behavior_node,:])
#		raw_input('...')
		
		sum_acts = {}
		for t in range( start_at, self.func.timesteps_lived - window_size ):
			this_window = self.func.acts[behavior_node,t:t+window_size]
			sum_acts[t] = sum(this_window)
		
#		print "sum_acts=%s" % sum_acts
		
		# which t's had the largest sum_acts ?
		most_active = max( sum_acts.values() )
		most_active_times = [ int(key) for key, value in sum_acts.iteritems() if value == most_active ]
		
		# return the latest most_active_time
		return max( most_active_times )
		

	
if __name__ == '__main__':
	c = pw_agent('brainAnatomy_62_incept.txt', 'brainFunction_84.txt' )
	
	
