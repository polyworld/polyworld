# These functions may be imported into
# .../site-packages/networkx/algorithms/traversal/path.py
# by removing the import of networkx and eliminating
# "nx." from the other function invocations.
# They are defined here so it is possible to use
# these algorithms without modifying NetworkX.
# It is suggested that this extension module be imported as:
#
# import networkx_extensions as nxe

import networkx as nx

def efficiency(G, weighted=False):
	""" Return the efficiency.

	Parameters
	----------
	G : NetworkX graph

	weighted : bool, optional, default=False 
	   If true use edge weights on path.  If False,
	   use 1 as the edge distance.
	   (weighted not yet implemented)

	Examples
	--------
	>>> G=nx.path_graph(4)
	>>> print nx.efficiency(G)
	0.722222222222

	"""
	if weighted:
		print 'Weighted efficiency not yet implemented'
		sys.exit(1)

	shortest_paths = nx.all_pairs_shortest_path_length(G)
	
	N = len(G)
	sum = 0.0
	for i in range(N):
		for j in range(N):
			if i != j:
				try:
					sum += 1.0 / float(shortest_paths[i][j])
				except:
					pass
	
	efficiency = sum / (N * (N - 1))

	return efficiency
	

def connectivity_length(G, weighted=False):
	""" Return Marchiori & Latora's connectivity length.

	Parameters
	----------
	G : NetworkX graph

	weighted : bool, optional, default=False 
	   If true use edge weights on path.  If False,
	   use 1 as the edge distance.
	   (weighted not yet implemented)

	Examples
	--------
	>>> G=nx.path_graph(4)
	>>> print nx.connectivity_length(G)
	1.38461538462

	"""

	if weighted:
		path_length = nx.single_source_dijkstra_path_length
	else:
		path_length = nx.single_source_shortest_path_length

	N = len(G)
	sum = 0.0
	for n in G:
		lengths = path_length(G,n).values()
		for length in lengths:
			if length != 0:
				sum += 1.0 / length

	try:
		connectivity_length = (N * (N - 1)) / sum
	except ZeroDivisionError:
		connectivity_length = float('+inf')

	return connectivity_length


def normalized_path_length(G, weighted = False, w_max = 1.0):
	""" Return the normalized path length.

	Parameters
	----------
	G : NetworkX graph

	weighted : bool, optional, default=False 
	   If true use edge weights on path.  If False,
	   use 1 as the edge distance.


	"""

	if weighted:
		path_length=nx.single_source_dijkstra_path_length
		d_min = 1.0 / w_max
	else:
		path_length=nx.single_source_shortest_path_length
		d_min = 1

	N = len(G)
	d_max = N * d_min
	sum = 0.0
	for n in G:
		lengths = path_length(G,n).values()
		#print n, ':', lengths
		l = len(lengths)
		for i in range(l):
			sum += min(lengths[i], d_max )
		sum += (N-l) * d_max  # for the unreachable nodes

	# abs() is because roundoff error can make it go slightly negative (-10^-16)
	npl = abs((sum / (N*N - N)  -  d_min) / (d_max  -  d_min))

	return npl
	

# This is a replacement for, and based on, NetworkX's average_shortest_path_length()
# The key difference is that it obtains the average by dividing by the actual
# number of nodes that contributed to the sum, instead of the number of nodes
# in the network (some of which may not be linked to any other nodes,
# especially in directed graphs).
def characteristic_path_length(G, weighted=False):
	""" Return the characteristic path length.

	Parameters
	----------
	G : NetworkX graph

	weighted : bool, optional, default=False 
	   If true use edge weights on path.  If False,
	   use 1 as the edge distance.

	Examples
	--------
	>>> G=nx.path_graph(4)
	>>> print nx.average_shortest_path_length(G)
	1.25

	"""
	if weighted:
		path_length = nx.single_source_dijkstra_path_length
	else:
		path_length = nx.single_source_shortest_path_length

	num_connected_nodes = 0
	avg = 0.0
	for n in G:
		l = path_length(G,n).values()

		if len(l) > 1:	# > 1 because NetworkX includes a zero-length self-connection in this list
			num_connected_nodes += 1
			avg += float(sum(l)) / (len(l)-1)
	
	if num_connected_nodes > 0:
		return avg / num_connected_nodes
	else:
		return float('inf')

## Here is how NetworkX does it (or at least used to):
##	  avg = 0.0
##	  for n in G:
##		  l = path_length(G,n).values()
##		  avg += float(sum(l))/len(l)
##
##	  return avg/len(G)


def edge_weight_lims(G):
	# Find the minimum and maximum edge weights
	minweight = float('inf')
	maxweight = -1
	for i,j,d in G.edges_iter(data=True):
		wt=d.get('weight',1)
		if i != j:
			if wt < minweight:
				minweight = wt
			if wt > maxweight:
				maxweight = wt
	if minweight == float('inf'):
		minweight = 1
	if maxweight == -1:
		maxweight = 1
	return minweight, maxweight
