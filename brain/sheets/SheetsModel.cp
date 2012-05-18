#include "SheetsModel.h"

#include <stdlib.h>

#include "misc.h"

using namespace sheets;
using namespace std;

#define trc(x...)
//#define trc(x...) cout << x << endl;

// verbose trace
#define vtrc(x...)
//#define vtrc(x...) cout << x << endl;

namespace sheets
{
	std::ostream &operator<<( std::ostream &out, const NeuronSubset &n )
	{
		return out << "{" << n.begin << " --> " << n.end << "}";
	}
}


//===========================================================================
// Neuron
//===========================================================================

bool NeuronKeyCompare::operator()( Neuron *x, Neuron *y )
{
	return x->nonCulledId < y->nonCulledId;
}

//---------------------------------------------------------------------------
// Neuron::~Neuron
//---------------------------------------------------------------------------
Neuron::~Neuron()
{
	itfor( SynapseMap, synapsesOut, it )
		delete it->second;
}

//===========================================================================
// Sheet
//===========================================================================

const char *Sheet::getName( Type type )
{
	switch( type )
	{
	case Input: return "Input";
	case Output: return "Output";
	case Internal: return "Internal";
	default: assert( false );
	}
}

//---------------------------------------------------------------------------
// Sheet::Sheet
//---------------------------------------------------------------------------
Sheet::Sheet( SheetsModel *model,
			  string name,
			  int id,
			  Type type,
			  Orientation orientation,
			  float slot,
			  const Vector2f &center,
			  const Vector2f &size,
			  const Vector2i &neuronCount,
			  function<void (Neuron*)> &neuronCreated )
: _sheetsModel( model )
, _name( name )
, _id( id )
, _type( type )
, _orientation( orientation )
, _slot( slot )
, _center( center )
, _size( size )
, _neuronCount( neuronCount )
, _neuronSpacing( 1.0f / _neuronCount.a,
				  1.0f / _neuronCount.b )
, _neuronInsets( _neuronSpacing / 2 )
, _nneurons( neuronCount.a * neuronCount.b )
{
	trc( "Sheet [orig] Center=" << _center << " Size=" << _size << " --> Corners=[ " << (_center - (_size / 2)) << " , " << (_center + (_size / 2)) << " ]" );

	// If the sheet exceeds the model boundaries, then we must scale it down and translate it to fit.
	trim( _center.a, _size.a );
	trim( _center.b, _size.b );

	trc( "Sheet [trim] Center=" << _center << " Size=" << _size << " --> Corners[ " << (_center - (_size / 2)) << " , " << (_center + (_size / 2)) << " ]" );

	createNeurons( neuronCreated );
}

//---------------------------------------------------------------------------
// Sheet::~Sheet
//---------------------------------------------------------------------------
Sheet::~Sheet()
{
	delete [] _neurons;
}

//---------------------------------------------------------------------------
// Sheet::getName
//---------------------------------------------------------------------------
string Sheet::getName()
{
	return _name;
}

//---------------------------------------------------------------------------
// Sheet::getId
//---------------------------------------------------------------------------
int Sheet::getId()
{
	return _id;
}

//---------------------------------------------------------------------------
// Sheet::getType
//---------------------------------------------------------------------------
Sheet::Type Sheet::getType()
{
	return _type;
}

//---------------------------------------------------------------------------
// Sheet::getNeuronCount
//---------------------------------------------------------------------------
const Vector2i &Sheet::getNeuronCount()
{
	return _neuronCount;
}

//---------------------------------------------------------------------------
// Sheet::getNeuron
//---------------------------------------------------------------------------
Neuron *Sheet::getNeuron( int a, int b )
{
	assert( a < _neuronCount.a );
	assert( a >= 0 );
	assert( b < _neuronCount.b );
	assert( b >= 0 );

	int offset = (a + (b * _neuronCount.a));
	assert( (offset >= 0) && (offset < _nneurons) );

	return _neurons + offset;
}

//---------------------------------------------------------------------------
// Sheet::getNeuron
//---------------------------------------------------------------------------
Neuron *Sheet::getNeuron( Vector2i index )
{
	return getNeuron( index.a, index.b );
}

//---------------------------------------------------------------------------
// Sheet::addReceptiveField
//---------------------------------------------------------------------------
void Sheet::addReceptiveField( ReceptiveFieldRole role,
							   Vector2f fieldOffset,
							   Vector2f fieldSize,
							   Sheet *other,
							   function<bool (Neuron *, ReceptiveFieldNeuronRole)> neuronPredicate,
							   function<void (Synapse *)> synapseCreated )
{
	fieldSize.a = max( fieldSize.a, other->_neuronSpacing.a );
	fieldSize.b = max( fieldSize.b, other->_neuronSpacing.b );

	ReceptiveFieldNeuronRole currentNeuronRole;
	ReceptiveFieldNeuronRole otherNeuronRole;
	switch( role )
	{
	case Source:
		currentNeuronRole = To;
		otherNeuronRole = From;
		break;
	case Target:
		currentNeuronRole = From;
		otherNeuronRole = To;
		break;
	default:
		assert( false );
	}

	// Iterate over each neuron in this sheet.
	for( int i = 0; i < _nneurons; i++ )
	{
		Neuron *currentNeuron = _neurons + i;

		if( !neuronPredicate(currentNeuron, currentNeuronRole) )
			continue;

		// ---
		// --- Find all neurons within the receptive field.
		// ---
		NeuronSubset allReceptiveFieldNeurons =
			other->findReceptiveFieldNeurons( currentNeuron->sheetPosition,
											  fieldOffset,
											  fieldSize );
		
		// ---
		// --- Find which neurons pass the predicate, and determine the mean distance.
		// ---

		Neuron *fieldNeurons[ (allReceptiveFieldNeurons.end.a - allReceptiveFieldNeurons.begin.a + 1)
							  * (allReceptiveFieldNeurons.end.b - allReceptiveFieldNeurons.begin.b + 1) ];
		int nfieldNeurons = 0;
		float totalDistance = 0;

		for( int a = allReceptiveFieldNeurons.begin.a; a <= allReceptiveFieldNeurons.end.a; a++ )
		{
			for( int b = allReceptiveFieldNeurons.begin.b; b <= allReceptiveFieldNeurons.end.b; b++ )
			{
				Neuron *otherNeuron = other->getNeuron( a, b );

				// Ignore self-synapse
				if( currentNeuron == otherNeuron )
					continue;

				if( !neuronPredicate(otherNeuron, otherNeuronRole) )
					continue;

				fieldNeurons[ nfieldNeurons++ ] = otherNeuron;
				totalDistance += currentNeuron->absPosition.distance( otherNeuron->absPosition );
			}
		}

		// No neurons in receptive field.
		if( nfieldNeurons == 0 )
			continue;

		// ---
		// --- Determine number of synapses to create
		// ---
		float meanDistance = totalDistance / nfieldNeurons;
		float probabilitySynapse = _sheetsModel->getProbabilitySynapse( meanDistance );
		int nsynapses = round( probabilitySynapse * nfieldNeurons );

		if( nsynapses == 0 )
			continue;

		// ---
		// --- Create synapses
		// ---
		int stride = nfieldNeurons / nsynapses;

		for( int offset = 0; nsynapses > 0; offset += stride, nsynapses-- )
		{
			Neuron *otherNeuron = fieldNeurons[offset];
			Synapse *synapse = NULL;

			switch( role )
			{
			case Source:
				synapse = createSynapse( otherNeuron, currentNeuron );
				break;
			case Target:
				synapse = createSynapse( currentNeuron, otherNeuron );
				break;
			default:
				assert( false );
			}

			if( synapse )
				synapseCreated( synapse );
		}
	}
}

//---------------------------------------------------------------------------
// Sheet::trim
//---------------------------------------------------------------------------
void Sheet::trim( float &center, float &size )
{
	float overflow;

	overflow = (center + size/2) - 1.0;
	if( overflow > 0 )
	{
		size -= overflow;
		center -= overflow / 2;

		assert( fabs(center + (size/2) - 1.0) < 1e-6 );
	}
	else
	{
		overflow = 0 - (center - size/2);
		if( overflow > 0 )
		{
			size -= overflow;
			center += overflow / 2;

			assert( fabs(center - (size/2)) < 1e-6 );
		}
	}
}

//---------------------------------------------------------------------------
// Sheet::createNeurons
//---------------------------------------------------------------------------
void Sheet::createNeurons( function<void (Neuron*)> &neuronCreated )
{
	_neurons = new Neuron[ _nneurons ];

	for( int i = 0; i < _neuronCount.a; i++ )
	{
		for( int j = 0; j < _neuronCount.b; j++ )
		{
			Neuron *neuron = getNeuron( i, j );

			neuron->sheet = this;
			neuron->nonCulledId = _sheetsModel->_numNonCulledNeurons++;
			neuron->id = -1;
			neuron->sheetIndex.set( i, j );
			neuron->cullState.touchedFromInput = false;
			neuron->cullState.touchedFromOutput = false;

			// ---
			// --- Compute position within sheet 
			// ---

			// 2D position within sheet
			neuron->sheetPosition.set( _neuronInsets.a + (i * _neuronSpacing.a),
									   _neuronInsets.b + (j * _neuronSpacing.b) );

			// ---
			// --- Compute absolute 3D position within model
			// ---

			// Scale 2D position by sheet size
			Vector2f position = neuron->sheetPosition;
			position.scale( _size );

			// Translate by sheet location
			position += _center - ( _size / 2 );

			// Create 3D position
			neuron->absPosition.set( _orientation, position, _slot );

			// Scale to model size.
			neuron->absPosition.scale( _sheetsModel->getSize() );

			trc( "Neuron[" << i << "," << j << "]: sheetPos=" << neuron->sheetPosition << ", absPos=" << neuron->absPosition );

			assert( neuron == getNeuron(neuron->sheetIndex) );

			neuronCreated( neuron );
		}
	}
}

//---------------------------------------------------------------------------
// Sheet::findNeurons
//---------------------------------------------------------------------------
NeuronSubset Sheet::findNeurons( const Vector2f &ul,
								 const Vector2f &lr )
{
	NeuronSubset result;

	result.begin.a = max( 0, (int)ceilf( (ul.a - _neuronInsets.a) / _neuronSpacing.a ) );
	result.end.a = min( _neuronCount.a - 1, (int)floorf( (lr.a - _neuronInsets.a) / _neuronSpacing.a ) );
	result.begin.b = max( 0, (int)ceilf( (ul.b - _neuronInsets.b) / _neuronSpacing.b ) );
	result.end.b = min( _neuronCount.b - 1, (int)floorf( (lr.b - _neuronInsets.b) / _neuronSpacing.b ) );

	return result;
}

//---------------------------------------------------------------------------
// Sheet::findReceptiveFieldNeurons
//---------------------------------------------------------------------------
NeuronSubset Sheet::findReceptiveFieldNeurons( const Vector2f &neuronPosition,
											   const Vector2f &fieldOffset,
											   const Vector2f &fieldSize )
{	
	struct local
	{
		static void offsetCenter( float &center, float offset )
		{
			if( offset < 0 )
				center += center * offset;
			else
				center += (1 - center) * offset;
		}
	};

	vtrc( "Receptive neuron pos=" << neuronPosition << ", offset=" << fieldOffset << ", size=" << fieldSize );

	Vector2f center = neuronPosition;

	local::offsetCenter( center.a, fieldOffset.a );
	local::offsetCenter( center.b, fieldOffset.b );

	vtrc( "  Adjusted center=" << center );

	Vector2f ul = center - (fieldSize / 2);
	Vector2f lr = center + (fieldSize / 2);

	vtrc( "  ul=" << ul << ", lr=" << lr );

	return findNeurons( ul, lr );
}

//---------------------------------------------------------------------------
// Sheet::createSynapse
//---------------------------------------------------------------------------
Synapse *Sheet::createSynapse( Neuron *from, Neuron *to )
{
	if( from == to )
		return NULL;

	if( from->synapsesOut[to] != NULL )
		return NULL;

	Synapse *synapse = new Synapse();
	synapse->from = from;
	synapse->to = to;

	from->synapsesOut[to] = synapse;
	to->synapsesIn[from] = synapse;

	trc( "Synapse [" << from->sheet->_id << "] " << from->absPosition << " --> [" << to->sheet->_id << "] " << to->absPosition );

	return synapse;
}


//===========================================================================
// SheetsModel
//===========================================================================

//---------------------------------------------------------------------------
// SheetsModel::SheetsModel
//---------------------------------------------------------------------------
SheetsModel::SheetsModel( const Vector3f &size, float synapseProbabilityX )
: _numNonCulledNeurons( 0 )
, _size( size )
, _synapseProbabilityX( synapseProbabilityX )
{
}

//---------------------------------------------------------------------------
// SheetsModel::~SheetsModel
//---------------------------------------------------------------------------
SheetsModel::~SheetsModel()
{
	itfor( SheetVector, _allSheets, it )
		delete *it;
}

//---------------------------------------------------------------------------
// SheetsModel::createSheet
//---------------------------------------------------------------------------
Sheet *SheetsModel::createSheet( string name,
								 int id,
								 Sheet::Type type,
								 Orientation orientation,
								 float slot,
								 const Vector2f &center,
								 const Vector2f &size,
								 const Vector2i &neuronCount,
								 function<void (Neuron*)> &neuronCreated )
{
	if( id == -1 )
		id = (int)_allSheets.size();

	Sheet *sheet = new Sheet( this,
							  name,
							  id,
							  type,
							  orientation,
							  slot,
							  center,
							  size,
							  neuronCount,
							  neuronCreated );

	if( (int)_allSheets.size() <= id )
		_allSheets.resize( id + 1 );
	assert( _allSheets[id] == NULL );
	_allSheets[id] = sheet;

	switch( type )
	{
	case Sheet::Input:
		_inputSheets.push_back( sheet );
		break;
	case Sheet::Output:
		_outputSheets.push_back( sheet );
		break;
	case Sheet::Internal:
		_internalSheets.push_back( sheet );
		break;
	default:
		assert( false );
		break;
	}

	return sheet;
}

//---------------------------------------------------------------------------
// SheetsModel::getSheet
//---------------------------------------------------------------------------
Sheet *SheetsModel::getSheet( int id )
{
	return _allSheets[ id ];
}

//---------------------------------------------------------------------------
// SheetsModel::getSheets
//---------------------------------------------------------------------------
const SheetVector &SheetsModel::getSheets( Sheet::Type type )
{
	switch( type )
	{
	case Sheet::Input: return _inputSheets;
	case Sheet::Output: return _outputSheets;
	case Sheet::Internal: return _internalSheets;
	default: assert( false );
	}
}

//---------------------------------------------------------------------------
// SheetsModel::cull
//---------------------------------------------------------------------------
void SheetsModel::cull()
{
	assert( !_inputSheets.empty() );
	assert( !_outputSheets.empty() );

	for( Sheet *sheet : _inputSheets )
	{
		const Vector2i count = sheet->getNeuronCount();
		for( int a = 0; a < count.a; a++ )
			for( int b = 0; b < count.b; b++ )
				touchFromInput( sheet->getNeuron(a, b) );
	}

	for( Sheet *sheet : _outputSheets )
	{
		const Vector2i count = sheet->getNeuronCount();
		for( int a = 0; a < count.a; a++ )
			for( int b = 0; b < count.b; b++ )
				touchFromOutput( sheet->getNeuron(a, b) );
	}

	addNonCulledNeurons( _inputSheets );
	addNonCulledNeurons( _outputSheets );
	addNonCulledNeurons( _internalSheets );
}

//---------------------------------------------------------------------------
// SheetsModel::getNeurons
//---------------------------------------------------------------------------
NeuronVector &SheetsModel::getNeurons()
{
	return _neurons;
}

//---------------------------------------------------------------------------
// SheetsModel::getProbabilitySynapse
//---------------------------------------------------------------------------
float SheetsModel::getProbabilitySynapse( float distance )
{
	return (1 / _synapseProbabilityX) * exp(-distance/_synapseProbabilityX);
}

//---------------------------------------------------------------------------
// SheetsModel::touchFromInput
//---------------------------------------------------------------------------
void SheetsModel::touchFromInput( Neuron *neuron )
{
	if( neuron->cullState.touchedFromInput )
		return;

	neuron->cullState.touchedFromInput = true;

	itfor( SynapseMap, neuron->synapsesOut, it )
	{
		Synapse *syn = it->second;
		touchFromInput( syn->to );
	}
}

//---------------------------------------------------------------------------
// SheetsModel::touchFromOutput
//---------------------------------------------------------------------------
void SheetsModel::touchFromOutput( Neuron *neuron )
{
	if( neuron->cullState.touchedFromOutput )
		return;

	neuron->cullState.touchedFromOutput = true;

	itfor( SynapseMap, neuron->synapsesIn, it )
	{
		Synapse *syn = it->second;
		touchFromOutput( syn->from );
	}
}

//---------------------------------------------------------------------------
// SheetsModel::addNonCulledNeurons
//---------------------------------------------------------------------------
void SheetsModel::addNonCulledNeurons( SheetVector &sheets )
{
	itfor( SheetVector, sheets, it )
	{
		Sheet *sheet = *it;
		const Vector2i count = sheet->getNeuronCount();
		for( int a = 0; a < count.a; a++ )
		{
			for( int b = 0; b < count.b; b++ )
			{
				Neuron *neuron = sheet->getNeuron( a, b );
				if( neuron->cullState.touchedFromOutput && neuron->cullState.touchedFromInput )
				{
					neuron->id = (int)_neurons.size();
					_neurons.push_back( neuron );
				}
				else
				{
					trc( "CULLED: " << "[" << sheet->getId() << "] " << neuron->absPosition );

					itfor( SynapseMap, neuron->synapsesIn, it_syn )
					{
						Synapse *syn = it_syn->second;
						syn->from->synapsesOut.erase( neuron );
						delete syn;
					}
					neuron->synapsesIn.clear();

					itfor( SynapseMap, neuron->synapsesOut, it_syn )
					{
						Synapse *syn = it_syn->second;
						syn->to->synapsesIn.erase( neuron );
						delete syn;
					}
					neuron->synapsesOut.clear();
				}
			}
		}
	}

}

//===========================================================================
// TEST
//===========================================================================
static void test()
{
	/*
	SheetsModel model( Vector3f(1, 1, 1),	// size
					   1.);					// probability synapse X

	Sheet *sheetInput = model.createSheet( -1,
										   Sheet::Input,
										   PlaneXZ,
										   0.5,							// slot
										   Vector2f(0.25, 0.5),			// center
										   Vector2f(1.),				// size
										   Vector2i(2, 2) );				// neuronCount
	Sheet *sheetOutput = model.createSheet( -1,
											Sheet::Output,
											PlaneXZ,
											0.5,							// slot
											Vector2f(0.25, 0.5),			// center
											Vector2f(1.),				// size
											Vector2i(2, 2) );				// neuronCount
	Sheet *sheetInternal = model.createSheet( -1,
											  Sheet::Internal,
											  PlaneXZ,
											  0.5,							// slot
											  Vector2f(0.25, 0.5),			// center
											  Vector2f(1.),				// size
											  Vector2i(2, 2) );				// neuronCount
	Sheet *sheetInternal1 = model.createSheet( -1,
											   Sheet::Internal,
											   PlaneXZ,
											   0.5,							// slot
											   Vector2f(0.25, 0.5),			// center
											   Vector2f(1.),				// size
											   Vector2i(2, 2) );				// neuronCount

	sheetInput->addReceptiveField( Sheet::Target,
								   Vector2f(0.0, 0.0),	// offset
								   Vector2f(0.1),		// size
								   sheetInternal,
								   NULL );
	sheetOutput->addReceptiveField( Sheet::Source,
									Vector2f(0.0, 0.0),	// offset
									Vector2f(0.5),		// size
									sheetInternal,
									NULL );

	sheetInternal1->addReceptiveField( Sheet::Source,
									   Vector2f(0.0, 0.0),	// offset
									   Vector2f(0.5),		// size
									   sheetInternal,
									   NULL );

	model.cull();

	cout << "Neurons: " << endl;
	itfor( NeuronVector, model.getNeurons(), it )
	{
		cout << (*it)->id << " " << (*it)->absPosition << endl;
	}
	*/
	/*
	sheet->addReceptiveField( Sheet::Target,
							  Vector2f(0.5, 0.5),	// offset
							  Vector2f(.1),		// size
							  sheet,
							  NULL );
	*/

	/*
	NeuronSubset fieldNeurons = sheet->findReceptiveFieldNeurons( Vector2f(0.5, 0.5),   // neuron pos
																  Vector2f(-0.5, 0.5),       // field offset      
																  Vector2f(0.5, 0.5) ); // field size
	cout << fieldNeurons << endl;
	*/

	//cout << sheet->findNeurons( Vector2f(0.15, 0.15), Vector2f(0.2, 0.2) ) << endl;


	exit(0);

}

#if false
static class sinit {
 public:
	sinit() { test(); }
} _sinit;
#endif
