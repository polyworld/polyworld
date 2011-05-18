'''
plot.py

Visualization tools for agents
'''

import matplotlib
import matplotlib.pyplot as plt
import matplotlib.cm as cm
from matplotlib.ticker import NullFormatter
from matplotlib.colors import Normalize

'''
For info on animation, use mencode on the output:
http://matplotlib.sourceforge.net/examples/animation/movie_demo.html

See also: make_movie.sh
'''

# default normalization object for 75 clusters. Will be reworked with
# new clustering tools to support an arbitrary # of clusters.

def plot_step(t, p, cluster, n_clusters, output_dir='anim', cmap='gist_earth', debug=False):
    ''' Function to plot the motion of a given population at a given timestep
    with cluster information. '''

    norm = Normalize(0,n_clusters)
    # grab agents alive at time t
    p_t = [a for a in p if a.alive_at_timestep(t)] 
    
    # set up bounds for histogram and main window
    left, width = 0.1, 0.6
    bottom, height = 0.4, 0.6
    bottom_h = left_h = left+width+0.05
    
    left, right = 0.1, 0.9
    bottom, top = 0.35, 0.95
    width = right-left
    height = top-bottom

    hist_bottom = 0.05
    hist_height = 0.2

    # create position arguments for graph figures
    sq_scatter = [left, bottom, width, height]
    rect_histx = [left, hist_bottom, width, hist_height]
     
    # configure figure dimensions and position and histogram objects
    fig = plt.figure(figsize=(6,8))
    pos = plt.axes(sq_scatter)
    hist = plt.axes(rect_histx)

    # Eliminate labels for both axes of the position graph. 
    # Eliminate x-axis labels for the histogram.
    nullfmt = NullFormatter()
    pos.xaxis.set_major_formatter(nullfmt)
    pos.yaxis.set_major_formatter(nullfmt)
    hist.yaxis.set_major_formatter(nullfmt)

    # set up 4 lists for x-pos, y-pos, size and color
    xs, ys, ss, cs = [[], [], [], []]
    nxs, nys, nss, ncs = [[], [], [], []]
    for a in p_t:
        if a.complexity:
            xs.append(a.positions[t][0])
            ys.append(a.positions[t][2])
            cs.append(cluster[a.id])
            ss.append(a.complexity*240)
        else:
            nxs.append(a.positions[t][0])
            nys.append(a.positions[t][2])
            ncs.append(cluster[a.id])
            nss.append(50)

    # create position graph
    if nxs != []:
        pos.scatter(nxs, nys, s=nss, alpha=0.75, cmap=cmap, c=ncs, norm=norm, marker='s')
    pos.scatter(xs, ys, s=ss, alpha=0.75, cmap=cmap, c=cs, norm=norm, marker='o')
    pos.set_title('agent positions at time %d' % t)
    pos.grid(True)


    if ncs:
        cs.extend(ncs)
    # initialize histogram
    N, bins, patches = hist.hist(cs, bins=range(n_clusters+1), orientation='horizontal')

    # Set histogram colors. Technique adapted from:
    # http://matplotlib.sourceforge.net/examples/pylab_examples/hist_colormapped.html
    cmap = getattr(cm, cmap)
    for i, patch in enumerate(patches):
        color = cmap(i/float(len(N)))
        patch.set_facecolor(color)
    hist.set_title('cluster populations')
    hist.set_ylim(0,n_clusters,auto=False) # auto=False disables resizing
    hist.set_xlim(0,250,auto=False)
    hist.set_autoscalex_on(False)
    hist.grid(True)
   
    # Write plot to file
    plt.savefig("%s/step%05d.png" % (output_dir, t), dpi=100)

    if debug:
        print "wrote file %d" % t

    #clear figure
    plt.clf()

    # ensure that memory is freed
    del p_t, xs, ys, ss, cs

