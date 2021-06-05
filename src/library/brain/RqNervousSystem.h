#pragma once

#include <string>

#include "NervousSystem.h"

namespace genome
{
	class Genome;
}

class RqNervousSystem : public NervousSystem
{
 public:
	enum Mode
	{
		RANDOM,
		QUIESCENT
	};

	virtual ~RqNervousSystem();

	virtual void grow( genome::Genome *g );

	void setMode( Mode mode );

 private:
	void createInput( const std::string &name );
	void createOutput( const std::string &name );
};
