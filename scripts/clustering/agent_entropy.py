from multiprocessing import Pool
from agent import *
from agent.genome import gene_entropy, get_label

N = get_population()
N = [agent.genome for agent in N]

def calc_entropy(gene):
    e = 1- gene_entropy(gene, N)
    return (gene, e, get_label(gene))

pool = Pool()
result = pool.map_async(calc_entropy, range(0,2494))
result = result.get()
pool.close()

import pickle
import sys
f = open(sys.argv[-1], 'wb')
pickle.dump(result,f)
f.close()
