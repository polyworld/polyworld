#include "SpeedSensor.h"

#include <assert.h>

#include "agent.h"
#include "NervousSystem.h"
#include "RandomNumberGenerator.h"

#define DISABLE_PROPRIOCEPTION false

SpeedSensor::SpeedSensor( agent *self )
{
	this->self = self;
}

SpeedSensor::~SpeedSensor()
{
}

void SpeedSensor::sensor_grow( NervousSystem *cns )
{
	nerve = cns->getNerve( "SpeedFeedback" );
}

void SpeedSensor::sensor_prebirth_signal( RandomNumberGenerator *rng )
{
	nerve->set( rng->drand() );
}

void SpeedSensor::sensor_update( bool print )
{
#if DISABLE_PROPRIOCEPTION
	float activation = 0;
#else
	float activation = self->NormalizedSpeed();
#endif

	nerve->set( activation );
}
