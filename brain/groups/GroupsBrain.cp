#include "GroupsBrain.h"

#include "agent.h"
#include "error.h"
#include "FiringRateModel.h"
#include "globals.h"
#include "GroupsGenome.h"
#include "GroupsNeuralNetRenderer.h"
#include "NervousSystem.h"
#include "RandomNumberGenerator.h"
#include "SpikingModel.h"

using namespace genome;

static float initminweight = 0.0; // could read this in

#define IsInputNeuralGroup( group ) ( _genome->getSchema()->getNeurGroupType(group) == NGT_INPUT )
#define IsOutputNeuralGroup( group ) ( _genome->getSchema()->getNeurGroupType(group) == NGT_OUTPUT )
#define IsInternalNeuralGroup( group ) ( _genome->getSchema()->getNeurGroupType(group) == NGT_INTERNAL )


#define __ALLOC_STACK_BUFFER(NAME, TYPE, N) TYPE *NAME = (TYPE *)alloca( N * sizeof(TYPE) ); Q_CHECK_PTR(NAME)
#define ALLOC_STACK_BUFFER(NAME, TYPE) __ALLOC_STACK_BUFFER( NAME, TYPE, __numgroups )

#define ALLOC_GROW_STACK_BUFFERS()										\
	int __numgroups = _genome->getGroupCount(NGT_ANY);			\
	ALLOC_STACK_BUFFER( firsteneur, short );							\
	ALLOC_STACK_BUFFER( firstineur, short );							\
	ALLOC_STACK_BUFFER( eeremainder, float );							\
	ALLOC_STACK_BUFFER( eiremainder, float );							\
	ALLOC_STACK_BUFFER( iiremainder, float );							\
	ALLOC_STACK_BUFFER( ieremainder, float );							\
	__ALLOC_STACK_BUFFER( neurused,										\
						  bool,											\
						  max((int)_genome->get("ExcitatoryNeuronCount_max"), \
							  (int)_genome->get("InhibitoryNeuronCount_max")) )


GroupsBrain::Configuration GroupsBrain::config;


//---------------------------------------------------------------------------
// GroupsBrain::processWorldfile
//---------------------------------------------------------------------------
void GroupsBrain::processWorldfile( proplib::Document &doc )
{
    GroupsBrain::config.minvisneurpergroup = doc.get( "MinVisionNeuronsPerGroup" );
    GroupsBrain::config.maxvisneurpergroup = doc.get( "MaxVisionNeuronsPerGroup" );
    GroupsBrain::config.mininternalneurgroups = doc.get( "MinInternalNeuralGroups" );
    GroupsBrain::config.maxinternalneurgroups = doc.get( "MaxInternalNeuralGroups" );
    GroupsBrain::config.mineneurpergroup = doc.get( "MinExcitatoryNeuronsPerGroup" );
    GroupsBrain::config.maxeneurpergroup = doc.get( "MaxExcitatoryNeuronsPerGroup" );
    GroupsBrain::config.minineurpergroup = doc.get( "MinInhibitoryNeuronsPerGroup" );
    GroupsBrain::config.maxineurpergroup = doc.get( "MaxInhibitoryNeuronsPerGroup" );
    GroupsBrain::config.minconnectiondensity = doc.get( "MinConnectionDensity" );
    GroupsBrain::config.maxconnectiondensity = doc.get( "MaxConnectionDensity" );
    GroupsBrain::config.mintopologicaldistortion = doc.get( "MinTopologicalDistortion" );
    GroupsBrain::config.maxtopologicaldistortion = doc.get( "MaxTopologicalDistortion" );
	GroupsBrain::config.enableTopologicalDistortionRngSeed = doc.get( "EnableTopologicalDistortionRngSeed" );
	GroupsBrain::config.minTopologicalDistortionRngSeed = doc.get( "MinTopologicalDistortionRngSeed" );
	GroupsBrain::config.maxTopologicalDistortionRngSeed = doc.get( "MaxTopologicalDistortionRngSeed" );
	GroupsBrain::config.enableInitWeightRngSeed = doc.get( "EnableInitWeightRngSeed" );
	GroupsBrain::config.minInitWeightRngSeed = doc.get( "MinInitWeightRngSeed" );
	GroupsBrain::config.maxInitWeightRngSeed = doc.get( "MaxInitWeightRngSeed" );
}

//---------------------------------------------------------------------------
// GroupsBrain::init
//---------------------------------------------------------------------------
void GroupsBrain::init()
{
	RandomNumberGenerator::set( RandomNumberGenerator::TOPOLOGICAL_DISTORTION,
								RandomNumberGenerator::LOCAL );
	RandomNumberGenerator::set( RandomNumberGenerator::INIT_WEIGHT,
								RandomNumberGenerator::LOCAL );

	int numinputneurgroups = 5;
	if( agent::config.enableMateWaitFeedback )
		numinputneurgroups++;
	if( agent::config.enableSpeedFeedback )
		numinputneurgroups++;
	if( agent::config.enableCarry )
		numinputneurgroups += 2;
	config.numinputneurgroups = numinputneurgroups;

	int numoutneurgroups = 7;
	if( agent::config.enableGive )
		numoutneurgroups++;
	if( agent::config.enableCarry )
		numoutneurgroups += 2;
	config.numoutneurgroups = numoutneurgroups;

    config.maxnoninputneurgroups = config.maxinternalneurgroups + config.numoutneurgroups;
    config.maxneurgroups = config.maxnoninputneurgroups + config.numinputneurgroups;
    config.maxneurpergroup = config.maxeneurpergroup + config.maxineurpergroup;
    config.maxinternalneurons = config.maxneurpergroup * config.maxinternalneurgroups;
    config.maxinputneurons = GroupsBrain::config.maxvisneurpergroup * 3 + (numinputneurgroups - 1);
    config.maxnoninputneurons = config.maxinternalneurons + config.numoutneurgroups;
    config.maxneurons = config.maxinternalneurons + config.maxinputneurons + config.numoutneurgroups;

    // the 2's are due to the input & output neurons
    //     doubling as e & i presynaptically
    // the 3's are due to the output neurons also acting as e-neurons
    //     postsynaptically, accepting internal connections
    // the -'s are due to the output & internal neurons not self-stimulating
    config.maxsynapses = config.maxinternalneurons * config.maxinternalneurons 	// internal
                + 2 * config.numoutneurgroups * config.numoutneurgroups					// output
                + 3 * config.maxinternalneurons * config.numoutneurgroups       			// internal/output
                + 2 * config.maxinternalneurons * config.maxinputneurons  				// internal/input
                + 2 * config.maxinputneurons * config.numoutneurgroups       				// input/output
                - 2 * config.numoutneurgroups													// output/output
                - config.maxinternalneurons;                   									// internal/internal
}

//---------------------------------------------------------------------------
// GroupsBrain::GroupsBrain
//---------------------------------------------------------------------------
GroupsBrain::GroupsBrain( NervousSystem *cns )
: Brain( cns )
, _genome(NULL)
, _numgroups(0)
, _numgroupsWithNeurons(0)
{
}

//---------------------------------------------------------------------------
// GroupsBrain::~GroupsBrain
//---------------------------------------------------------------------------
GroupsBrain::~GroupsBrain()
{
}

//---------------------------------------------------------------------------
// GroupsBrain::grow
//---------------------------------------------------------------------------
void GroupsBrain::grow( Genome *genome )
{
	_genome = dynamic_cast<GroupsGenome *>(genome);
	grow();
}

//---------------------------------------------------------------------------
// GroupsBrain::initNeuralNet
//---------------------------------------------------------------------------
void GroupsBrain::initNeuralNet( float initial_activation )
{
	switch( Brain::config.neuronModel )
	{
	case Brain::Configuration::SPIKING:
		{
			SpikingModel *spiking = new SpikingModel( _cns,
													  _genome->get("ScaleLatestSpikes") );
			_neuralnet = spiking;
			_renderer = new GroupsNeuralNetRenderer<SpikingModel>( spiking, _genome );
		}
		break;
	case Brain::Configuration::FIRING_RATE:
	case Brain::Configuration::TAU:
		{
			FiringRateModel *firingRate = new FiringRateModel( _cns );
			_neuralnet = firingRate;
			_renderer = new GroupsNeuralNetRenderer<FiringRateModel>( firingRate, _genome );
		}
		break;
	default:
		assert(false);
	}

	_neuralnet->init( &_dims, initial_activation );
}

//---------------------------------------------------------------------------
// GroupsBrain::nearestFreeNeuron
//---------------------------------------------------------------------------
short GroupsBrain::nearestFreeNeuron(short iin, bool* used, short num, short exclude)
{
    short iout;
    bool tideishigh;
    short hitide = iin;
    short lotide = iin;

    if (iin < (num-1))
    {
        iout = iin + 1;
        tideishigh = true;
    }
    else
    {
        iout = iin - 1;
        tideishigh = false;
    }

    while (used[iout] || (iout == exclude))
    {
        if (tideishigh)
        {
            hitide = iout;
            if (lotide > 0)
            {
                iout = lotide - 1;
                tideishigh = false;
            }
            else if (hitide < (num-1))
                iout++;
        }
        else
        {
            lotide = iout;
            if (hitide < (num-1))
            {
                iout = hitide + 1;
                tideishigh = true;
            }
            else if (lotide > 0)
                iout--;
        }

        if ((lotide == 0) && (hitide == (num-1)))
            error(2,"brain::nearestfreeneuron search failed");
    }

    return iout;
}

//---------------------------------------------------------------------------
// Brain::grow
//---------------------------------------------------------------------------
void GroupsBrain::grow()
{
    debugcheck( "(brain) on entry" );

	ALLOC_GROW_STACK_BUFFERS();
	
    _numgroups = _genome->getGroupCount(NGT_ANY);
	_numgroupsWithNeurons = 0;
	for( int i = 0; i < _numgroups; i++ )
	{
		if( _genome->getNeuronCount(i) > 0 )
		{
			_numgroupsWithNeurons++;
		}
	}

#if DebugBrainGrow
	if( DebugBrainGrowPrint )
	{
		cout << "****************************************************" nlf;
		cout << "Starting a new brain with numneurgroups = " << _numgroups nlf;
	}
#endif

	_dims.numInputNeurons = 0;
	_dims.numNonInputNeurons = 0;

	// ---
	// --- Configure Input/Output Neurons
	// ---
	int index = 0;
	for( NervousSystem::nerve_iterator
			 it = _cns->begin_nerve(),
			 end = _cns->end_nerve();
		 it != end;
		 it++ )
	{
		Nerve *nerve = *it;

		int numneurons = _genome->getNeuronCount( EXCITATORY, nerve->igroup );
		nerve->config( numneurons, index );

        firsteneur[nerve->igroup] = index;
        firstineur[nerve->igroup] = index; // input/output neurons double as e & i

		if( nerve->type == Nerve::INPUT )
		{
			_dims.numInputNeurons += numneurons;
		}
		else
		{
			_dims.numNonInputNeurons += numneurons;
		}

		index += numneurons;

#if DebugBrainGrow
		if( DebugBrainGrowPrint )
			cout << "group " << nerve->igroup << " (" << nerve->name << ") has " << nerve->getNeuronCount() << " neurons" nlf;
#endif
	}

#if DebugBrainGrow
	if( DebugBrainGrowPrint )
		cout << "numinputneurons = " << _dims.numInputNeurons nlf;
#endif
    _dims.firstNonInputNeuron = _dims.numInputNeurons;

	_dims.numOutputNeurons = _cns->getNeuronCount( Nerve::OUTPUT );

	_dims.firstOutputNeuron = _cns->get( Nerve::OUTPUT, 0 )->getIndex();
	_dims.firstInternalNeuron = _dims.firstOutputNeuron + _dims.numOutputNeurons;

	// ---
	// --- Configure Internal Groups
	// ---
    for( int i = _cns->getNerveCount();
		 i < _numgroups;
		 i++ )
    {
		firsteneur[i] = _dims.numInputNeurons + _dims.numNonInputNeurons;
		_dims.numNonInputNeurons += _genome->getNeuronCount(EXCITATORY, i);
		firstineur[i] = _dims.numInputNeurons + _dims.numNonInputNeurons;
		_dims.numNonInputNeurons += _genome->getNeuronCount(INHIBITORY, i);

#if DebugBrainGrow
		if( DebugBrainGrowPrint )
		{
			cout << "group " << i << " has " << _genome->getNeuronCount(EXCITATORY, i) << " e-neurons" nlf;
			cout << "  and " << i << " has " << _genome->getNeuronCount(INHIBITORY, i) << " i-neurons" nlf;
		}
#endif
	}

#if DebugBrainGrow
	if( DebugBrainGrowPrint )
	{
		cout << "fNumRedNeurons, fNumGreenNeurons, fNumBlueNeurons = "
			 << _cns->get("red")->getNeuronCount() cms _cns->get("green")->getNeuronCount() cms _cns->get("blue")->getNeuronCount() nlf;;
	}
#endif

	// ---
	// --- Count Synapses
	// ---
    _dims.numsynapses = 0;

    for( int i = _cns->getNerveCount( Nerve::INPUT );
		 i < _numgroups;
		 i++ )
    {
#if DebugBrainGrow
		if( DebugBrainGrowPrint )
		{
			cout << "group " << i << " has " << _genome->getNeuronCount(EXCITATORY, i) << " e-neurons" nlf;
			cout << "  and " << i << " has " << _genome->getNeuronCount(INHIBITORY, i) << " i-neurons" nlf;
		}
#endif
        for (int j = 0; j < _numgroups; j++)
        {
            _dims.numsynapses += _genome->getSynapseCount(j,i);

#if DebugBrainGrow
			if( DebugBrainGrowPrint )
			{
				cout << "  from " << j << " to " << i << " there are "
					 << _genome->getSynapseCount(_genome->EE,j,i) << " e-e synapses" nlf;
				cout << "  from " << j << " to " << i << " there are "
					 << _genome->getSynapseCount(_genome->IE,j,i) << " i-e synapses" nlf;
				cout << "  from " << j << " to " << i << " there are "
					 << _genome->getSynapseCount(_genome->EI,j,i) << " e-i synapses" nlf;
				cout << "  from " << j << " to " << i << " there are "
					 << _genome->getSynapseCount(_genome->II,j,i) << " i-i synapses" nlf;
				cout << "  from " << j << " to " << i << " there are "
					 << _genome->getSynapseCount(j,i) << " total synapses" nlf;
			}
#endif

        }
    }
    
    _dims.numneurons = _dims.numNonInputNeurons + _dims.numInputNeurons;
	if( _dims.numneurons > config.maxneurons )
		error( 2, "numneurons (", _dims.numneurons, ") > maxneurons (", config.maxneurons, ") in brain::grow" );
        
	if( _dims.numsynapses > config.maxsynapses )
		error( 2, "numsynapses (", _dims.numsynapses, ") > maxsynapses (", config.maxsynapses, ") in brain::grow" );

#if DebugBrainGrow
	if( DebugBrainGrowPrint )
	{
		cout << "numneurons = " << _dims.numneurons << "  (of " << config.maxneurons pnlf;
		cout << "numsynapses = " << _dims.numsynapses << "  (of " << config.maxsynapses pnlf;
	}
#endif

	// ---
	// --- Allocate Neural Net
	// ---
    debugcheck( "before allocating brain memory" );

	initNeuralNet( 0.1 );	// lsy? - why is this initializing activations to 0.1?

    debugcheck( "after allocating brain memory" );

    long numsyn = 0;
    short numneur = _dims.numInputNeurons;

#if DebugBrainGrow
	if( DebugBrainGrowPrint )
		cout << "  Initializing input neurons:" nlf;
#endif

	// ---
	// --- Create NeuronModel-specific neuron attributes struct
	// ---
	union
	{
		void *opaque;
		SpikingModel__NeuronAttrs *spiking;
		FiringRateModel__NeuronAttrs *firingRate;
	} neuronAttrs;

	switch( Brain::config.neuronModel )
	{
	case Brain::Configuration::SPIKING:
		neuronAttrs.spiking = (SpikingModel__NeuronAttrs *)alloca( sizeof(SpikingModel__NeuronAttrs) );
		break;
	case Brain::Configuration::FIRING_RATE:
	case Brain::Configuration::TAU:
		neuronAttrs.firingRate = (FiringRateModel__NeuronAttrs *)alloca( sizeof(FiringRateModel__NeuronAttrs) );
		break;
	default:
		assert(false);
	}

	// ---
	// --- Initialize Input Neuron Activations
	// ---
	switch( Brain::config.neuronModel )
	{
	case Brain::Configuration::SPIKING:
		neuronAttrs.spiking->SpikingParameter_a = 0;
		neuronAttrs.spiking->SpikingParameter_b = 0;
		neuronAttrs.spiking->SpikingParameter_c = 0;
		neuronAttrs.spiking->SpikingParameter_d = 0;
		neuronAttrs.spiking->bias = 0.0;
		break;
	case Brain::Configuration::FIRING_RATE:
	case Brain::Configuration::TAU:
		neuronAttrs.firingRate->tau = 0.0;
		neuronAttrs.firingRate->bias = 0.0;
		break;
	default:
		assert(false);
	}

    for (int i = 0, ineur = 0; i < config.numinputneurgroups; i++)
    {
        for (int j = 0; j < _genome->getNeuronCount(EXCITATORY, i); j++, ineur++)
        {
			switch( Brain::config.neuronModel )
			{
			case Brain::Configuration::SPIKING:
				neuronAttrs.spiking->group = i;
					break;
			case Brain::Configuration::FIRING_RATE:
			case Brain::Configuration::TAU:
				neuronAttrs.firingRate->group = i;
				break;
			default:
				assert(false);
			}

			_neuralnet->set_neuron( ineur, neuronAttrs.opaque );
        }
    }

	// ---
	// --- Grow Synapses
	// ---
    for (int groupIndex_to = config.numinputneurgroups; groupIndex_to < _numgroups; groupIndex_to++)
    {
#if DebugBrainGrow
		if( DebugBrainGrowPrint )
			cout << "For group " << groupIndex_to << ":" nlf;
#endif

		switch( Brain::config.neuronModel )
		{
		case Brain::Configuration::SPIKING:
			neuronAttrs.spiking->group = groupIndex_to;
			neuronAttrs.spiking->bias = _genome->get(_genome->BIAS,groupIndex_to);
			if(Brain::config.Spiking.enableGenes) {
				neuronAttrs.spiking->SpikingParameter_a = _genome->get(_genome->SPIKING_A, groupIndex_to);
				neuronAttrs.spiking->SpikingParameter_b = _genome->get(_genome->SPIKING_B, groupIndex_to);
				neuronAttrs.spiking->SpikingParameter_c = _genome->get(_genome->SPIKING_C, groupIndex_to);
				neuronAttrs.spiking->SpikingParameter_d = _genome->get(_genome->SPIKING_D, groupIndex_to);
			} else {
				neuronAttrs.spiking->SpikingParameter_a = 0.02;
				neuronAttrs.spiking->SpikingParameter_b = 0.2;
				neuronAttrs.spiking->SpikingParameter_c = -65;
				neuronAttrs.spiking->SpikingParameter_d = 6;
			}
			break;
		case Brain::Configuration::TAU:
			neuronAttrs.firingRate->tau = _genome->get(_genome->TAU,groupIndex_to);
			// fall through
		case Brain::Configuration::FIRING_RATE:
			neuronAttrs.firingRate->group = groupIndex_to;
			neuronAttrs.firingRate->bias = _genome->get(_genome->BIAS,groupIndex_to);
			break;
		default:
			assert(false);
		}

#if DebugBrainGrow
		if( DebugBrainGrowPrint )
			cout << "  groupbias = " << groupbias nlf;
#endif

        for (int groupIndex_from = 0; groupIndex_from < _numgroups; groupIndex_from++)
        {
            eeremainder[groupIndex_from] = 0.0;
            eiremainder[groupIndex_from] = 0.0;
            iiremainder[groupIndex_from] = 0.0;
            ieremainder[groupIndex_from] = 0.0;
        }

        // setup all e-neurons for this group
        int neuronCount_to = _genome->getNeuronCount(EXCITATORY, groupIndex_to);

#if DebugBrainGrow
		if( DebugBrainGrowPrint )
			cout << "  Setting up " << neuronCount_to << " e-neurons" nlf;
#endif


		// "Local Index" means that the index is relative to the start of the neuron type (I or E) within
		// the group as opposed to the entire neuron array.
        for (int neuronLocalIndex_to = 0; neuronLocalIndex_to < neuronCount_to; neuronLocalIndex_to++)
        {
            int neuronIndex_to = neuronLocalIndex_to + firsteneur[groupIndex_to];

#if DebugBrainGrow
			if( DebugBrainGrowPrint )
				cout << "  For neuronLocalIndex_to, neuronIndex_to = "
					 << neuronLocalIndex_to cms neuronIndex_to << ":" nlf;
#endif

			_neuralnet->set_neuron( neuronIndex_to, neuronAttrs.opaque, numsyn );

			growSynapses( groupIndex_to,
						  neuronCount_to,
						  eeremainder,
						  neuronLocalIndex_to,
						  neuronIndex_to,
						  firsteneur,
						  numsyn,
						  _genome->EE );

			growSynapses( groupIndex_to,
						  neuronCount_to,
						  ieremainder,
						  neuronLocalIndex_to,
						  neuronIndex_to,
						  firstineur,
						  numsyn,
						  _genome->IE );

			_neuralnet->set_neuron_endsynapses( neuronIndex_to, numsyn );
            numneur++;
        }

        // setup all i-neurons for this group

        if( IsOutputNeuralGroup( groupIndex_to ) )
            neuronCount_to = 0;  // output/behavior neurons are e-only postsynaptically
        else
            neuronCount_to = _genome->getNeuronCount(INHIBITORY,  groupIndex_to );

#if DebugBrainGrow
		if( DebugBrainGrowPrint )
			cout << "  Setting up " << neuronCount_to << " i-neurons" nlf;
#endif

		// "Local Index" means that the index is relative to the start of the neuron type (I or E) within
		// the group as opposed to the entire neuron array.
        for (int neuronLocalIndex_to = 0; neuronLocalIndex_to < neuronCount_to; neuronLocalIndex_to++)
        {
            int neuronIndex_to = neuronLocalIndex_to + firstineur[groupIndex_to];

#if DebugBrainGrow
			if( DebugBrainGrowPrint )
			{
				cout << "  For neuronLocalIndex_to, neuronIndex_to = "
					 << neuronLocalIndex_to cms neuronIndex_to << ":" nlf;
			}
#endif

			_neuralnet->set_neuron( neuronIndex_to, neuronAttrs.opaque, numsyn );

			growSynapses( groupIndex_to,
						  neuronCount_to,
						  eiremainder,
						  neuronLocalIndex_to,
						  neuronIndex_to,
						  firsteneur,
						  numsyn,
						  _genome->EI );

			growSynapses( groupIndex_to,
						  neuronCount_to,
						  iiremainder,
						  neuronLocalIndex_to,
						  neuronIndex_to,
						  firstineur,
						  numsyn,
						  _genome->II );

			_neuralnet->set_neuron_endsynapses( neuronIndex_to, numsyn );
            numneur++;
        }
    }


	// ---
	// --- Sanity Checks
	// ---
	if (numneur != (_dims.numneurons))
        error(2,"Bad neural architecture, numneur (",numneur,") not equal to numneurons (",_dims.numneurons,")");

    if( numsyn != _dims.numsynapses )
	{
		if( Brain::config.synapseFromOutputNeurons )
		{
			if( (numsyn > _dims.numsynapses)
				|| (( (_dims.numsynapses - numsyn) / float(_dims.numsynapses) ) > 1.e-3) )
			{
				error(2,"Bad neural architecture, numsyn (",numsyn,") not equal to numsynapses (",_dims.numsynapses,")");
			}
		}

		_dims.numsynapses = numsyn;
	}
	//printf( "numsynapses=%ld\n", numsyn );
	
	// ---
	// --- Calculate Energy Use
	// ---
    _energyUse = Brain::config.maxneuron2energy * float(_dims.numneurons) / float(config.maxneurons)
		+ Brain::config.maxsynapse2energy * float(_dims.numsynapses) / float(config.maxsynapses);

    debugcheck( "after setting up brain architecture" );
}

//---------------------------------------------------------------------------
// GroupsBrain::growSynapses
//---------------------------------------------------------------------------
void GroupsBrain::growSynapses( int groupIndex_to,
								short neuronCount_to,
								float *remainder,
								short neuronLocalIndex_to,
								short neuronIndex_to,
								short *firstneur,
								long &synapseCount_brain,
								GroupsSynapseType *synapseType )
{
#if DebugBrainGrow
	if( DebugBrainGrowPrint )
		cout << "    Setting up " << synapseType->name << " connections:" nlf;
#endif

	RandomNumberGenerator *td_rng;
	Gene *td_seedGene;
	if( config.enableTopologicalDistortionRngSeed )
	{
		td_rng = RandomNumberGenerator::create( RandomNumberGenerator::TOPOLOGICAL_DISTORTION );
		td_seedGene = _genome->gene( "TopologicalDistortionRngSeed" );
	}
	else
	{
		td_rng = _cns->getRNG();
		td_seedGene = NULL;
	}
	RandomNumberGenerator *weight_rng;
	Gene *weight_seedGene;
	if( config.enableInitWeightRngSeed )
	{
		weight_rng = RandomNumberGenerator::create( RandomNumberGenerator::INIT_WEIGHT );
		weight_seedGene = _genome->gene( "InitWeightRngSeed" );
	}
	else
	{
		weight_rng = _cns->getRNG();
		weight_seedGene = NULL;
	}

	for (int groupIndex_from = 0; groupIndex_from < _numgroups; groupIndex_from++)
	{
		if( !Brain::config.synapseFromOutputNeurons && IsOutputNeuralGroup(groupIndex_from) )
			continue;

		int neuronCount_from = _genome->getNeuronCount(synapseType->nt_from, groupIndex_from);

#if DebugBrainGrow
		if( DebugBrainGrowPrint )
		{
			cout << "      From group " << groupIndex_from nlf;
			cout << "      with neuronCount_from, (old)" << synapseType->name << "remainder = "
				 << neuronCount_from cms remainder[groupIndex_from] nlf;
		}
#endif

		int synapseCount_fromto = _genome->getSynapseCount( synapseType,
															groupIndex_from,
															groupIndex_to );
		float nsynjiperneur = float(synapseCount_fromto)/float(neuronCount_to);
		int synapseCount_new = short(nsynjiperneur + remainder[groupIndex_from] + 1.e-5);
		remainder[groupIndex_from] += nsynjiperneur - synapseCount_new;
		float td_fromto = _genome->get( _genome->TOPOLOGICAL_DISTORTION,
										synapseType,
										groupIndex_from,
										groupIndex_to );
		if( config.enableTopologicalDistortionRngSeed )
		{
			long td_seed = _genome->get( td_seedGene,
										 synapseType,
										 groupIndex_from,
										 groupIndex_to );
			td_rng->seed( td_seed );
		}
		if( config.enableInitWeightRngSeed )
		{
			long weight_seed = _genome->get( weight_seedGene,
											 synapseType,
											 groupIndex_from,
											 groupIndex_to );
			weight_rng->seed( weight_seed );
		}

		// "Local Index" means that the index is relative to the start of the neuron type (I or E) within
		// the group as opposed to the entire neuron array.
		int neuronLocalIndex_fromBase = short((float(neuronLocalIndex_to) / float(neuronCount_to)) * float(neuronCount_from) - float(synapseCount_new) * 0.5);
		neuronLocalIndex_fromBase = max<short>(0, min<short>(neuronCount_from - synapseCount_new, neuronLocalIndex_fromBase));

#if DebugBrainGrow
		if( DebugBrainGrowPrint )
		{
			cout << "      and synapseCount_fromto, nsynjiperneur, synapseCount_new = "
				 << synapseCount_fromto cms nsynjiperneur cms synapseCount_new nlf;
			cout << "      and (new)" << synapseType->name << "remainder, td_fromto, neuronLocalIndex_fromBase = "
				 << remainder[groupIndex_from] cms td_fromto cms neuronLocalIndex_fromBase nlf;
		}
#endif

		if ((neuronLocalIndex_fromBase + synapseCount_new) > neuronCount_from)
		{
			char msg[128];
			sprintf( msg, "more %s synapses from group ", synapseType->name.c_str() );
			error(2,"Illegal architecture generated: ",
				  msg, groupIndex_from,
				  " to group ",groupIndex_to,
				  " than there are i-neurons in group ",groupIndex_from);
		}

		bool neurused[neuronCount_from];
		memset( neurused, 0, sizeof(neurused) );
				
		for (int isyn = 0; isyn < synapseCount_new; isyn++)
		{
			// "Local Index" means that the index is relative to the start of the neuron type (I or E) within
			// the group as opposed to the entire neuron array.
			int neuronLocalIndex_from;

			if (td_rng->drand() < td_fromto)
			{
				short distortion = short(nint(td_rng->range(-0.5,0.5)*td_fromto*neuronCount_from));
				neuronLocalIndex_from = isyn + neuronLocalIndex_fromBase + distortion;
                        
				if (neuronLocalIndex_from < 0)
					neuronLocalIndex_from += neuronCount_from;
				else if (neuronLocalIndex_from >= neuronCount_from)
					neuronLocalIndex_from -= neuronCount_from;
			}
			else
			{
				neuronLocalIndex_from = isyn + neuronLocalIndex_fromBase;
			}                      

			if (((neuronLocalIndex_from+firstneur[groupIndex_from]) == neuronIndex_to) // same neuron or
				|| neurused[neuronLocalIndex_from] ) // already connected to this one
			{
				if ( (groupIndex_to == groupIndex_from) // same group
					 && (synapseType->nt_from == synapseType->nt_to // same neuron type (I or E)
						 || IsOutputNeuralGroup(groupIndex_to)) )
					neuronLocalIndex_from = nearestFreeNeuron( neuronLocalIndex_from,
															   &neurused[0],
															   neuronCount_from,
															   neuronIndex_to - firstneur[groupIndex_from] );
				else
					neuronLocalIndex_from = nearestFreeNeuron( neuronLocalIndex_from,
															   &neurused[0],
															   neuronCount_from,
															   neuronLocalIndex_from);
			}

			neurused[neuronLocalIndex_from] = true;

			int neuronIndex_from = neuronLocalIndex_from + firstneur[groupIndex_from];

			// We should never have a self-synapsing neuron.
			assert( neuronIndex_from != neuronIndex_to );

			float efficacy;
			if( synapseType->nt_from == INHIBITORY )
			{
				efficacy = min(-1.e-10, -weight_rng->range(initminweight, Brain::config.initMaxWeight));
			}
			else
			{
				efficacy = weight_rng->range(initminweight, Brain::config.initMaxWeight);
			}				

			float lrate;
			if( !Brain::config.outputSynapseLearning
				&& (IsOutputNeuralGroup(groupIndex_from) || IsOutputNeuralGroup(groupIndex_to)) )
			{
				lrate = 0;
			}
			else
			{
				lrate = _genome->get( _genome->LEARNING_RATE,
									  synapseType,
									  groupIndex_from,
									  groupIndex_to );
			}

			_neuralnet->set_synapse( synapseCount_brain,
									 neuronIndex_from,
									 neuronIndex_to,
									 efficacy,
									 lrate );

			synapseCount_brain++;
		}
	}

	if( config.enableTopologicalDistortionRngSeed )
	{
		RandomNumberGenerator::dispose( td_rng );
	}
	if( config.enableInitWeightRngSeed )
	{
		RandomNumberGenerator::dispose( weight_rng );
	}
}
