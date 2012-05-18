#include "CarryingSensor.h"

#include <assert.h>

#include "agent.h"
#include "NervousSystem.h"
#include "RandomNumberGenerator.h"

#define DISABLE_PROPRIOCEPTION true

CarryingSensor::CarryingSensor( agent *self )
{
	this->self = self;
}

CarryingSensor::~CarryingSensor()
{
}

void CarryingSensor::sensor_grow( NervousSystem *cns )
{
	nerve = cns->getNerve( "Carrying" );
}

void CarryingSensor::sensor_prebirth_signal( RandomNumberGenerator *rng )
{
	nerve->set( rng->drand() );
}

void CarryingSensor::sensor_update( bool print )
{
	float activation;

	if( self->NumCarries() > 0 )
		activation = 1.0;
	else
		activation = 0.0;

	nerve->set( activation );
}
