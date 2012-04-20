#include "NervousSystem.h"

#include <assert.h>
#include <string.h>

#include "AbstractFile.h"
#include "Brain.h"
#include "Nerve.h"
#include "RandomNumberGenerator.h"

using namespace genome;


NervousSystem::NervousSystem()
{
	rng = RandomNumberGenerator::create( RandomNumberGenerator::NERVOUS_SYSTEM );
}

NervousSystem::~NervousSystem()
{
	for( nerve_iterator
			 it = begin_nerve(),
			 end = this->end_nerve();
		 it != end;
		 it++ )
	{
		delete *it;
	}

	RandomNumberGenerator::dispose( rng );
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

	b->Update( bprint );
}

void NervousSystem::setBrain( Brain *b )
{
	this->b = b;
}

Brain *NervousSystem::getBrain()
{
	return b;
}

RandomNumberGenerator *NervousSystem::getRNG()
{
	return rng;
}

Nerve *NervousSystem::add( Nerve::Type type,
						   const std::string &name )
{
	Nerve *nerve = new Nerve( type,
							  name,
							  all.size() );
	map[name] = nerve;
	lists[type].push_back( nerve );
	all.push_back( nerve );

	return nerve;
}

Nerve *NervousSystem::get( const std::string &name )
{
	return map[name];
}

Nerve *NervousSystem::get( int igroup )
{
	return all[igroup];
}

Nerve *NervousSystem::get( Nerve::Type type,
						   int i )
{
	NerveList &list = lists[type];
	if( i < 0 )
	{
		i = list.size() - i;
	}

	return list[i];
}

void NervousSystem::add( Sensor *sensor )
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

	for( nerve_iterator
			 it = begin_nerve( type ),
			 end = this->end_nerve( type );
		 it != end;
		 it++ )
	{
		Nerve *nerve = *it;

		count += nerve->getNeuronCount();
	}

	return count;
}

NervousSystem::nerve_iterator NervousSystem::begin_nerve()
{
	return all.begin();
}

NervousSystem::nerve_iterator NervousSystem::end_nerve()
{
	return all.end();
}

NervousSystem::nerve_iterator NervousSystem::begin_nerve( Nerve::Type type )
{
	return lists[type].begin();
}

NervousSystem::nerve_iterator NervousSystem::end_nerve( Nerve::Type type )
{
	return lists[type].end();
}

void NervousSystem::grow( Genome *g )
{
	b->grow( g );

	for( SensorList::iterator
			 it = sensors.begin(),
			 end = sensors.end();
		 it != end;
		 it++ )
	{
		(*it)->sensor_grow( this );
	}
}

void NervousSystem::prebirth()
{
	b->Prebirth();
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

void NervousSystem::__test()
{
	NervousSystem ifs;

	ifs.add( Nerve::INPUT,
			 "red" );

	ifs.get( "red" );
}
