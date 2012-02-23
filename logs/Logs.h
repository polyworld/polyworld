#include <list>
#include <vector>

#include "ContactEntry.h"
#include "Energy.h"
#include "Logger.h"
#include "simconst.h"

//===========================================================================
// Logs
//===========================================================================

// Global instance. Constructed/deleted by TSimulation.
extern class Logs *logs;

class Logs
{
 private:
	friend class Logger;

	typedef std::list<Logger *> LoggerList;
	static LoggerList _loggers;

 private:
	friend class TSimulation;

	Logs( class TSimulation *sim, proplib::Document *doc );
	virtual ~Logs();
	
	void birth( const sim::AgentBirthEvent &birth );	
	void death( const sim::AgentDeathEvent &death );

 public:
	//
	// Note that the Logger base class constructor will automatically register
	// a logger with Logs.
	//

	//===========================================================================
	// AdamiComplexityLog
	//===========================================================================
	class AdamiComplexityLog : public FileLogger
	{
	public:
		void update();

	protected:
		virtual void init( class TSimulation *sim, proplib::Document *doc );
		virtual int getMaxFileCount();

	private:
		int _frequency;

	} adamiComplexity;

	//===========================================================================
	// AgentPositionLog
	//===========================================================================
	class AgentPositionLog : public DataLibLogger
	{
	public:
		void update( class agent *a );

	protected:
		virtual void init( class TSimulation *sim, proplib::Document *doc );
		virtual void birth( const sim::AgentBirthEvent &birth );
		virtual void death( const sim::AgentDeathEvent &death );

	} agentPosition;

	//===========================================================================
	// BirthsDeathsLog
	//===========================================================================
	class BirthsDeathsLog : public FileLogger
	{
	protected:
		virtual void init( class TSimulation *sim, proplib::Document *doc );
		virtual void birth( const sim::AgentBirthEvent &birth );
		virtual void death( const sim::AgentDeathEvent &death );

	} birthsDeaths;

	//===========================================================================
	// CarryLog
	//===========================================================================
	class CarryLog : public DataLibLogger
	{
	public:
		enum CarryAction { Pickup = 0, DropRecent, DropObject };

		void update( class agent *a,
					 class gobject *obj,
					 CarryAction action );
	protected:
		virtual void init( class TSimulation *sim, proplib::Document *doc );

	} carry;

	//===========================================================================
	// CollisionLog
	//===========================================================================
	class CollisionLog : public DataLibLogger
	{
	public:
		void update( class agent *a,
					 sim::ObjectType ot );
	protected:
		virtual void init( class TSimulation *sim, proplib::Document *doc );

	} collision;

	//===========================================================================
	// ContactLog
	//===========================================================================
	class ContactLog : public DataLibLogger
	{
	public:
		void update( ContactEntry &entry );

	protected:
		virtual void init( class TSimulation *sim, proplib::Document *doc );

	} contact;

	//===========================================================================
	// EnergyLog
	//===========================================================================
	class EnergyLog : public DataLibLogger
	{
	public:
		enum EventType { Give = 0, Fight, Eat };

		void update( class agent *c,
					 class gobject *obj,
					 float neuralActivation,
					 const Energy &energy,
					 EventType elet );
	protected:
		virtual void init( class TSimulation *sim, proplib::Document *doc );

	} energy;

	//===========================================================================
	// GenomeLog
	//===========================================================================
	class GenomeLog : public AbstractFileLogger
	{
	protected:
		virtual void init( class TSimulation *sim, proplib::Document *doc );
		virtual void birth( const sim::AgentBirthEvent &birth );
	} genome;

	//===========================================================================
	// GenomeSubsetLog
	//===========================================================================
	class GenomeSubsetLog : public DataLibLogger
	{
	protected:
		virtual void init( class TSimulation *sim, proplib::Document *doc );
		virtual void birth( const sim::AgentBirthEvent &birth );

	private:
		std::vector<int> _geneIndexes;
	} genomeSubset;

	//===========================================================================
	// LifeSpanLog
	//===========================================================================
	class LifeSpanLog : public DataLibLogger
	{
	protected:
		virtual void init( class TSimulation *sim, proplib::Document *doc );
		virtual void death( const sim::AgentDeathEvent &death );
	} lifespan;

	//===========================================================================
	// SeparationLog
	//===========================================================================
	class SeparationLog : public DataLibLogger
	{
	public:
		void update( class agent *a, class agent *b );

	protected:
		virtual void init( class TSimulation *sim, proplib::Document *doc );
		virtual void death( const sim::AgentDeathEvent &death );

	} separation;

};
