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

	void mate( agent *a, int status );
	void fight( agent *a, int status );
	void give( agent *a, int status );

	void log( DataLibWriter *out );

 private:
	class AgentInfo
	{
	public:
		void init( agent *a );
		void encode( char **buf );

	public:
		long number;
		int mate;
		int fight;
		int give;
	};

	AgentInfo *get( agent *a );

	long step;
	AgentInfo c;
	AgentInfo d;
};
