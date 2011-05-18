#!/usr/bin/python
import matplotlib
matplotlib.use('agg')
from pylab import *

from agent import *
import agent.genome as genome
import readcluster as rc

import os
import os.path

def plot(cluster, filename="plot.png", func=lambda a: a.id, 
         plot_title='', cmap='Paired', filter=lambda a: True):
    ac = rc.load(cluster)
    clusters = rc.load_clusters(cluster)

    p = get_population()
    p.sort(key=lambda a:-len(clusters[ac[a.id]]))

    xs = [(a.birth + a.death) / 2 for a in p if filter(a)]
    ys = [func(a) for a in p if filter(a)]
    cs = [ac[a.id] for a in p if filter(a)]

    scatter(xs,ys, cmap=cmap, c=cs, s=10, alpha=0.6, edgecolors='none')
    title(plot_title)
    xlim(0,30000)
    ylim(ymin=0)
    savefig(filename, dpi=200)
    

if __name__ == '__main__':
    import sys
    from optparse import OptionParser

    usage = "usage: %prog [options] cluster_file"
    parser = OptionParser(usage)
    parser.set_defaults(mode='gene', gene=None, outfile=None, cmap='Paired')
    parser.add_option("-f", "--fitness", action="store_const",
                      dest='mode', const='fitness',
                      help="draw fitness scatter plot")
    parser.add_option("-c", "--complexity", action="store_const",
                      dest='mode', const='complexity',
                      help="draw complexity scatter plot")
    parser.add_option("-g", "--gene", dest="gene", type="int",
                      help="draw gene scatter plot for GENE")
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

        root = 'plots/'
        root += os.path.basename(cluster)
        root = root.rsplit('.', 1)[0]
        outfile = "%s_%s.png" % (root, options.mode)

    if options.mode == 'complexity': 
        plot(cluster, outfile, filter=lambda a: a.complexity,
             func=lambda a: a.complexity, cmap=options.cmap)
    elif options.mode == 'fitness':
        plot(cluster, outfile, filter=lambda a: a.fitness,
             func=lambda a: a.fitness, cmap=options.cmap)
    elif options.gene is not None:
        if not options.outfile:
            outfile = "%s_%s%d_%s.png" % (root, options.mode, options.gene,
                                          genome.get_label(options.gene))

        plot(cluster, outfile,
             func=lambda a: a.genome[options.gene],
             plot_title=genome.get_label(options.gene), cmap=options.cmap)
    else:
        parser.print_help()
        print "\n*** ERROR: Please specify an action! ***"
        

        
