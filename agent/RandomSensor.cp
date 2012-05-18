#include "RandomSensor.h"

#include "NervousSystem.h"
#include "RandomNumberGenerator.h"

RandomSensor::RandomSensor( RandomNumberGenerator *rng )
{
	this->rng = rng;
}

RandomSensor::~RandomSensor()
{
}

void RandomSensor::sensor_grow( NervousSystem *cns )
{
	nerve = cns->getNerve( "Random" );
}

void RandomSensor::sensor_prebirth_signal( RandomNumberGenerator *rng )
{
	sensor_update( false );
}

void RandomSensor::sensor_update( bool print )
{
	nerve->set( rng->drand() );
}
