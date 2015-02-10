#pragma once

#include <string>

#include <QObject>

#include "AgentListener.h"


class AgentTracker : public QObject
{
	Q_OBJECT

 public:

	enum Mode
	{
		FITNESS
	};

	struct Parms
	{
		static Parms createFitness( int rank, bool trackTilDeath = true )
		{ Parms p; p.mode = FITNESS; p.trackTilDeath = trackTilDeath; p.fitness.rank = rank; return p; }

		Mode mode;
		bool trackTilDeath;

		union {
			struct {
				int rank;
			} fitness;
		};
	};

	AgentTracker( std::string name, const Parms &parms );
	virtual ~AgentTracker();

	const char *getName();
	class agent *getTarget();
	const Parms &getParms();
	std::string getStateTitle();

 private:
	friend class Listener;
	friend class MonitorManager;

	void setTarget( class agent *a );

 signals:
	void targetChanged( AgentTracker *tracker );

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



