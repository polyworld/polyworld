import os.path
import datalib
from brain import BrainAnatomy, BrainFunction
from genome import Genome
from motion import Positions
from numpy import sum, zeros
from lazy import Lazy
from collections import defaultdict
from reader import BackwardsReader

# dictionary objects to hold the lifespan and complexity data tables for lazy
# loading. Need to create a lazy dictionary to handle this case better
# Also need to examine parallelism
# TODO: LazyDict
lifespans = {}
complexity = {}
parents = {}
children = {}

def load_birthdeath(filepath):
    parents[filepath] = {}
    children[filepath] = defaultdict(list)
    f=open(filepath)
    for line in f:
        timestep, event, agents = line.split(' ', 2)
        if event == 'BIRTH':
            agent, p1, p2 = agents.split()
            parents[filepath][int(agent)] = [int(p1), int(p2)]
            children[filepath][int(p1)].append(int(agent))
            children[filepath][int(p2)].append(int(agent))
    f.close()



class Agent(object):
    agents = defaultdict(lambda: defaultdict(lambda: None))

    ''' Represents a single Polyworld agent, loading in lifespan, lineage,
    neural anatomy, function, genome, and movement. Lazily loads the larger
    files. Intiializes with a run directory and an agent ID, building the 
    filepaths on the fly.'''
    def __hash__(self):
        return self.id

    def __new__(cls, id, run_dir='../run/', *args, **kwargs):
        if not Agent.agents[run_dir][id]:
            Agent.agents[run_dir][id] = super(Agent, cls).__new__(cls) 
        return Agent.agents[run_dir][id]

    def __init__(self, id, run_dir='../run/', load_anat=False):
        self.id = id

        # Construct file paths:
        self.run_dir = run_dir
        self.lifespans_filename = "%s/lifespans.txt" % (run_dir)
        assert os.path.isfile(self.lifespans_filename),\
            "Lifespans file required"
        
        self.genome_filename = "%s/genome/genome_%d.txt" % (run_dir, id)
        self.positions_filename = "%s/motion/position/position_%d.txt"\
                                % (run_dir, id)

        self.anat_filename = {}
        self.anat_filename['birth'] = "%s/brain/anatomy/brainAnatomy_%d_birth.txt"\
                                % (run_dir, id)
        self.anat_filename['incept'] = "%s/brain/anatomy/brainAnatomy_%d_incept.txt"\
                                % (run_dir, id)
        self.anat_filename['death'] = "%s/brain/anatomy/brainAnatomy_%d_death.txt"\
                                % (run_dir, id)

        self.birthdeath_filename = "%s/BirthsDeaths.log" % run_dir

        self.func_filename = "%s/brain/function/brainFunction_%d.txt"\
                                % (run_dir, id)
        if not os.path.isfile(self.func_filename):
            self.func_filename = os.path.join(run_dir + "/brain/function/",\
                                    "incomplete_brainFunction_%d.txt" % id)

        # intialize lifespans -- need to move this to new LazyDict class.
        # TODO: LazyDict
        if self.lifespans_filename not in lifespans:
            lifespans[self.lifespans_filename] =\
                datalib.parse(self.lifespans_filename,
                              keycolname='Agent')['LifeSpans']
        lifespan_table = lifespans[self.lifespans_filename]

        # grab birth and death information
        self.birth = lifespan_table[id]['BirthStep']
        self.death = lifespan_table[id]['DeathStep']
        self.birth_reason = lifespan_table[id]['BirthReason']
        self.death_reason = lifespan_table[id]['DeathReason']
        self.lifespan = self.death - self.birth

        self.complexity_filename = "%s/brain/Recent/%d/complexity_P.plt"\
                                % (run_dir, (((self.death-1) / 1000 ) + 1) * 1000)


        # Validate file paths and load files - once LazyDict is written, this
        # will become much cleaner:
        self.anat = {} 
        if load_anat and os.path.isfile(self.anat_filename['birth']):
            self.anat['birth'] = BrainAnatomy(self.anat_filename['birth'])
        else:
            self.anat_filename['birth'] = None
            self.anat['birth'] = None

        if load_anat and os.path.isfile(self.anat_filename['incept']):
            self.anat['incept'] = BrainAnatomy(self.anat_filename['incept'])
        else:
            self.anat_filename['incept'] = None
            self.anat['incept'] = None
        
        if load_anat and os.path.isfile(self.anat_filename['death']):
            self.anat['death'] = BrainAnatomy(self.anat_filename['death'])
        else:
            self.anat_filename['death'] = None
            self.anat['death'] = None

        if self.birthdeath_filename not in parents:
            load_birthdeath(self.birthdeath_filename)
       
        if self.birth_reason == 'NATURAL':
            self.parent1 = parents[self.birthdeath_filename][self.id][0]
            self.parent2 = parents[self.birthdeath_filename][self.id][1]
        else:
            self.parent1 = None
            self.parent2 = None
       

        self.children = children[self.birthdeath_filename][self.id]
        
    def __repr__(self):
        ''' string representation of agent '''
        return "<Agent %d (%d-%d: %d steps)>"\
                % (self.id, self.birth, self.death, self.lifespan)




    #######################
    ### LAZY PROPERTIES ###
    #######################

    @Lazy
    def complexity(self):
        ''' Lazy loading of complexity file '''
        return self._get_complexity()

    def _get_complexity(self):
        if self.death_reason == 'NATURAL':
            # TODO: LazyDict
            if self.complexity_filename not in complexity:
                complexity[self.complexity_filename] =\
                    datalib.parse(self.complexity_filename,
                                  keycolname='CritterNumber')['P']
            complexity_table = complexity[self.complexity_filename]
    
            return complexity_table[self.id]['Complexity']
        else:
            return 0.0 

    @Lazy
    def genome(self):
        ''' Lazy loading of genome file '''
        return self._get_genome()

    def _get_genome(self):
        if os.path.isfile(self.genome_filename):
            return Genome(self.genome_filename)
        else:
            return None
    
    @Lazy
    def positions(self):
        ''' Lazy loading of positions file'''
        return self._get_positions()

    def _get_positions(self):
        if os.path.isfile(self.positions_filename):
            return Positions(self.positions_filename)
        else:
            return None
    
    @Lazy
    def func(self):
        ''' Lazy loading of brain function file '''
        return self._get_func()

    def _get_func(self):
        if os.path.isfile(self.positions_filename):
            return BrainFunction(self.func_filename)
        else:
            return None
    
    @Lazy
    def fitness(self):
        ''' Lazy loading of genome file '''
        return self._get_fitness()

    def _get_fitness(self):
        if os.path.isfile(self.func_filename):
            fitness_file = BackwardsReader(self.func_filename)
            last_line = fitness_file.readline()
            fitness_file.close()

            if last_line.startswith('end fitness'):
                return float(last_line.split()[-1])
            else:
                return None
        else:
            return None
    
    ###########################
    ### END LAZY PROPERTIES ###
    ###########################


    
    #############################
    ### RESET LAZY PROPERTIES ###
    #############################
    def reset(self):
        ''' function to reset lazily loaded data - was used for some memory
        allocation optimizations. Ran into trouble with the Python GIL. Contact
        Jaimie with questions. '''
        self._reset_complexity()
        self._reset_genome()
        self._reset_func()
        self._reset_positions()

    def _reset_complexity(self):
        if 'complexity' in self.__dict__:
            del self.complexity
        complexity = Lazy(Agent._get_complexity)
    
    def _reset_genome(self):
        if 'genome' in self.__dict__:
            del self.genome
        genome = Lazy(Agent._get_genome)
    
    def _reset_positions(self):
        if 'positions' in self.__dict__:
            del self.positions
        positions = Lazy(Agent._get_positions)

    def _reset_func(self):
        if 'func' in self.__dict__:
            del self.func
        func = Lazy(Agent._get_func)

    ###########################
    ### END LAZY PROPERTIES ###
    ###########################



    def alive_at_timestep(self, step):
        ''' returns whether the Agent is alive at a given timestep '''
        return self.birth < step <= self.death
    
    def context_network( self, behavior_node, numsteps ):
        ''' returns all nodes within numsteps of a given behavior node '''

        assert behavior_node in self.func.neurons['behavior'],\
                "passed node wasn't in behavior nodes!"

        context_nodes = self.anat.trace_back( behavior_node, numsteps )
        print "CN nodes=%s" % context_nodes
        
        context_network = []
        for n in context_nodes:
            this_cn = [ (i,n) for i in context_nodes 
                                    if self.anat.cxnmatrix[n][i] ]
        #    print "Found connections", this_cn 
            context_network.extend( this_cn )

        # return all of the unique pairs of the context_network
        return sorted(list(set(context_network)))
    
    def reference_time( self, behavior_node, window_size, start_at ):
        ''' returns the most time at which the agent's neural activity was the
        most active. '''
        
        assert window_size >= 1, "window size must be >= 1"
        assert start_at >= 20,\
            "You want start_at to be at least 20 for decent sampling"
        assert behavior_node in self.func.neurons['behavior'],\
            "behavior_node wasn't in the behvior nodes!"
        
        if start_at + window_size > self.lifespan:
            return None
        
#        print self.func.acts.shape
#        print self.func.acts[behavior_node,:]
#        print len(self.func.acts[behavior_node,:])
#        raw_input('...')
        
        sum_acts = {}
        for t in range( start_at, self.func.timesteps_lived - window_size ):
            this_window = self.func.acts[behavior_node,t:t+window_size]
            sum_acts[t] = sum(this_window)
        
#        print "sum_acts=%s" % sum_acts
        
        # which t's had the largest sum_acts ?
        most_active = max( sum_acts.values() )
        most_active_times = [ int(key) for key, value in sum_acts.iteritems()
                                                      if value == most_active ]
        
        # return the latest most_active_time
        return max( most_active_times )






#################################
### Population building tools ###
#################################
def _file_len(fname):
    ''' grabsline length length '''
    with open(fname) as f:
        for i, l in enumerate(f):
            pass
    return i + 1

'''
from multiprocessing.managers import BaseManager
class AgentManager(BaseManager): pass
AgentManager.register("Agent", Agent)
'''

from multiprocessing import Pool
# workaround for Pool.map_async...
def _wrap_Agent(a):
    ''' wrapper function to enable use of asyncronous map '''
    return Agent(*a)



def get_agents(ids, run_dir='../run/', complexity_filter=False):
    '''grab an agent with the given run directory. If complexity_filter = True,
    then only those agents with a complexity value are returned.''' 
    rd = [run_dir  for i in xrange(len(ids))]
    '''
    p = Pool()
    result = p.map_async(_wrap_Agent, zip(ids, rd))
    result = result.get()
    p.close()
    '''

    result = map(Agent, ids, rd)

    # filter by those agents with a complexity value
    if complexity_filter:
        result = [a for a in result if a.complexity != None]

    return result



def get_population(start=1, stop=None, run_dir='../run/'):
    ''' return all agents with start <= id <= stop  from a given run_dir'''
    if not stop:
        stop = _file_len(os.path.join(run_dir, 'lifespans.txt'))-15
    assert start <= stop, "stop must be greater than the start"

    return get_agents(range(start,stop))
    #return [Agent(i, run_dir) for i in xrange(start,stop)]



def get_agent_ids_at_time(time, run_dir='../run/'):
    ''' get agent ids at timestep time. This is used to build an arg for get_agents. '''

    lifespans_filename = "%s/lifespans.txt" % (run_dir)
    if lifespans_filename not in lifespans:
        lifespans[lifespans_filename] =\
            datalib.parse(lifespans_filename,
                          keycolname='Agent')['LifeSpans']
    lifespan_table = lifespans[lifespans_filename]
    
    alive = []
    for row in lifespan_table.rows():
        if (row['BirthStep'] <= time) and (row['DeathStep'] > time):
            alive.append(row['Agent']) 
    
    return alive

def get_population_at_time(time, run_dir='../run/'):
    ''' get all agents at timestep time '''
    alive = get_agent_ids_at_time(time, run_dir)
    return get_agents(alive)

    #return [Agent(i, run_dir) for i in alive]

def get_agent_ids_during_time(start, stop, run_dir='../run/'):
    ''' get all agent ids alive between start and stop '''
    lifespans_filename = "%s/lifespans.txt" % (run_dir)
    if lifespans_filename not in lifespans:
        lifespans[lifespans_filename] =\
            datalib.parse(lifespans_filename,
                          keycolname='Agent')['LifeSpans']
    lifespan_table = lifespans[lifespans_filename]
    
    alive = []
    for row in lifespan_table.rows():
        if (start <= row['DeathStep']) and (row['BirthStep'] <= stop):
            alive.append(row['Agent']) 
    
    return alive

def get_population_during_time(start, stop, run_dir='../run/'):
    ''' get all agents alive between start and stop '''
    alive = get_agent_ids_during_time(start, stop, run_dir)
    return get_agents(alive)


def get_agent_ids_until_time(time, run_dir='../run/'):
    ''' get all agent ids alive before timestep time. '''
    lifespans_filename = "%s/lifespans.txt" % (run_dir)
    if lifespans_filename not in lifespans:
        lifespans[lifespans_filename] =\
            datalib.parse(lifespans_filename,
                          keycolname='Agent')['LifeSpans']
    lifespan_table = lifespans[lifespans_filename]
    
    alive = []
    for row in lifespan_table.rows():
        if (row['BirthStep'] < time):
            alive.append(row['Agent']) 
    
    return alive

def get_population_until_time(time, run_dir='../run/'):
    ''' get all agents alive before timestep time '''
    alive = get_agent_ids_until_time(time, run_dir)
    
    return get_agents(alive)
    #return [Agent(i, run_dir) for i in alive]



def first_appearance(population):
    ''' given a population, return the first birth of the population. For use
    with clustering data. '''
    births = [a.birth for a in population]
    return min(births)

def average_step(population):
    ''' Calculates the peak timestep of the population. '''
    steps = []
    for a in population:
        steps.extend(range(a.birth, a.death))

    return sum(steps) / float(len(steps))

def average_genome(population):
    total = zeros(len(population[0].genome))
    for agent in population:
        total += agent.genome[:]

    return total / len(population)

