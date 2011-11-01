#!/usr/bin/env python
import matplotlib
import pylab 
params = {'backend': 'pdf',
          'grid.linewidth' : 0.5,
          'lines.linewidth' : 0.5,
          'axes.linewidth' : 0.5,
          'patch.linewidth' : 0.25,
          'xtick.major.pad' : 2,
          'xtick.major.size' : 2,
          'ytick.major.pad' : 2,
          'ytick.major.size' : 2,
          'axes.titlesize': 6,
          'axes.labelsize': 5,
          'text.fontsize': 5,
          'legend.fontsize': 4,
          'xtick.labelsize': 4,
          'ytick.labelsize': 4,
          'text.usetex': False,
          'figure.figsize': (3.4,1.7),
          'figure.subplot.bottom' : 0.125
          }
pylab.rcParams.update(params)

from collections import defaultdict
from itertools import chain, izip
from math import *

from agent import *
import datalib
import readcluster as rc

def load(contact_filename, cluster, filter=None):
    '''Loads contact information from the given filename, and determines inter
    and intra-cluster contacts from the given set of clusters'''
    # initialize filter if not set
    if filter is None:
        filter = lambda row: True

    contact_table = datalib.parse(contact_filename)['Contacts']
    
    intra = defaultdict(int)
    inter = defaultdict(int)

    total = 0

    for row in contact_table:
        total += 1

        if filter(row):
            if cluster[row['Agent1']] == cluster[row['Agent2']]:
                intra[row['Agent1']] += 1
                intra[row['Agent2']] += 1
            else:
                inter[row['Agent1']] += 1
                inter[row['Agent2']] += 1
            if False and  __debug__ and (total % 1000) == 0:
                 print 'contacts[%d] = %d, intra = %d, inter = %d' %\
                         (row['Agent1'], 
                          intra[row['Agent1']] + inter[row['Agent1']], 
                          intra[row['Agent1']], inter[row['Agent1']])
                 print 'contacts[%d] = %d, intra = %d, inter = %d' %\
                         (row['Agent2'], 
                          intra[row['Agent2']] + inter[row['Agent2']], 
                          intra[row['Agent2']], inter[row['Agent2']])

    return (intra, inter)

def offspring_per_contact(id, offspring, contacts):
    """ Helper to determine offspring per contact """
    n = len(list(offspring))
    rate = float(n) / contacts[id] if contacts[id] else 0.0
    return rate

def fraction(key, primary, secondary):
    return float(primary[key]) / (primary[key] + secondary[key])

def contact_info(pop, cluster, start=None, stop=None, filter=None, 
                 inter=None, intra=None):
    # initialize filter if not set
    print "CALCULATING", start, stop
    """
    if filter is None:
        if start:
            filter = lambda row: row['Timestep'] >= start
        elif stop and not start:
            filter = lambda row: row['Timestep'] < stop
        elif start and stop:
            filter = lambda row: (row['Timestep'] > start) and (row['Timestep'] < stop)
    """

    # initialize contact variables
    if inter is None and intra is None:
        intra, inter = load('../run/events/contacts.log', cluster, filter)
    same = dict()
    hybrid = dict()
    total = dict()
    same['contacts'] = sum(intra.values()) / 2
    hybrid['contacts'] = sum(inter.values()) / 2
    total['contacts'] = (same['contacts'] + hybrid['contacts']) / 2

    # intialize offspring variables
    same['offspring'] = hybrid['offspring'] = 0
    same['goffspring'] = hybrid['goffspring'] = 0
    same['Goffspring'] = hybrid['Goffspring'] = 0 

    # initialize per-contact variables
    same['offspring-rate'] = hybrid['offspring-rate'] = 0
    same['goffspring-rate'] = hybrid['goffspring-rate'] = 0
    same['Goffspring-rate'] = hybrid['Goffspring-rate'] = 0 
    same['offspring-rate2'] = hybrid['offspring-rate2'] = 0
    same['goffspring-rate2'] = hybrid['goffspring-rate2'] = 0
    same['Goffspring-rate2'] = hybrid['Goffspring-rate2'] = 0 

    # process population 
    for agent in pop:
        # first filter out messy data due to start/end simulation effects
        # only natural born and dying critters
        if agent.birth_reason != 'NATURAL' or agent.death_reason == 'SIMEND':
            pop.remove(agent)
            continue
        # only critters within the range
        elif (start and agent.death < start) or (stop and agent.birth > stop):
            pop.remove(agent)
            continue

        #determine parents
        parent1 = Agent(agent.parent1)
        parent2 = Agent(agent.parent2)

        #determine fitness
        fa = agent.fitness
        f1 = parent1.fitness 
        f2 = parent2.fitness

        #remove agents without a complete fitness score
        if agent.fitness is None\
            or parent1.fitness is None\
            or parent2.fitness is None:
            pop.remove(agent)
            continue

        # OFFSPRING CALCULATIONS
        # create hybrid function for below
        purebred = lambda child:\
            cluster[Agent(child).parent1] == cluster[Agent(child).parent2]

        # get agent's children spawned with agents from its own cluster
        children_same = [child for child in agent.children if purebred(child)]
        ps = offspring_per_contact(agent.id, children_same, intra)
        same['offspring-rate'] += ps
        same['offspring-rate2'] += (ps * ps)
        #TODO: reinsert stddev numbers
        
        # get agent's grandchildren resulting from children 
        # spawned with agents from its own cluster
        # grandchildren may come from reproduction with any cluster
        #grandchildren_same = [grandchild for grandchild in Agent(child).children 
        #                           for child in children_same]
        grandchildren_same = chain.from_iterable(
            [child.children for child in map(Agent, children_same)])
        grandchildren_same = set(grandchildren_same)
        ps = offspring_per_contact(agent.id, grandchildren_same, intra)
        same['goffspring-rate'] += ps
        same['goffspring-rate2'] += (ps * ps)
        
        # get this agent's children spawned with agents from other clusters
        children_hybrid = [child for child in agent.children 
                               if not purebred(child)]
        ps = offspring_per_contact(agent.id, children_hybrid, inter)
        hybrid['offspring-rate'] += ps
        hybrid['offspring-rate2'] += (ps * ps)
        
        # get this agent's grandchildren resulting from children 
        # spawned with agents from other clusters
        # grandchildren may come from reproduction with any cluster
        grandchildren_hybrid = chain.from_iterable(
            [child.children for child in map(Agent, children_hybrid)])
        grandchildren_hybrid = set(grandchildren_hybrid)
        ps = offspring_per_contact(agent.id, grandchildren_hybrid, inter)
        hybrid['goffspring-rate'] += ps
        hybrid['goffspring-rate2'] += (ps * ps)
       
        if cluster[parent1.id] == cluster[parent2.id]:
            same['offspring'] += 1
            same['goffspring'] += len(agent.children)
            same['Goffspring'] += len(agent.grandchildren)
        else:
            hybrid['offspring'] += 1
            hybrid['goffspring'] += len(agent.children)
            hybrid['Goffspring'] += len(agent.grandchildren)

    #print len(grandchildren_hybrid) + len(grandchildren_same)
    #print len(agent.grandchildren)

    total['offspring'] = same['offspring'] + hybrid['offspring']
    total['goffspring'] = same['goffspring'] + hybrid['goffspring']
    total['Goffspring'] = same['Goffspring'] + hybrid['Goffspring']

    print "\nCounts..."
    print_int(same, hybrid, 
               keys=['offspring', 'goffspring', 'Goffspring', 'contacts'],
               labels=['off', 'goff', 'Goff', 'cont']) 

    ### FRACTIONS
    same['offspring-frac'] = fraction('offspring', same, hybrid) 
    same['goffspring-frac'] = fraction('goffspring', same, hybrid) 
    same['Goffspring-frac'] = fraction('Goffspring', same, hybrid) 
    same['contacts-frac'] = fraction('contacts', same, hybrid) 

    hybrid['offspring-frac'] = fraction('offspring', hybrid, same) 
    hybrid['goffspring-frac'] = fraction('goffspring', hybrid, same) 
    hybrid['Goffspring-frac'] = fraction('Goffspring', hybrid, same) 
    hybrid['contacts-frac'] = fraction('contacts', hybrid, same) 

    print "\nFractions..."
    print_float(same, hybrid, 
                keys=['offspring-frac', 'goffspring-frac', 
                      'Goffspring-frac', 'contacts-frac'],
                labels=['off', 'goff', 'Goff', 'cont']) 

    ### NORMALIZATION
    same['offspring-norm'] = same['offspring-frac']\
        / same['contacts-frac']
    same['goffspring-norm'] = same['goffspring-frac']\
        / same['contacts-frac']
    same['Goffspring-norm'] = same['Goffspring-frac']\
        / same['contacts-frac']
    if hybrid['contacts-frac']:
        hybrid['offspring-norm'] = hybrid['offspring-frac']\
            / hybrid['contacts-frac']
        hybrid['goffspring-norm'] = hybrid['goffspring-frac']\
            / hybrid['contacts-frac']
        hybrid['Goffspring-norm'] = hybrid['Goffspring-frac']\
            / hybrid['contacts-frac']
    else:
        hybrid['offspring-norm'] = 0.0
        hybrid['goffspring-norm'] = 0.0
        hybrid['Goffspring-norm'] = 0.0
    
    print "\nNormalized..."
    print_ratio(same, hybrid, 
                keys=['offspring-norm', 'goffspring-norm', 'Goffspring-norm'],
                labels=['off', 'goff', 'Goff']) 

    ### FECUNDITY 
    print "\nFecundity (number of offspring and grand-offspring)..."
    same['offspring-fecundity'] =\
        float(same['goffspring']) / same['offspring'] 
    same['goffspring-fecundity'] =\
        float(same['Goffspring']) / same['offspring'] 
    if hybrid['offspring']:
        hybrid['offspring-fecundity'] =\
            float(hybrid['goffspring']) / hybrid['offspring'] 
        hybrid['goffspring-fecundity'] =\
            float(hybrid['Goffspring']) / hybrid['offspring'] 
    else:
        hybrid['offspring-fecundity'] = 0.0
        hybrid['goffspring-fecundity'] = 0.0
    
    #TODO: Add std err back

    print_ratio(same, hybrid, 
                keys=['offspring-fecundity', 'goffspring-fecundity'],
                labels=['off', 'goff']) 

    ## CHILD PRODUCTION PER CONTACT
    print "\nChild production per contact..."
    same['offspring-rate'] /= total['offspring']
    same['goffspring-rate'] /= total['offspring']

    hybrid['offspring-rate'] /= total['offspring']
    hybrid['goffspring-rate'] /= total['offspring']
    
    print_ratio(same, hybrid, 
                keys=['offspring-rate', 'goffspring-rate'],
                labels=['off', 'goff']) 

    same['offspring-rate-err'] = (
        sqrt(same['offspring-rate2'] / total['offspring']
             - same['offspring-rate'] * same['offspring-rate'])
        / sqrt(total['offspring']))
    hybrid['offspring-rate-err'] = (
        sqrt(hybrid['offspring-rate2'] / total['offspring']
             - hybrid['offspring-rate'] * hybrid['offspring-rate'])
        / sqrt(total['offspring']))
    same['goffspring-rate-err'] = (
        sqrt(same['goffspring-rate2'] / total['goffspring']
             - same['goffspring-rate'] * same['goffspring-rate'])
        / sqrt(total['goffspring']))
    hybrid['goffspring-rate-err'] = (
        sqrt(hybrid['goffspring-rate2'] / total['goffspring']
             - hybrid['goffspring-rate'] * hybrid['goffspring-rate'])
        / sqrt(total['goffspring']))
    
    print_ratio(same, hybrid, 
                keys=['offspring-rate-err', 'goffspring-rate-err'],
                labels=['off', 'goff']) 

    return (same, hybrid)
   
def plot(data, key, filename="plot.png", func=lambda a: a.id, 
         plot_title='', cmap='Paired', filter=lambda a: True, 
         draw_legend=False):

    cmap = pylab.cm.jet
    error_key = key + '-err'

    lines=[]
    data = zip(*data)
    for i, series in enumerate(data):
        color = cmap(float(i) / len(data))
        
        line_data = [x[key] for x in series] 
        if error_key in series: 
            err_up = [x[key] + x[error_key] for x in series]
            err_down = [x[key] - x[error_key] for x in series]
            fill = pylab.fill_between(range(len(line_data)), err_up[:], err_down[:], 
                                      color=color, alpha=0.35, linewidth=0,
                                      edgecolor=None, facecolor=color)

        #pylab.plot(range(30000), err_up, alpha=0.35, color=color)
        #pylab.plot(range(30000), err_down, alpha=0.35, color=color)
        
        line = pylab.plot(range(len(line_data)), line_data, color=color)

        lines.append(line)

    pylab.title("gOffspring", weight='black')
    #pylab.ylim(0, 255)
    pylab.xlim(0, len(line_data))
    pylab.ylabel("gOffspring", weight='bold')
    pylab.xlabel("Time", weight='bold')
    pylab.legend(lines, ['same', 'hybrid'], loc='center right')
    #pylab.figtext(0,.948, '(d)', size=6, weight='black')
    pylab.savefig("plots/test.pdf", dpi=300)

def plot_ratio(data, key, filename="plot.png", func=lambda a: a.id, 
         plot_title='', cmap='Paired', filter=lambda a: True, 
         draw_legend=False):

    cmap = pylab.cm.jet
    error_key = key + '-err'

    color = cmap(0.5)

    line_data = []
    err_up = []
    err_down = []
    for same, hybrid in data:
        if hybrid[key]:
            line_data.append(same[key] / hybrid[key]) 
            if error_key in same:
                err_up.append((same[key] + same[error_key]) / 
                              (hybrid[key] - hybrid[error_key]))
                err_down.append((same[key] - same[error_key]) / 
                                (hybrid[key] + hybrid[error_key]))
        else:
            line_data.append(0.0)
            if error_key in same:
                err_up.append(0.0)
                err_down.append(0.0)

    if err_up and err_down:
        fill = pylab.fill_between(range(len(line_data)), err_up[:], err_down[:], 
                                  color=color, alpha=0.35, linewidth=0,
                                  edgecolor=None, facecolor=color)
    #pylab.plot(range(30000), err_up, alpha=0.35, color=color)
    #pylab.plot(range(30000), err_down, alpha=0.35, color=color)
        
    line = pylab.plot(range(len(line_data)), line_data, color=color)


    pylab.title("gOffspring", weight='black')
    pylab.ylim(0, 5)
    pylab.xlim(0, len(line_data))
    pylab.ylabel("gOffspring Ratio", weight='bold')
    pylab.xlabel("Time", weight='bold')
    #pylab.legend(lines, ['same', 'hybrid'], loc='center right')
    #pylab.figtext(0,.948, '(d)', size=6, weight='black')
    pylab.savefig("plots/test-ratio.pdf", dpi=300)

def print_ratio(same, hybrid, keys, labels=None):
    print "%10s %7s %7s %7s" % ( " ", "same", "hybrid", "ratio" )
    print_table("%10s %6.5f %6.5f %6.5f", same, hybrid, keys, labels,
                fn=lambda a,b: a / b if b else 0.0)

def print_float(same, hybrid, keys, labels=None):
    print "%10s %7s %7s %7s" % ( " ", "same", "hybrid", "total" )
    print_table("%10s %6.5f %6.5f %6.5f", same, hybrid, keys, labels)

def print_int(same, hybrid, keys, labels=None):
    print "%10s %7s %7s %7s" % ( " ", "same", "hybrid", "total" )
    print_table("%10s %7d %7d %7d", same, hybrid, keys, labels)

def print_table(fmt, same, hybrid, keys, labels=None, fn=None):
    if fn is None:
        fn = lambda a, b: a + b

    if labels is None:
        for key in keys:
            print fmt % (key, same[key], hybrid[key],
                         fn(same[key],  hybrid[key]))
    else:
        if len(labels) != len(keys):
            raise ValueError("labels and keys do not have same length.")
        for label, key in izip(labels, keys):
            print fmt % (label, same[key], hybrid[key],
                         fn(same[key], hybrid[key]))
    


if __name__ == '__main__':
    import sys
    
    cluster = rc.load(sys.argv[-1])
    n_clusters = len(set(cluster.values()))
    
    #pop = get_population_during_time(25000,29999)
    pop = get_population()
    population = len(pop)
    print n_clusters, "clusters", population, "critters"
    
    intra, inter = load('../run/events/contacts.log', cluster)
    
    data = []
    for i in range(0,30000, 1000):
        same, hybrid = contact_info(pop[:], cluster, start=i, stop=i+1000, 
                            intra=intra, inter=inter)
        data.append((same, hybrid))

    plot(data, 'goffspring-fecundity')
    #plot_ratio(data, 'goffspring-fecundity')
