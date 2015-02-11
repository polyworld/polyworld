#include "Nerve.h"

#include <assert.h>
#include <string.h>

using namespace std;

Nerve::Nerve( Type _type,
			  const std::string &_name,
			  int _igroup )
: type(_type)
, name(_name)
, numneurons(0)
, index(-1)
{
	memset( activations, 0, sizeof(activations) );
}

float Nerve::get( int ineuron,
				  ActivationBuffer buf )
{
	if( numneurons == 0 )
		return 0.0;

	assert( (ineuron >= 0) && (ineuron < numneurons) && (index > -1) );
	
	return (*(activations[buf]))[index + ineuron];
}

void Nerve::set( float activation,
				 ActivationBuffer buf )
{
	if( numneurons == 0 )
		return;

	assert( numneurons == 1 );
	
	set( 0, activation, buf );
}

void Nerve::set( int ineuron,
				 float activation,
				 ActivationBuffer buf )
{
	assert( (ineuron >= 0) && (ineuron < numneurons) && (index > -1) );
	
	(*(activations[buf]))[index + ineuron] = activation;
}

int Nerve::getIndex()
{
	return index;
}

int Nerve::getNeuronCount()
{
	return numneurons;
}

void Nerve::config( int _numneurons,
					int _index )
{
	numneurons = _numneurons;
	index = _index;
}

void Nerve::config( float **_activations,
					float **_activations_swap )
{
	activations[CURRENT] = _activations;
	activations[SWAP] = _activations_swap;
}
