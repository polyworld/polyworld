#pragma once

#include <list>


class AgentListener
{
 public:
	virtual ~AgentListener() {}

	virtual void died( class agent * ) {}
};


typedef std::list<AgentListener *> AgentListeners;
