#pragma once

#include <list>
#include <ostream>
#include <string>


typedef std::list<class Monitor *> Monitors;
typedef std::list<class AgentTracker *> AgentTrackers;

class MonitorManager
{
 public:
	MonitorManager( class TSimulation *simulation, std::string monitorPath );
	virtual ~MonitorManager();

	const Monitors &getMonitors();
	const AgentTrackers &getAgentTrackers();
	class AgentTracker *findAgentTracker( std::string name );

	void step();

	void dump( std::ostream &out );

 private:
	void addMonitor( class Monitor *monitor );
	void addAgentTracker( class AgentTracker *tracker );

	class TSimulation *simulation;
	Monitors monitors;
	AgentTrackers agentTrackers;
};
