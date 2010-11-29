#pragma once

#include <stdio.h>

#include <map>
#include <string>
#include <vector>

#include "Nerve.h"
#include "Sensor.h"

class AbstractFile;
class Brain;
class RandomNumberGenerator;
namespace genome
{
	class Genome;
}

class NervousSystem
{
 public:
	typedef std::vector<Nerve *> NerveList;
	typedef NerveList::iterator nerve_iterator;

 public:
	NervousSystem();
	~NervousSystem();

	void update( bool bprint );

	void setBrain( Brain *b );
	Brain *getBrain();

	RandomNumberGenerator *getRNG();

	Nerve *add( Nerve::Type type,
				const std::string &name );

	Nerve *get( const std::string &name );
	Nerve *get( int igroup );
	Nerve *get( Nerve::Type type,
				int i );

	void add( Sensor *sensor );

	int getNerveCount();
	int getNerveCount( Nerve::Type type );
	int getNeuronCount( Nerve::Type type );

	nerve_iterator begin_nerve();
	nerve_iterator end_nerve();
	nerve_iterator begin_nerve( Nerve::Type type );
	nerve_iterator end_nerve( Nerve::Type type );

	void grow( genome::Genome *g, long agent_number, bool record_anatomy );
	void prebirthSignal();
	void startFunctional( AbstractFile *f );
	void dumpAnatomical( AbstractFile *f );

	void __test();
	
 private:
	Brain *b;
	RandomNumberGenerator *rng;

	typedef std::map<std::string, Nerve *> NerveMap;
	NerveMap map;
	NerveList all;
	NerveList lists[Nerve::__NTYPES];

	SensorList sensors;
};

