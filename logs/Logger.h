#pragma once

#include <string>

#include "AgentAttachedData.h"
#include "simtypes.h"


namespace proplib { class Document; }


//===========================================================================
// Logger
//===========================================================================
class Logger
{
 public:
	bool isEnabled();

 protected:
	friend class Logs;

	virtual ~Logger();

	virtual void init( class TSimulation *sim, proplib::Document *doc ) = 0;
	virtual int getMaxFileCount();

	virtual void birth( const sim::AgentBirthEvent &birth );
	virtual void death( const sim::AgentDeathEvent &death );

 protected:
	enum StateScope
	{
		NullStateScope,
		SimulationStateScope,
		AgentStateScope
	};

	Logger();

	void initRecording( StateScope scope, class TSimulation *sim );

	long getStep();
	void mkdirs( std::string path_log );

	void *getSimulationState();
	void *getAgentState( class agent *a );

	void setSimulationState( void *state );
	void setAgentState( class agent *a, void *state );

	StateScope _scope;
	class TSimulation *_simulation;
	bool _record;

 private:
	bool _dirMade;

	union
	{
		struct
		{
		} _nullScope;

		struct
		{
			void *data;
		} _simulationScope;

		struct
		{
			AgentAttachedData::SlotHandle slotHandle;
		} _agentScope;
	};
};


//===========================================================================
// FileLogger
//===========================================================================
class FileLogger : public Logger
{
 public:
	virtual ~FileLogger();

	virtual void init( class TSimulation *sim, proplib::Document *doc ) = 0;

 protected:
	FileLogger();

	FILE *createFile( const std::string &path, const char *mode = "w" );
	FILE *getFile();

	FILE *createFile( class agent *a, const std::string &path, const char *mode = "w" );
	FILE *getFile( class agent *a );
};


//===========================================================================
// AbstractFileLogger
//===========================================================================
class AbstractFileLogger : public Logger
{
 public:
	virtual ~AbstractFileLogger();

	virtual void init( class TSimulation *sim, proplib::Document *doc ) = 0;

 protected:
	AbstractFileLogger();

	class AbstractFile *createFile( const std::string &path );
	class AbstractFile *getFile();

	class AbstractFile *createFile( class agent *a, const std::string &path );
	class AbstractFile *getFile( class agent *a );
};


//===========================================================================
// DataLibLogger
//===========================================================================
class DataLibLogger : public Logger
{
 public:
	virtual ~DataLibLogger();

	virtual void init( class TSimulation *sim, proplib::Document *doc ) = 0;

 protected:
	DataLibLogger();

	class DataLibWriter *createWriter( const std::string &path,
									   bool randomAccess = false,
									   bool singleSchema = true );
	class DataLibWriter *getWriter();

	class DataLibWriter *createWriter( class agent *a,
									   const std::string &path,
									   bool randomAccess = false,
									   bool singleSchema = true );
	class DataLibWriter *getWriter( class agent *a );
};
