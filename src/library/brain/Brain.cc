/********************************************************************/
/* PolyWorld:  An Artificial Life Ecological Simulator              */
/* by Larry Yaeger                                                  */
/* Copyright Apple Computer 1990,1991,1992                          */
/********************************************************************/

// Self
#include "Brain.h"

#include <fstream>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>

// Local
#include "NervousSystem.h"
#include "NeuronModel.h"
#include "agent/agent.h"
#include "brain/groups/GroupsBrain.h"
#include "brain/sheets/SheetsBrain.h"
#include "sim/debug.h"
#include "sim/globals.h"
#include "sim/Simulation.h"
#include "utils/AbstractFile.h"
#include "utils/misc.h"

using namespace genome;


// Internal globals
#if DebugBrainGrow
	bool DebugBrainGrowPrint = true;
#endif

//===========================================================================
// Brain
//===========================================================================

Brain::Configuration Brain::config;

//---------------------------------------------------------------------------
// Brain::processWorldfile
//---------------------------------------------------------------------------
void Brain::processWorldfile( proplib::Document &doc )
{
	{
		string val = doc.get( "BrainArchitecture" );
		if( val == "Groups" )
			Brain::config.architecture = Brain::Configuration::Groups;
		else if( val == "Sheets" )
			Brain::config.architecture = Brain::Configuration::Sheets;
		else
			assert( false );
	}
	{
		string val = doc.get( "NeuronModel" );
		if( val == "F" )
			Brain::config.neuronModel = Brain::Configuration::FIRING_RATE;
		else if( val == "T" )
			Brain::config.neuronModel = Brain::Configuration::TAU_GAIN;
		else if( val == "S" )
			Brain::config.neuronModel = Brain::Configuration::SPIKING;
		else
			assert( false );
	}
	{
		string val = doc.get( "LearningMode" );
		if( val == "None" )
			Brain::config.learningMode = Brain::Configuration::LEARN_NONE;
		else if( val == "Prebirth" )
			Brain::config.learningMode = Brain::Configuration::LEARN_PREBIRTH;
		else if( val == "All" )
			Brain::config.learningMode = Brain::Configuration::LEARN_ALL;
		else
			assert( false );
	}

    Brain::config.Spiking.enableGenes = doc.get( "EnableSpikingGenes" );
	Brain::config.Spiking.aMinVal = doc.get( "SpikingAMin" );
	Brain::config.Spiking.aMaxVal = doc.get( "SpikingAMax" );
	Brain::config.Spiking.bMinVal = doc.get( "SpikingBMin" );
	Brain::config.Spiking.bMaxVal = doc.get( "SpikingBMax" );
	Brain::config.Spiking.cMinVal = doc.get( "SpikingCMin" );
	Brain::config.Spiking.cMaxVal = doc.get( "SpikingCMax" );
	Brain::config.Spiking.dMinVal = doc.get( "SpikingDMin" );
	Brain::config.Spiking.dMaxVal = doc.get( "SpikingDMax" );

	Brain::config.Tau.minVal = doc.get( "TauMin" );
	Brain::config.Tau.maxVal = doc.get( "TauMax" );
	Brain::config.Tau.seedVal = doc.get( "TauSeed" );

	Brain::config.Gain.minVal = doc.get( "GainMin" );
	Brain::config.Gain.maxVal = doc.get( "GainMax" );
	Brain::config.Gain.seedVal = doc.get( "GainSeed" );

    Brain::config.maxbias = doc.get( "MaxBiasWeight" );
	Brain::config.maxneuron2energy = doc.get( "EnergyUseNeurons" );
	Brain::config.outputSynapseLearning = doc.get( "OutputSynapseLearning" );
	Brain::config.synapseFromOutputNeurons = doc.get( "SynapseFromOutputNeurons" );
	Brain::config.synapseFromInputToOutputNeurons = doc.get( "SynapseFromInputToOutputNeurons" );
	Brain::config.numPrebirthCycles = doc.get( "PreBirthCycles" );

    Brain::config.logisticSlope = doc.get( "LogisticSlope" );
    Brain::config.fixedInitWeight = doc.get( "FixedInitSynapseWeight" );
    Brain::config.gaussianInitWeight = doc.get( "GaussianInitSynapseWeight" );
    Brain::config.gaussianInitMaxStdev = doc.get( "GaussianInitSynapseWeightMaxStdev" );
    Brain::config.maxWeight = doc.get( "MaxSynapseWeight" );
    Brain::config.initMaxWeight = doc.get( "MaxSynapseWeightInitial" );

    Brain::config.enableLearning = Brain::config.learningMode != Brain::Configuration::LEARN_NONE;
    Brain::config.minlrate = doc.get( "MinLearningRate" );
    Brain::config.maxlrate = doc.get( "MaxLearningRate" );

    Brain::config.maxsynapse2energy = doc.get( "EnergyUseSynapses" );
    Brain::config.decayRate = doc.get( "SynapseWeightDecayRate" );

 	// Set up retina values
	Brain::config.minWin = doc.get( "RetinaWidth" );

	GroupsBrain::processWorldfile( doc );
	SheetsBrain::processWorldfile( doc );
}

//---------------------------------------------------------------------------
// Brain::init
//---------------------------------------------------------------------------
void Brain::init()
{
    Brain::config.retinaWidth = max(Brain::config.minWin, GroupsBrain::config.maxvisneurpergroup);

    if (Brain::config.retinaWidth & 1)
        Brain::config.retinaWidth++;  // keep it even for efficiency (so I'm told)

    Brain::config.retinaHeight = Brain::config.minWin;

    if (Brain::config.retinaHeight & 1)
        Brain::config.retinaHeight++;

	GroupsBrain::init();
	SheetsBrain::init();
}

//---------------------------------------------------------------------------
// Brain::Brain
//---------------------------------------------------------------------------
Brain::Brain( NervousSystem *cns )
: _cns(cns)
, _neuralnet(NULL)
, _renderer(NULL)
, _energyUse(0)
, _frozen(false)
{
}


//---------------------------------------------------------------------------
// Brain::~Brain
//---------------------------------------------------------------------------
Brain::~Brain()
{
	delete _neuralnet;
	delete _renderer;
}

//---------------------------------------------------------------------------
// Brain::dumpAnatomical
//---------------------------------------------------------------------------
void Brain::dumpAnatomical( AbstractFile *file, long index, float fitness )
{
	// print the header, with index, fitness, and number of neurons
	file->printf( "brain %ld fitness=%g numneurons+1=%d maxWeight=%g maxBias=%g",
				  index, fitness, _dims.numNeurons+1, Brain::config.maxWeight, Brain::config.maxbias );

	_cns->dumpAnatomical( file );
	file->printf( "\n" );

	_neuralnet->dumpAnatomical( file );
}

//---------------------------------------------------------------------------
// Brain::startFunctional
//---------------------------------------------------------------------------
void Brain::startFunctional( AbstractFile *file, long index )
{
	file->printf( "version 1\n" );

	// print the header, with index (agent number)
	file->printf( "brainFunction %ld", index );

	// print neuron count, number of input neurons, number of synapses
	_neuralnet->startFunctional( file );

	// print timestep born
	file->printf( " %ld", TSimulation::fStep );

	// print organs portion
	_cns->startFunctional( file );

	file->printf( "\n" );
}

//---------------------------------------------------------------------------
// Brain::endFunctional
//---------------------------------------------------------------------------
void Brain::endFunctional( AbstractFile *file, float fitness )
{
	file->printf( "end fitness = %g\n", fitness );
}

//---------------------------------------------------------------------------
// Brain::writeFunctional
//---------------------------------------------------------------------------
void Brain::writeFunctional( AbstractFile *file )
{
	_neuralnet->writeFunctional( file );
}

//---------------------------------------------------------------------------
// Brain::dumpSynapses
//---------------------------------------------------------------------------
void Brain::dumpSynapses( AbstractFile *file, long index )
{
	file->printf( "synapses %ld maxweight=%g numsynapses=%ld numneurons=%d numinputneurons=%d numoutputneurons=%d\n",
				  index, Brain::config.maxWeight, _dims.numSynapses, _dims.numNeurons, _dims.numInputNeurons, _dims.numOutputNeurons );
	_neuralnet->dumpSynapses( file );
}

//---------------------------------------------------------------------------
// Brain::loadSynapses
//---------------------------------------------------------------------------
void Brain::loadSynapses( AbstractFile *file, float maxWeight )
{
	long index;
	float fileMaxWeight;
	long numSynapses;
	int numNeurons;
	int numInputNeurons;
	int numOutputNeurons;
	int rc = file->scanf( "synapses %ld maxweight=%g numsynapses=%ld numneurons=%d numinputneurons=%d numoutputneurons=%d\n",
						  &index, &fileMaxWeight, &numSynapses, &numNeurons, &numInputNeurons, &numOutputNeurons );
	assert( rc == 6 );
	assert( numSynapses == _dims.numSynapses );
	assert( numNeurons == _dims.numNeurons );
	assert( numInputNeurons == _dims.numInputNeurons );
	assert( numOutputNeurons == _dims.numOutputNeurons );
	_neuralnet->loadSynapses( file );
	if( maxWeight >= 0.0f )
		_neuralnet->scaleSynapses( maxWeight / fileMaxWeight );
}

//---------------------------------------------------------------------------
// Brain::copySynapses
//---------------------------------------------------------------------------
void Brain::copySynapses( Brain *other )
{
	assert( other->_dims.numSynapses == _dims.numSynapses );
	assert( other->_dims.numNeurons == _dims.numNeurons );
	assert( other->_dims.numInputNeurons == _dims.numInputNeurons );
	assert( other->_dims.numOutputNeurons == _dims.numOutputNeurons );
	_neuralnet->copySynapses( other->_neuralnet );
}

//---------------------------------------------------------------------------
// Brain::prebirth
//---------------------------------------------------------------------------
void Brain::prebirth()
{
    // now send some signals through the system
    // try pure noise for now...
    for( int i = 0; i < Brain::config.numPrebirthCycles; i++ )
    {
		_cns->prebirthSignal();

		update( false );
    }

    debugcheck( "after prebirth cycling" );
}

//---------------------------------------------------------------------------
// Brain::update
//---------------------------------------------------------------------------
void Brain::update( bool bprint )
{
	_neuralnet->update( bprint );
}

//---------------------------------------------------------------------------
// Brain::getRenderer
//---------------------------------------------------------------------------
NeuralNetRenderer *Brain::getRenderer()
{
	return _renderer;
}
