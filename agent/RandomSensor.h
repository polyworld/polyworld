#pragma once

#include "Sensor.h"

class Nerve;
class NervousSystem;
class RandomNumberGenerator;

class RandomSensor : public Sensor
{
 public:
	RandomSensor( RandomNumberGenerator *rng );
	virtual ~RandomSensor();

	virtual void sensor_grow( NervousSystem *cns );
	virtual void sensor_prebirth_signal( RandomNumberGenerator *rng );
	virtual void sensor_update( bool print );

 private:
	Nerve *nerve;
	RandomNumberGenerator *rng;
};
