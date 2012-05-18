#include "EnergySensor.h"

#include "agent.h"
#include "NervousSystem.h"
#include "RandomNumberGenerator.h"

EnergySensor::EnergySensor( agent *self )
{
	this->self = self;
}

EnergySensor::~EnergySensor()
{
}

void EnergySensor::sensor_grow( NervousSystem *cns )
{
	nerve = cns->getNerve( "Energy" );
}

void EnergySensor::sensor_prebirth_signal( RandomNumberGenerator *rng )
{
	nerve->set( rng->drand() );
}

void EnergySensor::sensor_update( bool print )
{
	nerve->set( self->NormalizedEnergy() );
}
