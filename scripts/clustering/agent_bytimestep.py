from agent import *

if __name__ == '__main__':
    import sys
    import os.path
    assert len(sys.argv) > 1, "Must specify a cluster file!"
    assert os.path.isfile(sys.argv[-1]), "Must specify a valid file!"

    import readcluster
    cluster = readcluster.load(sys.argv[-1])
    n_clusters = len(set(cluster.values()))
    print n_clusters

    
    p = get_population()

    pops = [[0 for i in range(n_clusters)] for i in range(30000)]

    for agent in p:
        for step in range(agent.birth,agent.death):
            pops[step][cluster[agent.id]] += 1

    for step, ps in enumerate(pops):
        print step,
        for cluster in ps:
            print cluster,
        print step




