#pragma once

#include <map>

class agent;
class DataLibWriter;

class SeparationCache
{
 public:
	SeparationCache();

 public:
	void start( DataLibWriter *log );
	void stop();

	void birth( agent *a );
	void death( agent *a );

	float separation( agent *a, agent *b );

 private:
	void record( agent *a );

 private:
	DataLibWriter *log;

	class AgentData
	{
	public:
		typedef std::map<long, float> SeparationMap;

		SeparationMap separationMap;
	};
};
