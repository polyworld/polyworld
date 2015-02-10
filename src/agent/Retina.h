#pragma once

#include "Brain.h"
#include "Sensor.h"

class Nerve;
class NervousSystem;
class RandomNumberGenerator;

class Retina : public Sensor
{
 public:
	Retina( int width );
	virtual ~Retina();

	virtual void sensor_grow( NervousSystem *cns );
	virtual void sensor_prebirth_signal( RandomNumberGenerator *rng );
	virtual void sensor_update( bool print );
	virtual void sensor_start_functional( AbstractFile *f );
	virtual void sensor_dump_anatomical( AbstractFile *f );

	void updateBuffer( short x, short y, short width, short height );

	const unsigned char *getBuffer();

 private:
	int width;
	unsigned char *buf;
#if PrintBrain
	bool bprinted;
#endif

	class Channel
	{
	public:

		const char *name;
		unsigned char *buf;
		Nerve *nerve;
		float xwidth;
		int index;
		int xintwidth;
		int numneurons;

		void init( Retina *retina,
				   NervousSystem *cns,
				   int index,
				   const char *name );

		void update( bool print );

		void start_functional( AbstractFile *f );
		void dump_anatomical( AbstractFile *f );

	} channels[3];
};
