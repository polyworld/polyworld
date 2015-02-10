#include "NervousSystem.h"

#include <assert.h>
#include <string.h>

#include "AbstractFile.h"
#include "Brain.h"
#include "Genome.h"
#include "misc.h"
#include "Nerve.h"
#include "RandomNumberGenerator.h"

using namespace genome;


NervousSystem::NervousSystem()
{
	rng = RandomNumberGenerator::create( RandomNumberGenerator::NERVOUS_SYSTEM );
}

NervousSystem::~NervousSystem()
{
	itfor( NerveList, all, it )
		delete *it;

	RandomNumberGenerator::dispose( rng );

	delete b;
}

void NervousSystem::grow( Genome *g )
{
	b = g->createBrain( this );

	for( SensorList::iterator
			 it = sensors.begin(),
			 end = sensors.end();
		 it != end;
		 it++ )
	{
		(*it)->sensor_grow( this );
	}
}

void NervousSystem::update( bool bprint )
{
	for( SensorList::iterator
			 it = sensors.begin(),
			 end = sensors.end();
		 it != end;
		 it++ )
	{
		(*it)->sensor_update( bprint );
	}	

	b->update( bprint );
}

float NervousSystem::getEnergyUse()
{
	return b->getEnergyUse();
}

Brain *NervousSystem::getBrain()
{
	return b;
}

RandomNumberGenerator *NervousSystem::getRNG()
{
	return rng;
}

Nerve *NervousSystem::createNerve( Nerve::Type type, const std::string &name )
{
	Nerve *nerve = new Nerve( type,
							  name,
							  all.size() );
	map[name] = nerve;
	lists[type].push_back( nerve );
	all.push_back( nerve );

	return nerve;
}

Nerve *NervousSystem::getNerve( const std::string &name )
{
	return map[name];
}

void NervousSystem::addSensor( Sensor *sensor )
{
	sensors.push_back( sensor );
}

int NervousSystem::getNerveCount()
{
	return all.size();
}

int NervousSystem::getNerveCount( Nerve::Type type )
{
	return lists[type].size();
}

int NervousSystem::getNeuronCount( Nerve::Type type )
{
	int count = 0;

	citfor( NerveList, getNerves(type), it )
	{
		Nerve *nerve = *it;

		count += nerve->getNeuronCount();
	}

	return count;
}

const NervousSystem::NerveList &NervousSystem::getNerves()
{
	return all;
}

const NervousSystem::NerveList &NervousSystem::getNerves( Nerve::Type type )
{
	return lists[type];
}

void NervousSystem::prebirth()
{
	b->prebirth();
}

void NervousSystem::prebirthSignal()
{
	for( SensorList::iterator
			 it = sensors.begin(),
			 end = sensors.end();
		 it != end;
		 it++ )
	{
		(*it)->sensor_prebirth_signal( rng );
	}	
}

void NervousSystem::startFunctional( AbstractFile *f )
{
	for( SensorList::iterator
			 it = sensors.begin(),
			 end = sensors.end();
		 it != end;
		 it++ )
	{
		(*it)->sensor_start_functional( f );
	}	
}

void NervousSystem::dumpAnatomical( AbstractFile *f )
{
	for( SensorList::iterator
			 it = sensors.begin(),
			 end = sensors.end();
		 it != end;
		 it++ )
	{
		(*it)->sensor_dump_anatomical( f );
	}	
}
