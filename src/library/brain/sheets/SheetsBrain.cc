#include "SheetsBrain.h"

#include <assert.h>

#include "FiringRateModel.h"
#include "misc.h"
#include "NervousSystem.h"
#include "SheetsGenome.h"
#include "SpikingModel.h"

using namespace genome;
using namespace sheets;

SheetsBrain::Configuration SheetsBrain::config;

//---------------------------------------------------------------------------
// SheetsBrain::processWorldfile
//---------------------------------------------------------------------------
void SheetsBrain::processWorldfile( proplib::Document &doc )
{
	proplib::Property &sheets = doc.get( "Sheets" );

	SheetsBrain::config.minBrainSize.x = sheets.get( "MinBrainSize" ).get( "X" );
	SheetsBrain::config.minBrainSize.y = sheets.get( "MinBrainSize" ).get( "Y" );
	SheetsBrain::config.minBrainSize.z = sheets.get( "MinBrainSize" ).get( "Z" );

	SheetsBrain::config.maxBrainSize.x = sheets.get( "MaxBrainSize" ).get( "X" );
	SheetsBrain::config.maxBrainSize.y = sheets.get( "MaxBrainSize" ).get( "Y" );
	SheetsBrain::config.maxBrainSize.z = sheets.get( "MaxBrainSize" ).get( "Z" );

	SheetsBrain::config.minSynapseProbabilityX = sheets.get( "MinSynapseProbabilityX" );
	SheetsBrain::config.maxSynapseProbabilityX = sheets.get( "MaxSynapseProbabilityX" );

	SheetsBrain::config.minLearningRate = sheets.get( "MinLearningRate" );
	SheetsBrain::config.maxLearningRate = sheets.get( "MaxLearningRate" );

    SheetsBrain::config.minVisionNeuronsPerSheet = sheets.get( "MinVisionNeuronsPerSheet" );
    SheetsBrain::config.maxVisionNeuronsPerSheet = sheets.get( "MaxVisionNeuronsPerSheet" );

    SheetsBrain::config.minInternalSheetsCount = sheets.get( "MinInternalSheetsCount" );
    SheetsBrain::config.maxInternalSheetsCount = sheets.get( "MaxInternalSheetsCount" );

    SheetsBrain::config.minInternalSheetSize = sheets.get( "MinInternalSheetSize" );
    SheetsBrain::config.maxInternalSheetSize = sheets.get( "MaxInternalSheetSize" );

    SheetsBrain::config.minInternalSheetNeuronCount = sheets.get( "MinInternalSheetNeuronCount" );
    SheetsBrain::config.maxInternalSheetNeuronCount = sheets.get( "MaxInternalSheetNeuronCount" );
}

//---------------------------------------------------------------------------
// SheetsBrain::init
//---------------------------------------------------------------------------
void SheetsBrain::init()
{
}

//---------------------------------------------------------------------------
// SheetsBrain::SheetsBrain
//---------------------------------------------------------------------------
SheetsBrain::SheetsBrain( NervousSystem *cns, SheetsGenome *genome, SheetsModel *model )
: Brain( cns )
, _numInternalSheets( 0 )
, _numInternalNeurons( 0 )
{
	memset( _numSynapses, 0, sizeof(_numSynapses) );

	grow( genome, model );
	delete model;
}

//---------------------------------------------------------------------------
// SheetsBrain::~SheetsBrain
//---------------------------------------------------------------------------
SheetsBrain::~SheetsBrain()
{
}

//---------------------------------------------------------------------------
// SheetsBrain::grow
//---------------------------------------------------------------------------
void SheetsBrain::grow( SheetsGenome *genome, SheetsModel *model )
{
	// ---
	// --- Configure Neuron Count
	// ---
	_dims.numNeurons = (int)model->getNeurons().size();

	// ---
	// --- Configure Synapse Count
	// ---
	itfor( NeuronVector, model->getNeurons(), it )
		_dims.numSynapses += (int)(*it)->synapsesOut.size();

	// ---
	// --- Configure Input/Output Neurons/Nerves
	// ---
	{
		int numInputSheets = (int)model->getSheets(Sheet::Input).size();
		int numOutputSheets = (int)model->getSheets(Sheet::Output).size();
		int numInternalSheets = (int)model->getSheets(Sheet::Internal).size();
		int numSheets = numInputSheets + numOutputSheets + numInternalSheets;
		int sheetNeuronCount[ numSheets ];
		memset( sheetNeuronCount, 0, sizeof(sheetNeuronCount) );

		itfor( NeuronVector, model->getNeurons(), it )
		{
			Neuron *neuron = *it;
			switch( neuron->sheet->getType() )
			{
			case Sheet::Input:
				_dims.numInputNeurons++;
				break;
			case Sheet::Output:
				_dims.numOutputNeurons++;
				break;
			case Sheet::Internal:
				// no-op
				break;
			default:
				assert( false );
			}

			sheetNeuronCount[ neuron->sheet->getId() ]++;
		}

		int neuronIndex = 0;

		for( int sheetId = 0; sheetId < (numInputSheets + numOutputSheets); sheetId++ )
		{
			Sheet *sheet = model->getSheet( sheetId );
			Nerve *nerve = _cns->getNerve( sheet->getName() );
		
			int neuronCount = sheetNeuronCount[sheetId];
			nerve->config( neuronCount, neuronIndex );
			neuronIndex += neuronCount;
		}

		for( int sheetId = numInputSheets + numOutputSheets; sheetId < numSheets; sheetId++ )
		{
			if( sheetNeuronCount[sheetId] )
			{
				_numInternalSheets++;
				_numInternalNeurons += sheetNeuronCount[sheetId];
			}
		}
	}

	// ---
	// --- Instantiate Neural Net 
	// ---
	{
		switch( Brain::config.neuronModel )
		{
		case Brain::Configuration::SPIKING:
			{
				SpikingModel *spiking = new SpikingModel( _cns,
														  genome->get("ScaleLatestSpikes") );
				_neuralnet = spiking;
			}
			break;
		case Brain::Configuration::FIRING_RATE:
		case Brain::Configuration::TAU:
			{
				FiringRateModel *firingRate = new FiringRateModel( _cns );
				_neuralnet = firingRate;
			}
			break;
		default:
			assert(false);
		}

		_neuralnet->init( &_dims, 0.0f );
	}

	// ---
	// --- Configure Neural Net 
	// ---
	{
		int synapseIndex = 0;

		itfor( NeuronVector, model->getNeurons(), it )
		{
			Neuron *neuron = *it;

			_neuralnet->set_neuron( neuron->id,
									&(neuron->attrs.neuronModel),
									synapseIndex );

			itfor( SynapseMap, neuron->synapsesIn, it_synapse )
			{
				Synapse *synapse = it_synapse->second;

				_neuralnet->set_synapse( synapseIndex++,
										 synapse->from->id,
										 synapse->to->id,
										 synapse->attrs.weight,
										 synapse->attrs.lrate );

				_numSynapses[ synapse->from->sheet->getType() ][ synapse->to->sheet->getType() ] += 1;
			}

			_neuralnet->set_neuron_endsynapses( neuron->id, synapseIndex );
		}
	}
}
