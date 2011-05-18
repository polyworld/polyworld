#!/usr/bin/python
from agent import *

# Verify argument is a cluster file
import sys
import os.path
assert len(sys.argv) > 1, "Must specify a cluster file!"
assert os.path.isfile(sys.argv[-1]), "Must specify a valid file!"

# load population
pop = get_population()

# load cluster info
import readcluster
cluster = readcluster.load(sys.argv[-1])
n_clusters = len(set(cluster.values()))
print n_clusters, "clusters"

# initialize variables
from collections import defaultdict
n = defaultdict(int)    # number


for agent in pop:
    #Only natural born critters
    if agent.birth_reason != 'NATURAL' or agent.death_reason == 'SIMEND':
        pop.remove(agent)
    else:
        #determine parents
        p1 = Agent(agent.parent1)
        p2 = Agent(agent.parent2)

        #determine parents' cluster number
        ia = cluster[agent.id]
        i1 = cluster[agent.parent1]
        i2 = cluster[agent.parent2]

        #determine fitness
        fa = agent.fitness
        fp1 = p1.fitness 
        fp2 = p2.fitness

        if fa is None or fp1 is None or fp2 is None:
            pop.remove(agent)
        else:
            if i1 < i2:
                n[(i1,i2)] += 1
            else:
                n[(i2,i1)] += 1
same = 0
diff = 0

for x in range(n_clusters):
    for y in range(n_clusters):
        print n[(x,y)],",",
        if x == y:
            same += n[(x,y)]
        else:
            diff += n[(x,y)]
   
    print " "


print "same cluster total:", same
print "diff cluster total:", diff
