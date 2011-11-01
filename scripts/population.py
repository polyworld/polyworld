"""
Performs calculations over entire populations.
"""
from agent import *
import readcluster as rc
from itertools import repeat

def print_timeline(clusters, run_dir='../run/'): 
    """
    Print timeline with stats for totalPop, start, peak, stop, maxPop
    """
    p = get_population(run_dir=run_dir)

    cluster_pops = [] 
    cluster_pop_max = []
    for clust,agents in enumerate(clusters):
        cluster_pops.append(list(repeat(0, 30000)))
        # maxPop, peak, start, stop
        cluster_pop_max.append([0,0,-1,0])
        for agent in agents:
            a = Agent(agent)
            for i in range(a.birth, a.death):
                cluster_pops[clust][i] += 1
                
                # set cluster start
                if cluster_pop_max[clust][2] == -1:
                    cluster_pop_max[clust][2] = i

                # set cluster stop
                if i > cluster_pop_max[clust][3]:
                    cluster_pop_max[clust][3] = i

                # set max Population
                if cluster_pops[clust][i] > cluster_pop_max[clust][0]:
                    cluster_pop_max[clust][0] = cluster_pops[clust][i]
                    cluster_pop_max[clust][1] = i

    print 'cluster, totalPop, start, peak, stop, maxPop'
    for clust,agents in enumerate(clusters):
        print clust, len(agents), cluster_pop_max[clust][2], cluster_pop_max[clust][1], cluster_pop_max[clust][3]+1, cluster_pop_max[clust][0]

if __name__ == '__main__':
    import sys

    # get clusters
    cluster_file = sys.argv[-1]
    ac = rc.load(cluster_file)
    clusters = rc.load_clusters(cluster_file)

    print_timeline(clusters)
