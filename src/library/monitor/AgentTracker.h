#pragma once

#include <string>

#include "agent/AgentListener.h"
#include "utils/Signal.h"


class AgentTracker
{
 public:

	enum Mode
	{
		FITNESS,
		NUMBER
	};

	struct Parms
	{
		static Parms createFitness( int rank, bool trackTilDeath = true )
		{ Parms p; p.mode = FITNESS; p.trackTilDeath = trackTilDeath; p.fitness.rank = rank; return p; }

		static Parms createNumber( long number, bool trackTilDeath = true )
		{ Parms p; p.mode = NUMBER; p.trackTilDeath = trackTilDeath; p.number = number; return p; }

		Mode mode;
		bool trackTilDeath;

		union {
			struct {
				int rank;
			} fitness;
			long number;
		};
	};

	AgentTracker( std::string name, const Parms &parms );
	virtual ~AgentTracker();

	const char *getName();
	class agent *getTarget();
	const Parms &getParms();
	std::string getStateTitle();

    util::Signal<AgentTracker *> targetChanged;

 private:
	friend class Listener;
	friend class MonitorManager;

	void setTarget( class agent *a );

 private:
	class Listener : public AgentListener
	{
	public:
		Listener( AgentTracker *_tracker ) : tracker(_tracker ) {}
		virtual void died( class agent *a );
		AgentTracker *tracker;
	} listener;

	std::string name;
	Parms parms;
	class agent *target;
};



