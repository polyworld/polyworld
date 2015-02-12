#include "MateWaitSensor.h"

#include <assert.h>

#include "agent.h"
#include "NervousSystem.h"
#include "RandomNumberGenerator.h"

#define DISABLE_PROPRIOCEPTION false

MateWaitSensor::MateWaitSensor( agent *self,
								long mateWait )
{
	this->self = self;
	this->mateWait = mateWait;
}

MateWaitSensor::~MateWaitSensor()
{
}

void MateWaitSensor::sensor_grow( NervousSystem *cns )
{
	nerve = cns->getNerve( "MateWaitFeedback" );
}

void MateWaitSensor::sensor_prebirth_signal( RandomNumberGenerator *rng )
{
	nerve->set( rng->drand() );
}

void MateWaitSensor::sensor_update( bool print )
{
#if DISABLE_PROPRIOCEPTION
	float activation = 0;
#else
	long age = self->Age();
	long lastMate = self->LastMate();
	float activation = 1.0 - float(age - lastMate) / mateWait;
	
	if(activation < 0)
	{
		activation = 0;
	}

	//printf( "age=%ld, lastMate=%ld, mateWait=%ld, activation=%f\n", age, lastMate, mateWait, activation );
#endif

	nerve->set( activation );
}
