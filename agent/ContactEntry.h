#pragma once

class agent;
class DataLibWriter;

class ContactEntry
{
 public:
	static void start( DataLibWriter *out );
	static void stop( DataLibWriter *out );

 public:
	ContactEntry( long step, agent *c, agent *d );

	void mate();
	void fight( agent *a );
	void give( agent *a );

	void log( DataLibWriter *out );

 private:
	class AgentInfo
	{
	public:
		void init( agent *a );
		void encode( char **buf );

	public:
		long number;
		bool mate;
		bool fight;
		bool give;
	};

	AgentInfo *get( agent *a );

	long step;
	AgentInfo c;
	AgentInfo d;
};
