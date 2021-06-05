#include "RqSensor.h"

#include <assert.h>
#include <string.h>

#include "brain/Brain.h"
#include "brain/Nerve.h"
#include "brain/NervousSystem.h"
#include "utils/misc.h"
#include "utils/RandomNumberGenerator.h"

#define GaussianRq 0

RqSensor::RqSensor( const std::string &name, RandomNumberGenerator *rng )
{
	this->name = name;
	this->rng = rng;
	mode = RqNervousSystem::RANDOM;
}

void RqSensor::sensor_grow( NervousSystem *cns )
{
	nerve = cns->getNerve( name );
}

void RqSensor::sensor_prebirth_signal( RandomNumberGenerator *rng )
{
	sensor_update( false );
}

void RqSensor::sensor_update( bool print )
{
	int neuronCount = nerve->getNeuronCount();
	switch( mode )
	{
	case RqNervousSystem::RANDOM:
		for( int i = 0; i < neuronCount; i++ )
		{
			#if GaussianRq
				nerve->set( i, logistic( rng->nrand(), Brain::config.logisticSlope ) );
			#else
				nerve->set( i, rng->drand() );
			#endif
		}
		break;
	case RqNervousSystem::QUIESCENT:
		for( int i = 0; i < neuronCount; i++ )
		{
			nerve->set( i, 0.0 );
		}
		break;
	default:
		assert( false );
		break;
	}
}

void RqSensor::setMode( RqNervousSystem::Mode mode )
{
	this->mode = mode;
}
