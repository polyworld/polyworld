#pragma once

#include <stdio.h>

#include <vector>

class AbstractFile;
class NervousSystem;
class RandomNumberGenerator;

class Sensor
{
 public:
	virtual ~Sensor() {}
	virtual void sensor_grow( NervousSystem *cns ) = 0;
	virtual void sensor_prebirth_signal( RandomNumberGenerator *rng ) = 0;
	virtual void sensor_update( bool bprint ) = 0;
	virtual void sensor_start_functional( AbstractFile *f ) {}
	virtual void sensor_dump_anatomical( AbstractFile *f ) {}
};

typedef std::vector<Sensor *> SensorList;
