#pragma once

#include "Sensor.h"

class agent;
class Nerve;
class NervousSystem;
class RandomNumberGenerator;

class CarryingSensor : public Sensor
{
 public:
	CarryingSensor( agent *self );
	virtual ~CarryingSensor();

	virtual void sensor_grow( NervousSystem *cns );
	virtual void sensor_prebirth_signal( RandomNumberGenerator *rng );
	virtual void sensor_update( bool print );

 private:
	agent *self;
	Nerve *nerve;
};
