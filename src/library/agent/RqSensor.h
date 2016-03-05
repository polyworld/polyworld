#pragma once

#include <string>

#include "brain/RqNervousSystem.h"
#include "brain/Sensor.h"

class Nerve;
class NervousSystem;
class RandomNumberGenerator;

class RqSensor : public Sensor
{
 public:
	RqSensor( const std::string &name, RandomNumberGenerator *rng );

	virtual void sensor_grow( NervousSystem *cns );
	virtual void sensor_prebirth_signal( RandomNumberGenerator *rng );
	virtual void sensor_update( bool print );

	void setMode( RqNervousSystem::Mode mode );

 private:
	std::string name;
	Nerve *nerve;
	RandomNumberGenerator *rng;
	RqNervousSystem::Mode mode;
};
