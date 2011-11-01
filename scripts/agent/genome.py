"""
genome.py

Genome class and helpers.
"""

import os.path
from numpy import array, histogram
from math import log
from lazy import Lazy

class Genome: 
    def __init__(self, filename):
        self.filename = filename
        assert os.path.isfile(self.filename),\
                "Invalid genome file: %s" % self.filename

    @Lazy
    def genome(self):
        """ Lazy loading of genome """

        # Open genome, clean lines
        f = open(self.filename)
        g = array(map(int, f.readlines()), dtype='u1')
        f.close()
        
        # Sanity check on genome
        assert 0 <= g.all() <= 255, "genes outside range [0,255]"

        return g

    def __getitem__(self, key):
        """ Allows for indexing over genome """
        return self.genome[key]

    def __len__(self):
        return len(self.genome)

    def select(self, indexes):
        S = []
        for i in indexes:
            S.append(self[i])
        return array(S, dtype='u1')



def gene_entropy(gene, genomes):
    '''Takes a list of genomes and calculates the entropy for the gene x'''
    num_agents = len(genomes)
    values = array([x[gene] for x in genomes])
    freq = histogram(values,bins=range(15,256,16), range=(0, 255))[0]

    return entropy(freq)



def entropy(frequencies):
    '''Calculates entropy over a list representing a frequency distribution.'''
    bins = len(frequencies)
    N = float(sum(frequencies))

    entropy = 0 
    for f in frequencies:
        p = f / N
        if p > 0:
            entropy += -p * log(p, bins)

    return entropy

import string
gene_index = []
def get_label(gene, index_filename='../run/geneindex.txt', abbr=False,
              length=None):
    '''Returns the label of a given gene index.'''

    # Lazy loading of gene index list.
    global gene_index
    if not gene_index:
        assert os.path.isfile(index_filename),\
            "Invalid geneindex.txt: %s" % index_filename

        f = open(index_filename)
        gene_index = [x.split()[1] for x in f.readlines()] 
        f.close()

    label = gene_index[gene]
    if abbr:
        if not length or len(label) < length:
            label = ''.join([x for x in label if x in string.uppercase or
                                                 x in string.punctuation or
                                                 x in string.digits])
    return label
