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
f = defaultdict(int)    # fitness
c = defaultdict(int)    # children
g = defaultdict(int)    # grandchildren
df = defaultdict(int)   # delta fitness
dc = defaultdict(int)   # delta children
dg = defaultdict(int)   # delta grandchildren
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
            # begin calculating number of children
            ca = len(agent.children)
            cp1 = len(p1.children)
            cp2 = len(p2.children)
    
            ga = sum([len(Agent(child).children) for child in agent.children])
            gp1 = sum([len(Agent(child).children) for child in p1.children])
            gp2 = sum([len(Agent(child).children) for child in p2.children])
    
            f[(i1,i2)] += fa
            c[(i1,i2)] += ca
            g[(i1,i2)] += ga
    
            df[(i1,i2)] += fa - (0.5 * (fp1+fp2))
            dc[(i1,i2)] += ca - (0.5 * (cp1+cp2))
            dg[(i1,i2)] += ga - (0.5 * (gp1+gp2))
    
            n[(i1,i2)] += 1
            


print f
print c
print g
print df
print dc
print dg

#fold into matricies
ft = defaultdict(float)
ct = defaultdict(int)
gt = defaultdict(int)
dft = defaultdict(float)
dct = defaultdict(int)
dgt = defaultdict(int)
nt = defaultdict(int)

for key in n.keys():
    # create upper triangular matrix
    if key[0] > key[1]:
        key = (key[1], key[0])

    ft[key] += f[key]
    ct[key] += c[key]
    gt[key] += g[key]
    dft[key] += df[key]
    dct[key] += dc[key]
    dgt[key] += dg[key]
    nt[key] += n[key] 

print ft
print ct
print gt
print dft
print dct
print dgt

#fold into matricies
fd = 0
cd = 0
gd = 0
dfd = 0
dcd = 0
dgd = 0
nd = 0

fs = 0
cs = 0
gs = 0
dfs = 0
dcs = 0
dgs = 0
ns = 0

#calculate averages
for key in nt.keys():
    if key[0] == key[1]:
       fs += ft[key]
       cs += ct[key]
       gs += gt[key]
       dfs += dft[key]
       dcs += dct[key]
       dgs += dgt[key]
       ns += nt[key]
    else:
       fd += ft[key]
       cd += ct[key]
       gd += gt[key]
       dfd += dft[key]
       dcd += dct[key]
       dgd += dgt[key]
       nd += nt[key]

nd *= 1.0
ns *= 1.0

print "different cluster averages:"
print [x / nd for x in [fd, cd, gd, dfd, dcd, dgd]]

print "same cluster averages:"
print [x / ns for x in [fs, cs, gs, dfs, dcs, dgs]]
