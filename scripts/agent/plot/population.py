#!/usr/bin/python
import matplotlib
import pylab 

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
          'legend.fontsize': 4,
          'xtick.labelsize': 4,
          'ytick.labelsize': 4,
          'text.usetex': False,
          'mathtext.default' : 'regular',
          'figure.figsize': (3.4,1.7),
          'figure.subplot.bottom' : 0.125
          }
pylab.rcParams.update(params)

from agent import *
import readcluster as rc
from collections import defaultdict
from itertools import repeat

def plot(cluster_file, filename="plot.png", func=lambda a: a.id, 
         plot_title='', cmap='Paired', filter=lambda a: True, 
         draw_legend=False, radius='2.25', sym=None, run_dir='../run/'):
    """
    Creates a line plot showing population per cluster
    """

    # retrieve cluster data from cluster file
    clusters = rc.load_clusters(cluster_file)

    # grab cluster population
    p = get_population(run_dir=run_dir)

    lines=[]
    for cluster, agents in enumerate(clusters):
        pop_by_time = list(repeat(0, 30000))
        for agent in agents:
            a = Agent(agent)
            for i in range(a.birth, a.death):
                pop_by_time[i] += 1
    
        lines.append(
            pylab.plot(range(30000), pop_by_time, 
                       label=("%d: k=%d" % (i, len(clusters[cluster]))),
                       color=pylab.cm.Paired(float(cluster)/len(clusters))))

    if draw_legend:
        pylab.figlegend(lines, ["%d: k=%d" % (i, len(cluster))
                                    for i,cluster in enumerate(clusters)], 
                        'center right', 
                        ncol=((len(clusters)/35)+1), prop=dict(size=6))

    title = r"Cluster Population ($\epsilon$ = %s, %d clusters)" % (radius, len(clusters))
    pylab.title(title, weight='black')
    pylab.xlabel("Time", weight='bold')
    pylab.ylabel("Population Size", weight='bold')
    if sym is not None:
        pylab.figtext(0,.954, '(%s)' % sym, size=6, weight='black')

    pylab.savefig(filename, dpi=300)



def main(run_dir, cluster_file):
    outfile = "plot.png"
    plot(cluster_file, filename=outfile, draw_legend=False, run_dir=run_dir)
    

if __name__ == '__main__':
    import sys
    from optparse import OptionParser

    usage = "usage: %prog [options] cluster_file"
    parser = OptionParser(usage)
    parser.set_defaults(radius=2.25, sym=None)
    parser.add_option('-r', dest ='radius')
    parser.add_option('-s', dest ='sym')
    options, args = parser.parse_args()

    if args:
        cluster = args[-1]
    else:
        parser.print_help()
        print "\n*** ERROR: Please specify a cluster file! ***"
        sys.exit(1)
        
    if not os.path.isdir('plots'):
        os.mkdir('plots')

    root = 'plots/pops_'
    root += os.path.basename(cluster)
    root = root.rsplit('.', 1)[0]
    outfile = "%s.pdf" % root

    #title = os.path.basename(cluster)[5:].rsplit('.',1)[0]
    plot(cluster, filename=outfile, draw_legend=False, radius=options.radius,
         sym=options.sym)
