'''
agent_cluster.py
USAGE: python agent_cluster.py <CLUSTER_PICKLE>

Calculates agent clusters based on genetic information. Outputs resulting
cluster object to <CLUSTER_PICKLE>. If <CLUSTER_PICKLE> is an existing file,
will continue clustering from the continuation.
'''

import pickle

import random
from numpy import dot, isnan, average, array, ndarray, zeros
from scipy.spatial.distance import euclidean as distance
from scipy.stats.stats import zs
from agent import average_step

from multiprocessing import Pool
from itertools import izip

def _distance(args):
    ''' wrapper function for Pool.map support '''
    return distance(*args)

class Cluster:
    ''' class to represent a k-means clustering object. '''
    def __init__(self, p, k, genes=range(12)):
        self.p = p[:]
        self.k = k 
        self.n = len(p)
        self.d = len(genes)
        self.genes = genes
        #self.gs = zs([agent.genome.select(genes) for agent in self.p])
        self.gs = [agent.genome.select(genes) for agent in self.p]
        for i, agent in enumerate(self.p):
            agent.ngenome = self.gs[i]

        self.clusters = []
        while len(self.clusters) < self.k:
            self._random_cluster()

        for agent in self.p:
            distances = [distance(agent.ngenome, self.clusters[j]['centroid']) 
                            for j in range(len(self.clusters))] 
            cluster = Cluster.index_min(distances)
            print agent.id, cluster
            self.clusters[cluster]['agents'].append(agent)
        for cluster in self.clusters:
            if len(cluster['agents']) > 0:
                genomes = array([agent.ngenome for agent in cluster['agents']])
                cluster['centroid'] = average(genomes, axis=0)
                cluster['radius'] = max([distance(g, cluster['centroid'])
                                                        for g in genomes])
                print cluster['centroid'], cluster['radius']


        self.hasNext = True
        self.iterations = 0

    def _random_cluster(self):
        new = random.choice(self.gs)
        self.clusters.append({
                'centroid' : random.choice(self.gs),
                'radius' : 300000,
                'agents' : []
            })

    def cluster_until_stable(self):
        ''' sets up a loop to cluster until a stability is achieved '''
        print "clustering (round %d)" % self.iterations 
        while(self.hasNext):
            self.cluster()
    
    def cluster(self):
        if self.hasNext:
            
            old_clusters = self.clusters[:]

            # calculate distance matrix
            dist = zeros((self.k, self.k))
            for i in range(self.k-1):
                c1 = self.clusters[i]
                compare_to = []
                for j in range (i+1, self.k):
                    c2 = self.clusters[j]
                    dist[i][j] = dist[j][i] = distance(c1['centroid'], 
                                                       c2['centroid'])


            print dist

            for i, cluster in enumerate(self.clusters):
                compare_to = []
                for j, c2 in enumerate(self.clusters):
                    if (tuple(cluster['centroid']) != tuple(c2['centroid'])) and\
                            (cluster['radius'] + c2['radius']) > dist[i][j]:
                        compare_to.append(j)

                print compare_to
                print "clustering agents in %d: %d"\
                        % (i,len(old_clusters[i]['agents']))
                
                for agent in old_clusters[i]['agents']:
                    d1 = distance(agent.ngenome, cluster['centroid'])
                    distances = [distance(agent.ngenome, self.clusters[j]['centroid']) 
                                    for j in compare_to]
                    if min(distances) < d1:
                        closest_cluster = compare_to[Cluster.index_min(distances)]
                        cluster['agents'].remove(agent)
                        self.clusters[closest_cluster]['agents'].append(agent)

            print "completed clustering, recalculating centroids"

            to_remove = []
            for i,cluster in enumerate(self.clusters):
                print "old", cluster['centroid'], cluster['radius'], len(cluster['agents'])
                # centroid calculation using numpy for optimizaiton
                if len(cluster['agents']) > 0:
                    genomes = array([agent.ngenome for agent in cluster['agents']])
                    cluster['centroid'] = average(genomes, axis=0)

                    cluster['radius'] = max([distance(g, cluster['centroid'])
                                                    for g in genomes])

                    print "new", cluster['centroid'], cluster['radius'], len(cluster['agents'])

                else:
                    to_remove.append(i)

            to_remove.reverse()
            for cluster in to_remove:
                del self.clusters[cluster]

            while len(self.clusters) < self.k:
                self._random_cluster()

            self.iterations += 1 



    @staticmethod
    def index_min(L):
        ''' finds the index of the minimum entry of a list. '''
        cur_min = L[0]
        cur_i = 0
        for i,e in enumerate(L):
            if cur_min > e:
                cur_min = e
                cur_i = i

        return cur_i

    ''' sort clusters by average peak '''
    def sort(self):
        pair = zip(self.centroids, self.clusters)
        
        pair.sort(key=lambda x: average_step(x[1]))

        self.centroids, self.clusters = zip(*pair)
   

    ### PICKLING/SERIALIZATION ###
    def pickle(self, file):
        ''' pickle the Cluster object into the given file '''
        f = open(file, 'wb')
        pickle.dump(self,f)
        f.close()

    @staticmethod
    def load(file):
        ''' load a Cluster picle ''' 
        f=open(file)
        c=pickle.load(f)
        f.close()
        return c
