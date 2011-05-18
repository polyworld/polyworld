'''
agent_plotcluster.py
USAGE: python agent_plotcluster.py <CLUSTER_PICKLE>

Takes a cluster file and plots agent movement and cluster populations across all
time steps for the default run directory.
'''


import matplotlib
matplotlib.use('agg')
from agent import *
import matplotlib.pyplot as plt
from matplotlib.colors import Normalize
from agent.plot import plot_step
import os


if __name__ == '__main__':
    import sys
    import os.path
    assert len(sys.argv) > 1, "Must specify a cluster file!"
    assert os.path.isfile(sys.argv[-1]), "Must specify a valid file!"
    anim_path = 'anim_%s' % sys.argv[-1][:-4]
    if not os.path.isdir(anim_path):
        os.mkdir(anim_path)
    frame_path = anim_path + '/frames'
    if not os.path.isdir(frame_path):
        os.mkdir(frame_path)
    '''
    print "unpickling clustsers ..."
    c = Cluster.load(sys.argv[-1])
    
    print "sorting clustsers ..."
    c.sort()
    cluster = c.agents
    del c
    
    print cluster
    exit()
    '''
    
    import readcluster
    clusters = readcluster.load_clusters(sys.argv[-1])

    new_clust = [clust for clust in clusters if len(clust) > 700]

    small_clust = []
    for clust in clusters:
        if len(clust) <= 700:
            small_clust.extend(clust)

    new_clust.append(small_clust)
    cluster = readcluster.agent_cluster(new_clust)
    
    n_clusters = len(set(cluster.values()))
    print n_clusters


    print "processing..." 
    # establish cluster object as a shared memory object.
    from multiprocessing import Pool, Manager
    manager = Manager()
    cluster = manager.dict(cluster)
    #p = manager.list(p)

    start = 1 
    stop = 30000
    '''
    # for testing purposes:
    p = get_population()
    def plot(t):
        plot_step(t, p, cluster)
    for t in range(start,stop,3):
        plot(t)
    ''' 

    for begin in range(start, stop, 3000):
        end = min(begin + 2999, stop)

        print "getting population at %d-%d..." % (begin,end)
        p = get_population_during_time(begin,end)

        def plot(t):
            plot_step(t, p, cluster, n_clusters, frame_path, cmap='jet')
        '''
        for t in range(begin,end,3):
            plot(t)
        '''
        #result = pl.map_async(plot, range(begin,end,3))
        # plot agents at every 3 steps
        pl = Pool()
        result = pl.map_async(plot, range(begin,end,3))
        result.get()
        pl.close()
        del p
   
    import subprocess
    subprocess.call(("mencoder mf://%s/*.png -o %s/output.avi -mf type=png:w=600:h=800:fps=30 -ovc x264 -x264encopts qp=20" % (frame_path, anim_path) ).split())
