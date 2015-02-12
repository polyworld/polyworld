#pragma once

#include <assert.h>

#include <string>

#include "AgentAttachedData.h"
#include "simtypes.h"


namespace proplib { class Document; }



//===========================================================================
// Logger
//
// Base class for all loggers. Provides ability to register for various
// events from the simulation. Derived classes must inform which events are
// to be received via initRecording(), and must also override the appropriate
// processEvent(). This class is also capable of maintaining state for derived 
// classes (e.g. FILE *), where that state can be associated with the lifetime
// of the simulation or with each agent.
//
// Derivations of this class will ultimately be members of the Logs class.
// By defining a Logger derivation as a field of Logs, the Logger's
// default constructor will be implicitly invoked, which ultimately will
// call Logs::installLogger(). That is, to install a logger, just define
// it in class Logs.
//
//===========================================================================
class Logger
{
 protected:
	friend class Logs;

	virtual ~Logger();

	virtual void init( class TSimulation *sim, proplib::Document *doc ) = 0;
	virtual int getMaxOpenFiles();

	// 
	// Derived classes must override any of these methods for which they register for events.
	// e.g. if a derived class invokes initRecording(..., sim::Event_AgentBirth), then it must
	// override processEvent( AgentBirthEvent )
	// 
	virtual void processEvent( const sim::SimInitedEvent &e ) { assert(false); }
	virtual void processEvent( const sim::AgentBirthEvent &e ) { assert(false); }
	virtual void processEvent( const sim::BrainGrownEvent &e ) { assert(false); }
	virtual void processEvent( const sim::AgentGrownEvent &e ) { assert(false); }
	virtual void processEvent( const sim::BrainUpdatedEvent &e ) { assert(false); }
	virtual void processEvent( const sim::AgentBodyUpdatedEvent &e ) { assert(false); }
	virtual void processEvent( const sim::AgentContactBeginEvent &e ) { assert(false); }
	virtual void processEvent( const sim::AgentContactEndEvent &e ) { assert(false); }
	virtual void processEvent( const sim::CollisionEvent &e ) { assert(false); }
	virtual void processEvent( const sim::CarryEvent &e ) { assert(false); }
	virtual void processEvent( const sim::EnergyEvent &e ) { assert(false); }
	virtual void processEvent( const sim::AgentDeathEvent &e ) { assert(false); }
	virtual void processEvent( const sim::BrainAnalysisBeginEvent &e ) { assert(false); }
	virtual void processEvent( const sim::BrainAnalysisEndEvent &e ) { assert(false); }
	virtual void processEvent( const sim::StepEndEvent &e ) { assert(false); }
	virtual void processEvent( const sim::EpochEndEvent &e ) { assert(false); }

 protected:
	enum StateScope
	{
		// The base logger class will not maintain any state (e.g. FILE *) on behalf
		// of the derived logger class.
		NullStateScope,
		// The base logger class will maintain state (e.g. FILE *) for the
		// duration of the simulation.
		SimulationStateScope,
		// The base logger class will maintain state (e.g. FILE *) for each agent.
		AgentStateScope
	};
	
	Logger();

	void initRecording( class TSimulation *sim,
						StateScope scope,
						sim::EventType events = sim::Event_None );

	long getStep();

	void *getSimulationState();
	void *getAgentState( class agent *a );

	void setSimulationState( void *state );
	void setAgentState( class agent *a, void *state );

	StateScope _scope;
	class TSimulation *_simulation;
	bool _record;

 private:
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
			// This gives us a reference to a per-agent opaque pointer.
			AgentAttachedData::SlotHandle slotHandle;
		} _agentScope;
	};
};


//===========================================================================
// FileLogger
//
// A convenient base class for loggers that use stdio FILE
//
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
//
// A convenient base class for loggers that use AbstractFile
//
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
//
// A convenient base class for loggers that use stdio DataLibWriter
//
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
