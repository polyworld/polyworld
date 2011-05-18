import os.path
from numpy import array, matrix, vstack, zeros, ones
from lazy import Lazy

#############################
### BrainAnatomy
#############################
class BrainAnatomy:
    ''' Data structure to represent the brain anatomy '''

    def __init__(self, filename):
        self.filename = filename
        assert os.path.isfile(self.filename),\
                "Invalid brain anatomy file: %s" % self.filename

        # Open brain anatomy, clean lines
        f = open(filename)
        
        # grab header and perform sanity checks
        header = f.readline().strip('\m; ').split(' ')
        f.close()

        assert header[0] == 'brain', "invalid brain anatomy header."
        assert len(header) >= 9, "header should have at least 9 parts."
        
        # intialize variables
        self.id = header[1]

        header_dict = dict([x.split('=') for x in header[2:]])
        self.max_bias = float(header_dict['maxBias'])
        self.max_weight = float(header_dict['maxWeight'])
        self.num_neurons = int(header_dict['numneurons+1'])
        self.num_inputneurons = 1 + int(header_dict['blueinput'].split('-')[1])\
                                  - int(header_dict['redinput'].split('-')[0])

        # sanity check on variables
        assert all([self.id, self.max_bias, self.max_weight, self.num_neurons,
                    self.num_inputneurons])
        
    
    @Lazy
    def cxnmatrix(self):
        ''' Lazy loading of connection matrix '''
        # Open brain anatomy, clean lines
        f = open(filename)
        lines = [ x.strip('\n; ') for x in f.readlines()]
        lines = lines[1:]
        f.close()

        assert len(lines) == self.num_neurons, "#lines != num_neurons"

        # import all neurons and build the connection matrix with sanity checks
        cells = [ float(cell) for row in lines for cell in row.split(' ') ]
        assert len(cells) == self.num_neurons*self.num_neurons,\
                "Did not find a square matrix in anatomy file"

        cxn = array(cells).reshape(self.num_neurons, self.num_neurons)
        assert -self.max_weight <= self.cxn.all() <= self.max_weight,\
                "anatomy matrix wasn't within [-max_weight,max_weight]"
        
        
        return cxn


    def trace(self, start_nodes, max_distance, threshold=0.0):
        '''Returns all nodes within max_distance connections from start_nodes --
        going fowards.  Ignores all connections less than threshold'''        
        return self.k_distance_from( start_nodes, max_distance, 
                                     threshold, backtrace=False )

    def trace_back(self, start_nodes, max_distance, threshold=0.0):
        '''Returns all nodes within max_distance connections from start_nodes --
        going backwards.  Ignores all connections less than threshold'''
        return self.k_distance_from( start_nodes, max_distance, 
                                     threshold, backtrace=True )
    
    def k_distance_from(self, start_nodes, max_distance, threshold, backtrace):
        '''ignore all connections less than threshold'''
        assert max_distance >= 1, "max_distance must be >= 1"
        
        # if we passed a single int, make a list of it.
        if type(start_nodes) is int:
            start_nodes = [ start_nodes ]

        assert 0 <= min(start_nodes) <= max(start_nodes) <= self.num_neurons,\
                "start_nodes weren't within [0,num_neurons]"

        # build matrix of active neurons. This matrix contains only 0s and 1s.
        m = matrix( abs(self.cxnmatrix) > threshold, dtype=int )

        # if going backwards, invert the matrix
        if backtrace:
            m = m.T

        # create an empty vector, representing all neurons, and then flag the
        # starting neurons to use as in the vector transformation.
        v = zeros( (m.shape[0],1), dtype=int )
        v[start_nodes] = 1
        connected = set()

        for i in range(max_distance):
            # print "depth %s START:\t %s" % (i+1, v.T )
            v = m*v
            indices = v.nonzero()[0].tolist()[0]
            # print "depth %s END:\t %s -- adding indices=%s" \
            #           % (i+1, v.T, indices)

            # append to our unique set of connected_nodes
            connected.update(indices)

        # print "returning: ", connected 
        return sorted(connected)


##################################
### BrainFunction
##################################
class BrainFunction:
    ''' Data structure to represent the brain anatomy '''

    def __init__(self, filename):
        self.filename = filename
        assert os.path.isfile(self.filename),\
                "Invalid brain function file: %s" % self.filename

        # Open brain function, clean lines
        f = open(filename)
        lines = [ x.strip() for x in f.readlines()]
        f.close()
       
        # validate version number
        version = lines.pop(0).split(' ')[1]
        assert version == '1', "Only compatible with version 1 brainFunction"

        # validate brain function header
        header = lines.pop(0).split(' ')
        assert header[0] == 'brainFunction', "invalid brain function header."
        assert len(header) >= 9, "header should have at least 9 parts."

        #assert lines[-1].startswith('end fitness'),\
        #        "brainFunction file did not finish"
        if lines and lines[-1].startswith('end fitness'):
            self.agent_fitness = float(lines.pop().split('=')[-1].strip())
        else:
            self.agent_fitness = None
        
        # remove summary line, extract overall fitness

        # process header information
        self.num_inputneurons = int(header[3])
        self.agent_index = int(header[1])
        self.num_neurons = int(header[2])
        self.num_synapes = int(header[4])
        self.timestep_born = int(header[5])

        self.neurons = {}
        Rstart, Rend = map(int, header[7].split('-'))
        self.neurons['red'] = range(Rstart,Rend+1)

        Gstart, Gend = map(int, header[8].split('-'))
        self.neurons['green'] = range(Gstart,Gend+1)

        Bstart, Bend = map(int, header[9].split('-'))
        self.neurons['blue'] = range(Bstart,Bend+1)
    
        self.timesteps_lived = int(len(lines) / self.num_neurons)
        
        ##############################################
        # define the neural groups
        ##############################################
        
        ALL_NEURONS = range( self.num_neurons )
        INPUT_NEURONS = range( self.num_inputneurons )

        PROCESSING_NEURONS = list(set(ALL_NEURONS) - set(INPUT_NEURONS))
        BEHAVIOR_NEURONS = range( self.num_neurons-7, self.num_neurons )
        INTERNAL_NEURONS = list(set(PROCESSING_NEURONS) - set(BEHAVIOR_NEURONS))
        assert len(BEHAVIOR_NEURONS) == 7, "There wasn't 7 behavior neurons"

        self.neurons['input'] = INPUT_NEURONS
        self.neurons['behavior'] = BEHAVIOR_NEURONS
        self.neurons['processing'] = PROCESSING_NEURONS
        self.neurons['internal'] = INTERNAL_NEURONS

        assert len(lines) % self.num_neurons == 0,\
                "#timesteps lived not evenly divisible by #neurons"
        
        




    ###########################################
    # Lazy loading of activations
    ###########################################
    @Lazy
    def acts(self):
        print "calculating activations"
        
        # Open brain function, clean lines
        f = open(self.filename)
        lines = [ x.strip() for x in f.xreadlines()]
        lines = lines[2:-1]
        f.close()

        activations = [ [] for i in xrange(self.num_neurons) ]
        
        for line in lines:
            parts = line.split(' ')
            assert len(parts) == 2, "function line was not in (neuron, act)"
            neuron, act = parts
            activations[ int(neuron) ].append( float(act) )

        # convert to numpy arrays
        activations = array( activations )
    
        numrows, numcols = activations.shape
        assert numrows == self.num_neurons, "#rows != num_neurons"
        assert numcols == self.timesteps_lived, "#cols != timesteps lived"
    
        # sanity check the activations == they're all within [0,1] ?
        assert 0.0 <= activations.all() <= 1.0, "acts had values not within [0.0,1.0]"

        

        ########################################################
        # NOW WE ACCOUNT FOR THE BIAS NODE.  WE DO THIS BY:
        #   1) Adding a key 'bias' to the self.neurons dictionary
        #    2) numneurons += 1
        #    3) adding a ROW of all 1.0's to end of the activity matrix
        ########################################################
        self.neurons['bias'] = max(self.neurons['behavior'])+1
        self.num_neurons += 1
        #print activations.shape
        # the bias node is always 1.0
        bias_acts = ones( (1,self.timesteps_lived) )
        activations = vstack( (activations, bias_acts) )

        #print activations.shape
        return activations
        

    def binarize_neurons( self, neurons ):
        '''binarizes the list of neurons'''
        for i in sorted(neurons):
            assert 0 <= i < self.acts.shape[0], "i wasn't within range of the neurons"
            self.acts[i] = round( self.acts[i] )


    def print_statistics( self ):
        '''print statistics about self.acts'''
        numrows, numcols = self.acts.shape
        assert numrows == self.num_neurons, "#cols wasn't same as num_neurons"

        print "input:", self.neurons['input']
        if self.neurons['internal']:
            print "internal", self.neurons['internal']
        else:
            print "internal: None"
        print "behavior:", self.neurons['behavior']

        means, variances = self.acts.mean(axis=1), self.acts.var(axis=1)
        num_entries = [ len(self.acts[i,:]) for i in range(self.acts.shape[0]) ]

        print "neuron statistics:"
        print ''
        for i, (m, v, n) in enumerate(zip(means,variances,num_entries)):
            
            print "neuron=%s \t mean=%.4f \t var=%.4f \t samples=%s" % ( i, m, v, n )
        
        print "acts.shape=", self.acts.shape
            
    def write_to_Rfile(self, output_filename, labels=None ):        
        assert self.num_neurons == self.acts.shape[0], "num_neurons didn't match the acts matrix!"
        assert self.timesteps_lived == self.acts.shape[1], "timesteps_lived didn't match the acts matrix!"

        temp = self.acts.T
        numrows, numcols = temp.shape

        f = open( output_filename, 'w' )

        if labels:
            assert len(labels) == numcols, "not same number of labels and columns"
            labels_line = ','.join([ str(x) for x in labels])
            f.write( labels_line + '\n' )

        for t in temp:
            line = ','.join( [ '%.5f' % x for x in t.tolist() ] )
    #        print 'line=', line
            f.write( line + '\n' )

        f.close()
        print "wrote to file '%s'" % output_filename
