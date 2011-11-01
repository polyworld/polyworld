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
          'figure.figsize': (3.4,1.7),
          'figure.subplot.bottom' : 0.125
          }
pylab.rcParams.update(params)

from agent import *
import numpy as np
from math import sqrt
from agent.genome import get_label

def plot(genes, filename="plot.png", plot_title='', cmap='Paired', 
         draw_legend=False):
    # get population
    p = get_population()

    # initialize population count
    pops = [[] for i in xrange(30000)]
    for a in p:
        for step in range(a.birth, a.death):
            pops[step].append(a)

    # initialize plot variables
    cmap = pylab.cm.jet
    lines=[]

    # build plot for each gene
    for i, gene in enumerate(genes):

        # initialize line variables
        data = []
        err_up = []
        err_down = []
        
        # calculate points for each time step
        for step in xrange(30000):
            datum = [agent.genome[gene] for agent in pops[step]]
            mean = sum(datum) / len(pops[step])
            stderr = np.std(datum) / sqrt(len(pops[step]))

            # calculate 1 std dev error bars
            err_up.append(min(mean+np.std(datum), 255))
            err_down.append(max(mean-np.std(datum),0))
            data.append(mean)
            
            # TODO: Implement stderr/stddev toggle
            """
            # calculate 1 std err error bars
            err_up.append(min(mean+stderr, 255))
            err_down.append(max(mean-stderr,0))
            """
        
        color = cmap(float(i)/len(genes))

        # use error bars
        fill = pylab.fill_between(range(0,30000,5), err_up[::5], err_down[::5], 
                                  color=color, alpha=0.35, linewidth=0,
                                  edgecolor=None, facecolor=color)
        # plot line
        line = pylab.plot(range(30000), data, color=color)

        # append to list of lines for use with data legend
        lines.append(line)

    # plot all the things
    pylab.title("Size and Internal Neural Group Count (INGC)", weight='black')
    pylab.ylim(0, 255)
    pylab.xlim(0, 30000)
    pylab.ylabel("Raw Gene Value", weight='bold')
    pylab.xlabel("Time", weight='bold')
    pylab.legend(lines, [get_label(gene) for gene in genes], loc='center right')
    pylab.figtext(0,.948, '(d)', size=6, weight='black')
    pylab.savefig("plots/test.pdf", dpi=300)

plot([5,11])
