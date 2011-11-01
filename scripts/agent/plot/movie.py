'''
Takes a cluster file and plots agent movement and cluster populations across all
time steps for the default run directory.
'''
from multiprocessing import Pool, Manager
import os
import os.path
import subprocess

import matplotlib
matplotlib.use('agg')
import matplotlib.pyplot as plt
import matplotlib.cm as cm
from matplotlib.ticker import NullFormatter
from matplotlib.colors import Normalize

from agent import *
import readcluster as rc

def plot(cluster_file, run_dir='../run/', anim_path=None, frame_path=None,
         min_size=None):
    # configure default animation and frame path
    if anim_path is None:
        anim_path = 'anim'
    if frame_path is None:
        frame_path = os.path.join(anim_path, 'frames')

    # ensure paths exist
    if not os.path.isdir(anim_path):
        os.mkdir(anim_path)
    if not os.path.isdir(frame_path):
        os.mkdir(frame_path)
   
    # load clusters
    clusters = rc.load_clusters(cluster_file, sort='random')

    # compress, if a threshold is set
    if min_size is not None:
        clusters = compress_clusters(clusters, min_size)
   
    # calculate number of clusters
    n_clusters = len(clusters)
    agent_cluster = rc.agent_cluster(clusters)

    # establish cluster object as a shared memory object.
    #manager = Manager()
    #cluster = manager.dict(agent_cluster)

    start = 1 
    stop = 300
    
    # grouping to keep memory footprint reasonably small.
    # TODO: Fix this using iterators and multiprocessing.Pool(). Running into
    # major issues with serialization of Agent class that is preventing
    # plot_step from being picklable.
    for begin in xrange(start, stop, 1500):
        end = min(begin + 1499, stop)
        
        p = get_population_during_time(begin, end)
              
        for t in xrange(begin, end, 3):
            plot_step(t, p, agent_cluster, n_clusters, frame_path, cmap='Paired')
   
    subprocess.call(("mencoder mf://%s/*.png -o %s/output.avi -mf type=png:w=600:h=800:fps=30 -ovc x264 -x264encopts qp=20" % (frame_path, anim_path) ).split())

def main(run_dir, cluster_file, anim_path=None, frame_path=None):
    plot(cluster_file, run_dir=run_dir,
         anim_path=anim_path, frame_path=frame_path, min_size=700)

def compress_clusters(clusters, min_size=700):
    """
    Takes a set of clusters and collapses all clusters below min_size members
    into a single miscellaneous cluster. Returns the new list of clusters.
    """
    # initialize new cluster list and misc cluster
    new_clusters = []
    misc_cluster = []

    # append cluster to new cluster if over threshold, otherwise extend misc
    for cluster in clusters:
        if len(cluster) >= min_size:
            new_clusters.append(cluster)
        else:
            misc_cluster.extend(cluster)

    # add misc cluster to list of clusters
    new_clusters.append(misc_cluster)
    
    return new_clusters 

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
    if xs != []:
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

if __name__ == '__main__':
    import sys

    assert len(sys.argv) > 1, "Must specify a cluster file!"
    assert os.path.isfile(sys.argv[-1]), "Must specify a valid file!"

    anim_path = 'anim_%s' % sys.argv[-1][:-4]
    frame_path = os.path.join(anim_path, 'frames')
    
    cluster_file = sys.argv[-1]

    main('../run/', cluster_file, anim_path, frame_path)


