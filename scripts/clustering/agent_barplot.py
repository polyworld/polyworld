#!/usr/bin/python
import matplotlib
matplotlib.use('ps')
from matplotlib.colors import Normalize

import numpy as np
from pylab import *

from agent import *
import agent.genome as genome
import readcluster as rc

from math import log, sqrt 


def plot(cluster, filename="plot-filter.eps", funcs=(lambda a: a.id),
         plot_title='Scores by Cluster', cmap='Paired', filter=lambda a: True,
         labels=('id'), plot_pop=False):
    assert len(labels) == len(funcs), "must have a label for every func"
    ac = rc.load(cluster)
    clusters = rc.load_clusters(cluster)
    clusters = [[i, clust] for i,clust in enumerate(clusters) 
                    if len(clust) > 700]
    ids, clusters = zip(*clusters)
    num_clusters = len(clusters)

    ind = np.arange(num_clusters)
    if plot_pop:
        width = 1.0 / (len(funcs) + 2)
    else:
        width = 1.0 / (len(funcs) + 1)

    data = [[np.average([func(a) for a in clust if filter(a)]) 
                for clust in clusters]
                    for func in funcs]
    if plot_pop:
        data.append([len(clust) / 25346.0 for clust in clusters])
        labels.append('population')

    bars = []
    for offset,datum in enumerate(data):
        b = bar(ind+(offset*width), datum, width,
                color=cm.Paired(float(offset)/len(funcs)))
        bars.append(b)

    ylabel('normalized value')
    title(plot_title)
    ylim(ymin=0)
    xticks(ind + ( width*len(funcs) / 2), 
           ["%d: k=%d" % (ids[i], len(clust)) 
                for i, clust in enumerate(clusters)],
           fontsize=8)
    legend([b[0] for b in bars], labels, loc='upper left', prop=dict(size=6)) 

    savefig(filename, dpi=200)


if __name__ == '__main__':
    import sys
    from optparse import OptionParser

    usage = "usage: %prog [options] cluster_file"
    parser = OptionParser(usage)
    options, args = parser.parse_args()

    if args:
        cluster = args[-1]

    plot(cluster, funcs=(lambda x: Agent(x).genome[4] / 255.0, 
                         lambda x: Agent(x).genome[5] / 255.0,
                         lambda x: Agent(x).genome[7] / 255.0,
                         lambda x: Agent(x).genome[11] / 255.0,
                         lambda x: Agent(x).complexity),
         labels=[genome.get_label(4),
                 genome.get_label(5),
                 genome.get_label(7),
                 genome.get_label(11),
                 'complexity'
                 ])

