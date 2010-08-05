#!/usr/bin/env python

# MotifLengths - Calculates and displays characteristic path length,
#				 normalized shortest path length, 1 - efficiency, and
#				 connectivity length
#				 for all 3-node and 4-node undirected graph motifs
#
# NOTE: Requires modified version of NetworkX that supports normalized
#		shortest path calculation

# Produces the following output:
#
# nodes = 3
# 0: links = 0, cpl =    inf, npl = 1.0000, 1-e = 1.0000, cl =    inf
# 1: links = 1, cpl = 1.0000, npl = 0.6667, 1-e = 0.6667, cl = 3.0000
# 2: links = 2, cpl = 1.3333, npl = 0.1667, 1-e = 0.1667, cl = 1.2000
# 3: links = 3, cpl = 1.0000, npl = 0.0000, 1-e = 0.0000, cl = 1.0000
# 
# nodes = 4
# 0: links = 0, cpl =    inf, npl = 1.0000, 1-e = 1.0000, cl =    inf
# 1: links = 1, cpl = 1.0000, npl = 0.8333, 1-e = 0.8333, cl = 6.0000
# 2: links = 2, cpl = 1.3333, npl = 0.5556, 1-e = 0.5833, cl = 2.4000
# 3: links = 3, cpl = 1.6667, npl = 0.2222, 1-e = 0.2778, cl = 1.3846
# 4: links = 3, cpl = 1.5000, npl = 0.1667, 1-e = 0.2500, cl = 1.3333
# 5: links = 4, cpl = 1.3333, npl = 0.1111, 1-e = 0.1667, cl = 1.2000
# 6: links = 5, cpl = 1.1667, npl = 0.0556, 1-e = 0.0833, cl = 1.0909
# 7: links = 6, cpl = 1.0000, npl = 0.0000, 1-e = 0.0000, cl = 1.0000

import networkx as nx
import networkx_extensions as nxe
import numpy
import sys

print '--------------'

d = 1.0

def print_stats(m):
	nodes = 0
	for i in range(len(m)):
		g = nx.from_numpy_matrix(m[i],nx.Graph())  # undirected
		if g.number_of_nodes() != nodes:
			nodes = g.number_of_nodes()
			print '\nnodes =', nodes

		cpl = nxe.characteristic_path_length(g, weighted=True)
		npl = nxe.normalized_path_length(g, weighted=True, w_max = 1.0/d)
		e = nxe.efficiency(g)
		cl = nxe.connectivity_length(g, weighted=True)
		
		print '%d: links = %d, cpl = %6.4f, npl = %6.4f, 1-e = %6.4f, cl = %6.4f' % (i, g.number_of_edges(), cpl, npl, 1-e, cl)

m = []

# . .
#  .
m.append(numpy.matrix([[0,0,0],
					   [0,0,0],
					   [0,0,0]]))

# ._.
#  .
m.append(numpy.matrix([[0,d,0],
					   [d,0,0],
					   [0,0,0]]))

# ._.
# \.
m.append(numpy.matrix([[0,d,d],
					   [d,0,0],
					   [d,0,0]]))

# ._.
# \./
m.append(numpy.matrix([[0,d,d],
					   [d,0,d],
					   [d,d,0]]))

print_stats(m)

m = []

# . .
# . .
m.append(numpy.matrix([[0,0,0,0],
					   [0,0,0,0],
					   [0,0,0,0],
					   [0,0,0,0]]))					   

# ._.
# . .
m.append(numpy.matrix([[0,d,0,0],
					   [d,0,0,0],
					   [0,0,0,0],
					   [0,0,0,0]]))					   

# ._.
# | .
m.append(numpy.matrix([[0,d,d,0],
					   [d,0,0,0],
					   [d,0,0,0],
					   [0,0,0,0]]))					   

# ._.
# |_.
m.append(numpy.matrix([[0,d,d,0],
					   [d,0,0,0],
					   [d,0,0,d],
					   [0,0,d,0]]))					   

# ._.
# |\.
m.append(numpy.matrix([[0,d,d,d],
					   [d,0,0,0],
					   [d,0,0,0],
					   [d,0,0,0]]))					   

# ._.
# |\|
m.append(numpy.matrix([[0,d,d,d],
					   [d,0,0,d],
					   [d,0,0,0],
					   [d,d,0,0]]))					   

# ._.
# |\|
#  -
m.append(numpy.matrix([[0,d,d,d],
					   [d,0,0,d],
					   [d,0,0,d],
					   [d,d,d,0]]))					   

# ._.
# |X|
#  -
m.append(numpy.matrix([[0,d,d,d],
					   [d,0,d,d],
					   [d,d,0,d],
					   [d,d,d,0]]))					   

print_stats(m)
