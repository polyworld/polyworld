#pragma once

#include "Sensor.h"

class agent;
class Nerve;
class NervousSystem;
class RandomNumberGenerator;

class MateWaitSensor : public Sensor
{
 public:
	MateWaitSensor( agent *self,
					long mateWait );
	virtual ~MateWaitSensor();

	virtual void sensor_grow( NervousSystem *cns );
	virtual void sensor_prebirth_signal( RandomNumberGenerator *rng );
	virtual void sensor_update( bool print );

 private:
	agent *self;
	long mateWait;
	Nerve *nerve;
};
