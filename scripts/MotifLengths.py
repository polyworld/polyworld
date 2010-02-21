#!/usr/local/bin/python

# MotifLengths - Calculates and displays characteristic path length,
#                normalized shortest path length, and 1 - efficiency
#                for all 3-node and 4-node undirected graph motifs
#
# NOTE: Requires modified version of NetworkX that supports normalized
#       shortest path calculation

# Produces the following output:
# 0: nodes = 3, links = 0, cpl = 0.0000, nsp = 1.0000, 1-e = 1.0000
# 1: nodes = 3, links = 1, cpl = 1.0000, nsp = 0.6667, 1-e = 0.6667
# 2: nodes = 3, links = 2, cpl = 1.3333, nsp = 0.1667, 1-e = 0.1667
# 3: nodes = 3, links = 3, cpl = 1.0000, nsp = 0.0000, 1-e = 0.0000
# 
# 0: nodes = 4, links = 0, cpl = 0.0000, nsp = 1.0000, 1-e = 1.0000
# 1: nodes = 4, links = 1, cpl = 1.0000, nsp = 0.8333, 1-e = 0.8333
# 2: nodes = 4, links = 2, cpl = 1.3333, nsp = 0.5556, 1-e = 0.5833
# 3: nodes = 4, links = 3, cpl = 1.6667, nsp = 0.2222, 1-e = 0.2778
# 4: nodes = 4, links = 3, cpl = 1.5000, nsp = 0.1667, 1-e = 0.2500
# 5: nodes = 4, links = 4, cpl = 1.3333, nsp = 0.1111, 1-e = 0.1667
# 6: nodes = 4, links = 5, cpl = 1.1667, nsp = 0.0556, 1-e = 0.0833
# 7: nodes = 4, links = 6, cpl = 1.0000, nsp = 0.0000, 1-e = 0.0000

import networkx as nx
import numpy
import sys

def print_stats(m):
	print
	for i in range(len(m)):
		g = nx.from_numpy_matrix(m[i],nx.Graph())  # undirected
		cpl = nx.average_shortest_path_length(g)
		nsp = nx.normalized_average_shortest_path_length(g)
		e = nx.efficiency(g)
		
		print '%d: nodes = %d, links = %d, cpl = %6.4f, nsp = %6.4f, 1-e = %6.4f' % (i, g.number_of_nodes(), g.number_of_edges(), cpl, nsp, 1-e)

m = []

# . .
#  .
m.append(numpy.matrix([[0,0,0],
					   [0,0,0],
					   [0,0,0]]))

# ._.
#  .
m.append(numpy.matrix([[0,1,0],
					   [1,0,0],
					   [0,0,0]]))

# ._.
# \.
m.append(numpy.matrix([[0,1,1],
					   [1,0,0],
					   [1,0,0]]))

# ._.
# \./
m.append(numpy.matrix([[0,1,1],
					   [1,0,1],
					   [1,1,0]]))

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
m.append(numpy.matrix([[0,1,0,0],
					   [1,0,0,0],
					   [0,0,0,0],
					   [0,0,0,0]]))					   

# ._.
# | .
m.append(numpy.matrix([[0,1,1,0],
					   [1,0,0,0],
					   [1,0,0,0],
					   [0,0,0,0]]))					   

# ._.
# |_.
m.append(numpy.matrix([[0,1,1,0],
					   [1,0,0,0],
					   [1,0,0,1],
					   [0,0,1,0]]))					   

# ._.
# |\.
m.append(numpy.matrix([[0,1,1,1],
					   [1,0,0,0],
					   [1,0,0,0],
					   [1,0,0,0]]))					   

# ._.
# |\|
m.append(numpy.matrix([[0,1,1,1],
					   [1,0,0,1],
					   [1,0,0,0],
					   [1,1,0,0]]))					   

# ._.
# |\|
#  -
m.append(numpy.matrix([[0,1,1,1],
					   [1,0,0,1],
					   [1,0,0,1],
					   [1,1,1,0]]))					   

# ._.
# |X|
#  -
m.append(numpy.matrix([[0,1,1,1],
					   [1,0,1,1],
					   [1,1,0,1],
					   [1,1,1,0]]))					   

print_stats(m)
