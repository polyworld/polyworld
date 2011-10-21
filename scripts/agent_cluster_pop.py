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

def plot(cluster, filename="plot.png", func=lambda a: a.id, 
         plot_title='', cmap='Paired', filter=lambda a: True, 
         draw_legend=False, radius='2.25', sym=None):
    ac = rc.load(cluster)
    clusters = rc.load_clusters(cluster)

    p = get_population()
    pops = [0 for i in range(30000)]
    cluster_pops = [] 
    cluster_pop_max = []
    for clust in range(len(clusters)):
        cluster_pops.append(pops[:])
        cluster_pop_max.append([0,0,-1,0])

    for clust,agents in enumerate(clusters):
        for agent in agents:
            a = Agent(agent)
            for i in range(a.birth, a.death):
                cluster_pops[clust][i] += 1
                if cluster_pop_max[clust][2] == -1:
                    cluster_pop_max[clust][2] = i
                if i > cluster_pop_max[clust][3]:
                    cluster_pop_max[clust][3] = i
                if cluster_pops[clust][i] > cluster_pop_max[clust][0]:
                    cluster_pop_max[clust][0] = cluster_pops[clust][i]
                    cluster_pop_max[clust][1] = i
    
    lines=[]
    for i,clust in enumerate(cluster_pops):
        lines.append(pylab.plot(range(30000),clust, 
                         label=("%d: k=%d" % (i, len(clusters[i]))),
                         color=pylab.cm.Paired(float(i)/len(clusters))))

    if draw_legend:
        pylab.figlegend(lines, ["%d: k=%d" % (i, len(clust))
                                    for i,clust in enumerate(clusters)], 
                        'center right', 
                        ncol=((len(clusters)/35)+1), prop=dict(size=6))
    else:
        print "should not draw!!!"

    title = r"Cluster Population ($\epsilon$ = %s, %d clusters)" % (radius, len(clusters))
    pylab.title(title, weight='black')
    pylab.xlabel("Time", weight='bold')
    pylab.ylabel("Population Size", weight='bold')
    if sym is not None:
        pylab.figtext(0,.954, '(%s)' % sym, size=6, weight='black')

    pylab.savefig(filename, dpi=300)

    print 'cluster, totalPop, start, peak, stop, maxPop'
    for clust,agents in enumerate(clusters):
        print clust, len(agents), cluster_pop_max[clust][2], cluster_pop_max[clust][1], cluster_pop_max[clust][3]+1, cluster_pop_max[clust][0]


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

