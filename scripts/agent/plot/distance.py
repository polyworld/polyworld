#!/usr/bin/python
import matplotlib
matplotlib.use('agg')
from pylab import *

from agent import *
import agent.genome as genome
import readcluster as rc

import os
import os.path

import numpy as np
from scipy.stats import zscore
def dist(x,y, root=True):
    dist = np.sum((np.array(x[:]) - np.array(y[:])) ** 2)
    if not root:
        return dist
    else:
        return np.sqrt(dist)

def dists(agent1, agent2):
    genetic_dist = dist(agent1.ngenome,
                        agent2.ngenome,
                        root = True)
    geographic_dist = dist(agent1.positions[agent1.birth + 1],
                           agent2.positions[agent2.birth + 1],
                           root=True)

    return (geographic_dist, genetic_dist)

def dists_wrapper(args):
    return dist(*args)


from multiprocessing import Pool
def plot(cluster, filename="plot.png", plot_title='', cmap='RdBu_r'):
    ac = rc.load(cluster)
    clusters = rc.load_clusters(cluster)

    p = get_population()[400:4000]
    p = [a for a in p if a.birth != 30000]

    #normalize genomes
    print "normalizing genomes..."
    gs = zscore([a.genome[:] for a in p], axis=1)
    for i, agent in enumerate(p):
        agent.ngenome = gs[i]
        del agent.genome

    print "calculating distances..."
    gene_results = []
    geo_results = []

    pool = Pool()
    for i,agent1 in enumerate(p):
        print agent1.id, "to everything else"
       
        x = [(agent1.ngenome, agent2.ngenome) for agent2 in p[i+1:]]
        r = pool.map(dists_wrapper, x)
        gene_results.extend(r)
 
        y = [(agent1.positions[agent1.birth+1], 
              agent2.positions[agent2.birth+1]) 
                 for agent2 in p[i+1:]]

        r = pool.map(dists_wrapper, y)
        geo_results.extend(r)

        #for agent2 in p[i+1:]:
        #    results.append(dists(agent1, agent2))

        del agent1.ngenome
        del agent1.positions

    xs = geo_results
    ys = gene_results

    print "plotting..."
    hexbin(xs, ys, cmap=cmap, alpha=0.6, edgecolors='none', mincnt=25)
    title(plot_title)
    xlabel('Geographic Distance')
    ylabel('Genetic Distance')
    xlim( (0,142) )
    ylim( (1000,6000) )
    colorbar()
    savefig(filename, dpi=200)


if __name__ == '__main__':
    import sys
    from optparse import OptionParser

    usage = "usage: %prog [options] cluster_file"
    parser = OptionParser(usage)
    parser.set_defaults(mode='gene', gene=None, outfile=None, cmap='jet')
    parser.add_option("-o", "--output", dest="outfile",
                      help="specify an output file for the plot")
    parser.add_option("-m", "--colormap", dest="cmap",
                      help="specify a pylab.cm colormap")

    options, args = parser.parse_args()

    if args:
        cluster = args[-1]
    else:
        parser.print_help()
        print "\n*** ERROR: Please specify a cluster file! ***"
        sys.exit(1)

    if options.outfile:
        outfile = options.outfile
    else:
        #autoname rendering
        if not os.path.isdir('plots'):
            os.mkdir('plots')

        root = 'plots/dist'
        root += os.path.basename(cluster)
        root = root.rsplit('.', 1)[0]
        outfile = "%s_%s.png" % (root, options.mode)

    plot(cluster, outfile, cmap=options.cmap) 
