#!/usr/bin/python
import matplotlib
from matplotlib.colors import Normalize

import numpy as np
from pylab import *
from scipy.stats.stats import sem

params = {'backend': 'pdf',
          'grid.linewidth' : 0.5,
          'lines.linewidth' : 0.5,
          'axes.linewidth' : 0.5,
          'patch.linewidth' : 0.25,
          'xtick.major.pad' : 2,
          'xtick.major.size' : 2,
          'ytick.major.pad' : 2,
          'ytick.major.size' : 2,
          'axes.titlesize': 6,
          'axes.labelsize': 5,
          'text.fontsize': 5,
          'legend.fontsize': 5,
          'xtick.labelsize': 4,
          'ytick.labelsize': 4,
          'text.usetex': False,
          'figure.figsize': (3.4,2.38),
          'figure.subplot.bottom' : 0.125
          }
rcParams.update(params)

from agent import *
import agent.genome as genome
import readcluster as rc

from math import log, sqrt 


def plot(cluster, funcs, labels, filename="fingerprints.pdf",
         plot_title='Scores by Cluster', cmap='Paired', filter=lambda a: True,
         plot_pop=False, run_dir="../run/"):
    assert len(labels) == len(funcs), "must have a label for every func"

    # load in cluster data
    ac = rc.load(cluster)
    clusters = rc.load_clusters(cluster)

    # compress clusters
    # TODO: replace with call to agent.cluster.compress(700)
    clusters = [[i, clust] for i,clust in enumerate(clusters) 
                    if len(clust) > 700]
    ids, clusters = zip(*clusters)
    num_clusters = len(clusters)

    # build up cluster data
    data = [[np.average([func(a) for a in clust if filter(a)]) 
                for clust in clusters]
                    for func in funcs]
    err = [[np.std([func(a) for a in clust if filter(a)]) /
            np.sqrt(len([func(a) for a in clust if filter(a)]))
               for clust in clusters]
                   for func in funcs]

    # plot actual bars for each function
    ind = np.arange(num_clusters)
    width = 1.0 / (len(funcs) + 1)
    bars = []
    for offset,datum in enumerate(data):
        b = bar(ind+(offset*width), datum, width,
                color=cm.Paired(float(offset)/len(funcs)),
                yerr=err[offset], capsize=1.5)
        bars.append(b)

    # generate final plot
    title(plot_title, weight='black')
    ylabel('Normalized Value', weight='bold')
    ylim(ymin=0,ymax=1.0)
    xlabel('Cluster', weight='bold')
    xticks(ind + ( width*len(funcs) / 2), 
           ["%d" % ids[i] 
                for i, clust in enumerate(clusters)])
    legend([b[0] for b in bars], labels, loc='upper left') 

    savefig(filename, dpi=200)

def get_gene_fn(i):
    return lambda x: Agent(x).genome[i] / 255.0

def main(run_dir, cluster_file, genes=None):
    if genes is None:
        genes = [4,5,7,11]

    funcs = [get_gene_fn(i) for i in genes]
    labels = [genome.get_label(i, abbr=True) for i in genes]

    plot(cluster_file, funcs, labels, 
         plot_title="Representative Gene Values", run_dir=run_dir)

if __name__ == '__main__':
    import sys
    import os.path
    from optparse import OptionParser

    usage = "usage: %prog [options] cluster_file"
    parser = OptionParser(usage)
    options, args = parser.parse_args()

    if args:
        cluster = args[-1]

    if not cluster or not os.path.isfile(cluster):
        print "Must specify a valid cluster file!"
        sys.exit()

    main('../run/', cluster)
