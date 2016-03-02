#include "EnergySensor.h"

#include "agent.h"
#include "brain/NervousSystem.h"
#include "utils/RandomNumberGenerator.h"

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
	if( Brain::config.enableLearning )
		nerve->set( rng->drand() );
	else
		nerve->set( 0.0 );
}

void EnergySensor::sensor_update( bool print )
{
	nerve->set( self->NormalizedEnergy() );
}
