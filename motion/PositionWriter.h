#ifndef POSITION_WRITER_H
#define POSITION_WRITER_H

#include "agent.h"

//===========================================================================
// PositionWriter
//===========================================================================
class PositionWriter
{
 public:
	PositionWriter(bool *_record);
	~PositionWriter();

 public:
	void birth(long step,
		   agent *c);
	void death(long step,
		   agent *c);
	void step(long step);

 private:
	bool &record;
};


#ifdef POSITION_WRITER_CP

#include <stdio.h>

//===========================================================================
// PositionFile
//===========================================================================
class PositionFile
{
 public:
	PositionFile(long step,
		     agent *c);
	~PositionFile();

 public:
	void step(long step);

 private:
	long step_birth;
	agent *c;
	FILE *file;
  
};

#endif // POSITION_WRITER_CP

#endif // POSITION_WRITER_H
