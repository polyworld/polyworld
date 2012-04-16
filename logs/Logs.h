#include <list>
#include <map>
#include <vector>

#include "cppprops.h"
#include "Energy.h"
#include "Logger.h"
#include "misc.h"
#include "simconst.h"

//===========================================================================
// Logs
//===========================================================================

// Global instance. Constructed/deleted by TSimulation.
extern class Logs *logs;

class Logs
{
 private:
	friend class TSimulation;

	Logs( class TSimulation *sim, proplib::Document *doc );
	virtual ~Logs();

 private:
	friend class Logger;

	// Invoked by Logger constructor.
	static void installLogger( Logger *logger );

	// Configure which events logger will receive.
	static void registerEvents( Logger *logger,
								sim::EventType eventTypes );

 private:
	typedef std::list<Logger *> LoggerList;
	typedef std::map< sim::EventType, LoggerList > EventRegistry;

	static LoggerList _installedLoggers;

	// Bitwise OR of all registered event types.
	static sim::EventType _registeredEvents;

	// Maps from a given event type to all registered logs.
	static EventRegistry _eventRegistry;
	
 public:
	//---------------------------------------------------------------------------
	// Logs::postEvent
	//
	// Route event to appropriate processEvent() method of Loggers registered for this
	// event type.
	//---------------------------------------------------------------------------
	template< typename T>
	void postEvent( const T &e )
	{
		// Check if any loggers are registered.
		if( _registeredEvents & e.getType() )
		{
			// Send event to loggers.
			LoggerList &loggers = _eventRegistry[ e.getType() ];
			itfor( LoggerList, loggers, it )
			{
				(*it)->processEvent( e );
			}
		}
	}

	int getMaxOpenFiles();

 private:
	//===========================================================================
	// AdamiComplexityLog
	//===========================================================================
	class AdamiComplexityLog : public FileLogger
	{
	protected:
		virtual void init( class TSimulation *sim, proplib::Document *doc );
		virtual int getMaxOpenFiles();
		virtual void processEvent( const sim::StepEndEvent &e );

	private:
		int _frequency;

	} _adamiComplexity;

	//===========================================================================
	// AgentPositionLog
	//===========================================================================
	class AgentPositionLog : public DataLibLogger
	{
	protected:
		virtual void init( class TSimulation *sim, proplib::Document *doc );
		virtual void processEvent( const sim::AgentBirthEvent &e );
		virtual void processEvent( const sim::AgentBodyUpdatedEvent &e );
		virtual void processEvent( const sim::AgentDeathEvent &e );

	private:
		enum Mode
		{
			Precise,
			Approximate
		} _mode;

	} _agentPosition;

	//===========================================================================
	// BirthsDeathsLog
	//===========================================================================
	class BirthsDeathsLog : public FileLogger
	{
	protected:
		virtual void init( class TSimulation *sim, proplib::Document *doc );
		virtual void processEvent( const sim::AgentBirthEvent &birth );
		virtual void processEvent( const sim::AgentDeathEvent &death );

	} _birthsDeaths;

	//===========================================================================
	// BrainAnatomyLog
	//===========================================================================
	class BrainAnatomyLog : public AbstractFileLogger
	{
	protected:
		virtual void init( class TSimulation *sim, proplib::Document *doc );
		virtual void processEvent( const sim::BrainGrownEvent &e );
		virtual void processEvent( const sim::AgentGrownEvent &e );
		virtual void processEvent( const sim::BrainAnalysisBeginEvent &e );
		virtual void processEvent( const sim::EpochEndEvent &e );

	private:
		void createAnatomyFile( agent *a, const char *suffix, float fitness );
		void recordEpochFittest( long step, sim::FitnessScope scope, const char *scopeName );

		bool _recordRecent;
		bool _recordBestRecent;
		bool _recordBestSoFar;

	} _brainAnatomy;

	//===========================================================================
	// BrainComplexityLog
	//===========================================================================
	class BrainComplexityLog : public DataLibLogger
	{
	public:
		virtual ~BrainComplexityLog();
	protected:

		virtual void init( class TSimulation *sim, proplib::Document *doc );
		virtual void processEvent( const sim::BrainAnalysisEndEvent &e );
		virtual void processEvent( const sim::EpochEndEvent &e );

	private:
		typedef std::map< long, float> ComplexityMap;

		void writeComplexityFile( long epoch, ComplexityMap &complexities );
		void writeBestRecent( long epoch );

		int _nseeds;
		int _seedsRemaining;
		std::string _complexityType;
		ComplexityMap _seedComplexity;
		ComplexityMap _recentComplexity;
		bool _recordBestRecent;
	} _brainComplexity;

	//===========================================================================
	// BrainFunctionLog
	//===========================================================================
	class BrainFunctionLog : public AbstractFileLogger
	{
	protected:
		virtual void init( class TSimulation *sim, proplib::Document *doc );
		virtual void processEvent( const sim::AgentGrownEvent &e );
		virtual void processEvent( const sim::BrainUpdatedEvent &e );
		virtual void processEvent( const sim::BrainAnalysisBeginEvent &e );
		virtual void processEvent( const sim::EpochEndEvent &e );

	private:
		void recordEpochFittest( long step, sim::FitnessScope scope, const char *scopeName );

		bool _recordRecent;
		bool _recordBestRecent;
		bool _recordBestSoFar;
		int _nseeds;

	} _brainFunction;

	//===========================================================================
	// CarryLog
	//===========================================================================
	class CarryLog : public DataLibLogger
	{
	protected:
		virtual void init( class TSimulation *sim, proplib::Document *doc );
		virtual void processEvent( const sim::CarryEvent &e );

	} _carry;

	//===========================================================================
	// CollisionLog
	//===========================================================================
	class CollisionLog : public DataLibLogger
	{
	protected:
		virtual void init( class TSimulation *sim, proplib::Document *doc );
		virtual void processEvent( const sim::CollisionEvent &e );

	} _collision;

	//===========================================================================
	// ContactLog
	//===========================================================================
	class ContactLog : public DataLibLogger
	{
	protected:
		virtual void init( class TSimulation *sim, proplib::Document *doc );
		virtual void processEvent( const sim::AgentContactEndEvent &e );
	private:
		void encode( const sim::AgentContactEndEvent::AgentInfo &info, char **buf );

	} _contact;

	//===========================================================================
	// EnergyLog
	//===========================================================================
	class EnergyLog : public DataLibLogger
	{
	protected:
		virtual void init( class TSimulation *sim, proplib::Document *doc );
		virtual void processEvent( const sim::EnergyEvent &e );

	} _energy;

	//===========================================================================
	// GeneStatsLog
	//===========================================================================
	class GeneStatsLog : public FileLogger
	{
	protected:
		virtual void init( class TSimulation *sim, proplib::Document *doc );
		virtual void processEvent( const sim::StepEndEvent &e );
	} _geneStats;

	//===========================================================================
	// GenomeLog
	//===========================================================================
	class GenomeLog : public AbstractFileLogger
	{
	protected:
		virtual void init( class TSimulation *sim, proplib::Document *doc );
		virtual void processEvent( const sim::AgentBirthEvent &birth );
	} _genome;

	//===========================================================================
	// GenomeMetaLog
	//===========================================================================
	class GenomeMetaLog : public FileLogger
	{
	protected:
		virtual void init( class TSimulation *sim, proplib::Document *doc );
		virtual void processEvent( const sim::SimInitedEvent &e );
	} _genomeMeta;

	//===========================================================================
	// GenomeSubsetLog
	//===========================================================================
	class GenomeSubsetLog : public DataLibLogger
	{
	protected:
		virtual void init( class TSimulation *sim, proplib::Document *doc );
		virtual void processEvent( const sim::AgentBirthEvent &birth );

	private:
		std::vector<int> _geneIndexes;
	} _genomeSubset;

	//===========================================================================
	// LifeSpanLog
	//===========================================================================
	class LifeSpanLog : public DataLibLogger
	{
	protected:
		virtual void init( class TSimulation *sim, proplib::Document *doc );
		virtual void processEvent( const sim::AgentDeathEvent &death );
	} _lifespan;

	//===========================================================================
	// SeparationLog
	//===========================================================================
	class SeparationLog : public DataLibLogger
	{
	protected:
		virtual void init( class TSimulation *sim, proplib::Document *doc );
		virtual void processEvent( const sim::AgentBirthEvent &birth );
		virtual void processEvent( const sim::AgentContactBeginEvent &e );
		virtual void processEvent( const sim::AgentDeathEvent &death );
		virtual void processEvent( const sim::StepEndEvent &e );

	private:
		enum { Contact, All } _mode;
		std::list<class agent *> _births;
	} _separation;

};
