#pragma once

#include "Sensor.h"

class agent;
class Nerve;
class NervousSystem;
class RandomNumberGenerator;

class EnergySensor : public Sensor
{
 public:
	EnergySensor( agent *self );
	virtual ~EnergySensor();

	virtual void sensor_grow( NervousSystem *cns );
	virtual void sensor_prebirth_signal( RandomNumberGenerator *rng );
	virtual void sensor_update( bool print );

 private:
	agent *self;
	Nerve *nerve;
};
