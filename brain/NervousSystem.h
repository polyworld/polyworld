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

 public:
	NervousSystem();
	~NervousSystem();

	void grow( genome::Genome *g );
	void update( bool bprint );

	RandomNumberGenerator *getRNG();
	Brain *getBrain();
	float getEnergyUse();

	Nerve *createNerve( Nerve::Type type, const std::string &name );
	Nerve *getNerve( const std::string &name );

	void addSensor( Sensor *sensor );

	int getNerveCount();
	int getNerveCount( Nerve::Type type );
	int getNeuronCount( Nerve::Type type );

	const NerveList &getNerves();
	const NerveList &getNerves( Nerve::Type type );

	void prebirth();
	void prebirthSignal();
	void startFunctional( AbstractFile *f );
	void dumpAnatomical( AbstractFile *f );
	
 private:
	Brain *b;
	RandomNumberGenerator *rng;

	typedef std::map<std::string, Nerve *> NerveMap;
	NerveMap map;
	NerveList all;
	NerveList lists[Nerve::__NTYPES];

	SensorList sensors;
};

