#pragma once

#include <stdio.h>

#include <vector>

class NervousSystem;
class RandomNumberGenerator;

class Sensor
{
 public:
	virtual ~Sensor() {}
	virtual void sensor_grow( NervousSystem *cns ) = 0;
	virtual void sensor_prebirth_signal( RandomNumberGenerator *rng ) = 0;
	virtual void sensor_update( bool bprint ) = 0;
	virtual void sensor_start_functional( FILE *f ) {}
	virtual void sensor_dump_anatomical( FILE *f ) {}
};

typedef std::vector<Sensor *> SensorList;
