from agent import average_step, get_agents
import random
random.seed(42)

def _load(filename, sort=True):
    f = open(filename)
    clusters = []
    
    for line in f:
        if line.startswith('cluster'):
            a = line.split()
            a = map(int, a[5:])
            clusters.append(a)
    
    f.close()
    
    if sort == 'random':
        clusters.sort(key=lambda x:random.random())
    elif sort:
        clusters.sort(key=lambda x:average_step(get_agents(x)))


    return clusters

def load(filename, sort=True):
    clusters = _load(filename, sort) 

    return agent_cluster(clusters)

def agent_cluster(clusters):
    agent_cluster = {}

    for i, cluster in enumerate(clusters):
        for agent in cluster:
            agent_cluster[agent] = i

    return agent_cluster

def load_clusters(filename,sort=True):
    clusters = _load(filename,sort)

    return clusters

if __name__ == '__main__':
    import sys
    print load(sys.argv[-1])
