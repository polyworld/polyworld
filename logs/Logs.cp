// todo prune includes

#include "Logs.h"

#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include "adami.h"
#include "agent.h"
#include "datalib.h"
#include "GenomeUtil.h"
#include "globals.h"
#include "misc.h"
#include "proplib.h"
#include "SeparationCache.h"
#include "Simulation.h"

using namespace datalib;
using namespace genome;
using namespace proplib;
using namespace std;


//===========================================================================
// Logs
//===========================================================================
Logs *logs = NULL;

Logs::LoggerList Logs::_installedLoggers;
sim::EventType Logs::_registeredEvents;
Logs::EventRegistry Logs::_eventRegistry;

//---------------------------------------------------------------------------
// Logs::Logs
//---------------------------------------------------------------------------
Logs::Logs( TSimulation *sim, proplib::Document *doc )
{
	assert( logs == NULL );

	_registeredEvents = 0;
	itfor( LoggerList, _installedLoggers, it )
	{
		(*it)->init( sim, doc );
	}
}

//---------------------------------------------------------------------------
// Logs::~Logs
//---------------------------------------------------------------------------
Logs::~Logs()
{
	// We don't have to delete the loggers since they're part of this datastructure.
	_installedLoggers.clear();
	logs = NULL;
}

//---------------------------------------------------------------------------
// Logs::installLogger
//---------------------------------------------------------------------------
void Logs::installLogger( Logger *logger )
{
	_installedLoggers.push_back( logger );
}

//---------------------------------------------------------------------------
// Logs::registerEvents
//---------------------------------------------------------------------------
void Logs::registerEvents( Logger *logger, sim::EventType eventTypes )
{
	int nbits = sizeof(sim::EventType) * 8;

	for( int bit = 0; bit < nbits; bit++ )
	{
		sim::EventType type = sim::EventType(1) << bit;

		if( eventTypes & type )
		{
			_eventRegistry[ type ].push_back( logger );
		}
	}

	_registeredEvents |= eventTypes;
}

//===========================================================================
// AdamiComplexityLog
//===========================================================================

//---------------------------------------------------------------------------
// Logs::AdamiComplexityLog::init
//---------------------------------------------------------------------------
void Logs::AdamiComplexityLog::init( TSimulation *sim, Document *doc )
{
	if( doc->get("RecordAdamiComplexity") )
	{
		_frequency = doc->get( "AdamiComplexityRecordFrequency" );

		initRecording( sim, NullStateScope, sim::Event_StepEnd );
	}
}

//---------------------------------------------------------------------------
// Logs::AdamiComplexityLog::getMaxFileCount
//---------------------------------------------------------------------------
int Logs::AdamiComplexityLog::getMaxFileCount()
{
	return 4;
}

//---------------------------------------------------------------------------
// Logs::AdamiComplexityLog::processEvent
//---------------------------------------------------------------------------
void Logs::AdamiComplexityLog::processEvent( const StepEndEvent &e )
{
	if( getStep() % _frequency == 0 )
	{
		FILE *FileOneBit = createFile( "run/genome/AdamiComplexity-1bit.txt", "a" );
		FILE *FileTwoBit = createFile( "run/genome/AdamiComplexity-2bit.txt", "a" );
		FILE *FileFourBit = createFile( "run/genome/AdamiComplexity-4bit.txt", "a" );
		FILE *FileSummary = createFile( "run/genome/AdamiComplexity-summary.txt", "a" );

		computeAdamiComplexity( getStep(),
								FileOneBit,
								FileTwoBit,
								FileFourBit,
								FileSummary );

		// Done computing AdamiComplexity.  Close our log files.
		fclose( FileOneBit );
		fclose( FileTwoBit );
		fclose( FileFourBit );
		fclose( FileSummary );
	}
}


//===========================================================================
// AgentPositionLog
//===========================================================================

//---------------------------------------------------------------------------
// Logs::AgentPositionLog::init
//---------------------------------------------------------------------------
void Logs::AgentPositionLog::init( TSimulation *sim, Document *doc )
{
	if( doc->get("RecordPosition") )
	{
		initRecording( sim,
					   AgentStateScope,
					   sim::Event_Birth | sim::Event_AgentBodyUpdated | sim::Event_Death );
	}
}

//---------------------------------------------------------------------------
// Logs::AgentPositionLog::processEvent
//---------------------------------------------------------------------------
void Logs::AgentPositionLog::processEvent( const sim::AgentBirthEvent &e )
{
	char path[512];
	sprintf( path,
			 "run/motion/position/agents/position_%ld.txt",
			 e.a->getTypeNumber() );

	DataLibWriter *writer = createWriter( e.a, path, true, false );

	static const char *colnames[] = {"Timestep", "x", "y", "z", NULL};
	static const datalib::Type coltypes[] = {datalib::INT, datalib::FLOAT, datalib::FLOAT, datalib::FLOAT};

	writer->beginTable( "Positions",
						colnames,
						coltypes );
}

//---------------------------------------------------------------------------
// Logs::AgentPositionLog::processEvent
//---------------------------------------------------------------------------
void Logs::AgentPositionLog::processEvent( const AgentBodyUpdatedEvent &e )
{
	getWriter( e.a )->addRow( getStep(),
							  e.a->x(),
							  e.a->y(),
							  e.a->z() );
}

//---------------------------------------------------------------------------
// Logs::AgentPositionLog::processEvent
//---------------------------------------------------------------------------
void Logs::AgentPositionLog::processEvent( const sim::AgentDeathEvent &e )
{
	delete getWriter( e.a );
}


//===========================================================================
// BirthsDeathsLog
//===========================================================================

//---------------------------------------------------------------------------
// Logs::BirthsDeathsLog::init
//---------------------------------------------------------------------------
void Logs::BirthsDeathsLog::init( TSimulation *sim, Document *doc )
{
	if( doc->get("RecordBirthsDeaths") )
	{
		initRecording( sim, SimulationStateScope, sim::Event_Birth | sim::Event_Death );

		createFile( "run/BirthsDeaths.log" );
		fprintf( getFile(), "%% Timestep Event Agent# Parent1 Parent2\n" );
	}
}

//---------------------------------------------------------------------------
// Logs::BirthsDeathsLog::processEvent
//---------------------------------------------------------------------------
void Logs::BirthsDeathsLog::processEvent( const sim::AgentBirthEvent &birth )
{
	switch( birth.reason )
	{
	case LifeSpan::BR_SIMINIT:
		// no-op
		break;
	case LifeSpan::BR_NATURAL:
	case LifeSpan::BR_LOCKSTEP:
		fprintf( getFile(),
				 "%ld BIRTH %ld %ld %ld\n",
				 getStep(),
				 birth.a->Number(),
				 birth.parent1->Number(),
				 birth.parent2->Number() );
		break;
	case LifeSpan::BR_VIRTUAL:
		fprintf( getFile(),
				 "%ld VIRTUAL %d %ld %ld\n",
				 getStep(),
				 0,	// agent IDs start with 1, so this also identifies virtual births
				 birth.parent1->Number(),
				 birth.parent2->Number() );
		break;
	case LifeSpan::BR_CREATE:
		fprintf( getFile(),
				 "%ld CREATION %ld\n",
				 getStep(),
				 birth.a->Number() );
		break;
	default:
		assert( false );
	}
}

//---------------------------------------------------------------------------
// Logs::BirthsDeathsLog::processEvent
//---------------------------------------------------------------------------
void Logs::BirthsDeathsLog::processEvent( const sim::AgentDeathEvent &death )
{
	if( death.reason != LifeSpan::DR_SIMEND )
	{
		fprintf( getFile(), "%ld DEATH %ld\n", getStep(), death.a->Number() );
	}
}


//===========================================================================
// CarryLog
//===========================================================================

//---------------------------------------------------------------------------
// Logs::CarryLog::init
//---------------------------------------------------------------------------
void Logs::CarryLog::init( TSimulation *sim, Document *doc )
{
	if( doc->get("RecordCarry") )
	{
		initRecording( sim, SimulationStateScope, sim::Event_Carry );

		DataLibWriter *writer = createWriter( "run/events/carry.log" );

		const char *colnames[] =
			{
				"T",
				"Agent",
				"Action",
				"ObjectType",
				"ObjectNumber",
				NULL
			};
		const datalib::Type coltypes[] =
			{
				datalib::INT,
				datalib::INT,
				datalib::STRING,
				datalib::STRING,
				datalib::INT
			};

		writer->beginTable( "Carry",
							colnames,
							coltypes );
	}
}

//---------------------------------------------------------------------------
// Logs::CarryLog::processEvent
//---------------------------------------------------------------------------
void Logs::CarryLog::processEvent( const CarryEvent &e )
{
	static const char *actions[] = {"P",
									"D",
									"Do"};

	const char *objectType;
	switch( e.obj->getType() )
	{
	case AGENTTYPE: objectType = "A"; break;
	case FOODTYPE: objectType = "F"; break;
	case BRICKTYPE: objectType = "B"; break;
	default: assert(false);
	}

	getWriter()->addRow( getStep(),
						 e.a->Number(),
						 actions[e.action],
						 objectType,
						 e.obj->getTypeNumber() );
}


//===========================================================================
// CollisionLog
//===========================================================================

//---------------------------------------------------------------------------
// Logs::CollisionLog::init
//---------------------------------------------------------------------------
void Logs::CollisionLog::init( TSimulation *sim, Document *doc )
{
	if( doc->get("RecordCollisions") )
	{
		initRecording( sim,
					   SimulationStateScope,
					   sim::Event_Collision );

		DataLibWriter *writer = createWriter( "run/events/collisions.log" );

		const char *colnames[] =
			{
				"Step",
				"Agent",
				"Type",
				NULL
			};
		const datalib::Type coltypes[] =
			{
				datalib::INT,
				datalib::INT,
				datalib::STRING
			};

		writer->beginTable( "Collisions",
							colnames,
							coltypes );
	}
}

//---------------------------------------------------------------------------
// Logs::CollisionLog::processEvent
//---------------------------------------------------------------------------
void Logs::CollisionLog::processEvent( const sim::CollisionEvent &e )
{
	// We store type as a string since this data will most likely
	// be processed by a script.
	static const char *names[] = {"agent",
								  "food",
								  "brick",
								  "barrier",
								  "edge"};

	getWriter()->addRow( getStep(),
						 e.a->Number(),
						 names[e.ot] );
}


//===========================================================================
// ContactLog
//===========================================================================

//---------------------------------------------------------------------------
// Logs::ContactLog::init
//---------------------------------------------------------------------------
void Logs::ContactLog::init( TSimulation *sim, Document *doc )
{
	if( doc->get("RecordContacts") )
	{
		initRecording( sim, SimulationStateScope, sim::Event_ContactEnd );

		DataLibWriter *writer = createWriter( "run/events/contacts.log" );

		const char *colnames[] =
			{
				"Timestep",
				"Agent1",
				"Agent2",
				"Events",
				NULL
			};

		const datalib::Type coltypes[] =
			{
				datalib::INT,
				datalib::INT,
				datalib::INT,
				datalib::STRING,
			};

		writer->beginTable( "Contacts",
							colnames,
							coltypes );
	}
}

//---------------------------------------------------------------------------
// Logs::ContactLog::processEvent
//---------------------------------------------------------------------------
void Logs::ContactLog::processEvent( const AgentContactEndEvent &e )
{
	char buf[32];
	char *b = buf;

	encode( e.c, &b );
	*(b++) = 'C';
	encode( e.d, &b );
	*(b++) = 0;

	getWriter()->addRow( getStep(),
						 e.c.number,
						 e.d.number,
						 buf );
}

//---------------------------------------------------------------------------
// Logs::ContactLog::encode
//---------------------------------------------------------------------------
void Logs::ContactLog::encode( const sim::AgentContactEndEvent::AgentInfo &info,
							   char **buf )
{
	char *b = *buf;

	if( info.mate )
	{
		*(b++) = 'M';
		
#define __SET( PREVENT_TYPE, CODE ) if( info.mate & MATE__PREVENTED__##PREVENT_TYPE ) *(b++) = CODE

		__SET( PARTNER, 'p' );
		__SET( CARRY, 'c' );
		__SET( MATE_WAIT, 'w' );
		__SET( ENERGY, 'e' );
		__SET( EAT_MATE_SPAN, 'f' );
		__SET( EAT_MATE_MIN_DISTANCE, 'i' );
		__SET( MAX_DOMAIN, 'd' );
		__SET( MAX_WORLD, 'x' );
		__SET( MAX_METABOLISM, 't' );
		__SET( MISC, 'm' );
		__SET( MAX_VELOCITY, 'v' );

#undef __SET

#ifdef OF1
		// implement
		assert( false );
#endif
	}

	if( info.fight )
	{
		*(b++) = 'F';

#define __SET( PREVENT_TYPE, CODE ) if( info.fight & FIGHT__PREVENTED__##PREVENT_TYPE ) *(b++) = CODE

		__SET( CARRY, 'c' );
		__SET( SHIELD, 's' );
		__SET( POWER, 'p' );

#undef __SET
	}

	if( info.give )
	{
		*(b++) = 'G';

#define __SET( PREVENT_TYPE, CODE ) if( info.give & GIVE__PREVENTED__##PREVENT_TYPE ) *(b++) = CODE

		__SET( CARRY, 'c' );
		__SET( ENERGY, 'e' );

#undef __SET
	}

	*buf = b;
}


//===========================================================================
// EnergyLog
//===========================================================================

//---------------------------------------------------------------------------
// Logs::EnergyLog::init
//---------------------------------------------------------------------------
void Logs::EnergyLog::init( TSimulation *sim, Document *doc )
{
	if( doc->get("RecordEnergy") )
	{
		initRecording( sim, SimulationStateScope, sim::Event_Energy );

		DataLibWriter *writer = createWriter( "run/events/energy.log" );

		const char *colnames_template[] =
			{
				"T",
				"Agent",
				"EventType",
				"ObjectNumber",
				"NeuralActivation",
			};
		const datalib::Type coltypes_template[] =
			{
				datalib::INT,
				datalib::INT,
				datalib::STRING,
				datalib::INT,
				datalib::FLOAT,
			};

		int lenTemplate = sizeof(colnames_template) / sizeof(char *);

		// Now add columns for the energy types, where the number is not known at compile-time.
		const char **colnames = new const char *[ lenTemplate + globals::numEnergyTypes + 1 ];
		datalib::Type *coltypes = new datalib::Type[ lenTemplate + globals::numEnergyTypes ];
		for( int i = 0; i < lenTemplate; i++ )
		{
			colnames[i] = colnames_template[i];
			coltypes[i] = coltypes_template[i];
		}
		for( int i = 0; i < globals::numEnergyTypes; i++ )
		{
			char buf[128];
			sprintf( buf, "Energy%d\n", i );
			colnames[ lenTemplate + i ] = strdup( buf );

			coltypes[ lenTemplate + i ] = datalib::FLOAT;
		}
		colnames[ lenTemplate + globals::numEnergyTypes ] = NULL;

		writer->beginTable( "Energy",
							colnames,
							coltypes );
	}
}

//---------------------------------------------------------------------------
// Logs::EnergyLog::processEvent
//---------------------------------------------------------------------------
void Logs::EnergyLog::processEvent( const sim::EnergyEvent &e )
{
	static const char *actionNames[] = {"G",
									   "F",
									   "E"};
	const char *actionName = actionNames[ e.action ];

	Variant coldata[ 5 + globals::numEnergyTypes ];
	coldata[0] = getStep();
	coldata[1] = e.a->Number();
	coldata[2] = actionName;
	coldata[3] = (long)e.obj->getTypeNumber();
	coldata[4] = e.neuralActivation;
	for( int i = 0; i < globals::numEnergyTypes; i++ )
	{
		coldata[5 + i] = e.energy[i];
	}

	getWriter()->addRow( coldata );
}


//===========================================================================
// GeneStatsLog
//===========================================================================

//---------------------------------------------------------------------------
// Logs::GeneStatsLog::init
//---------------------------------------------------------------------------
void Logs::GeneStatsLog::init( TSimulation *sim, Document *doc )
{
	if( doc->get("RecordGeneStats") )
	{
		initRecording( sim, SimulationStateScope, sim::Event_StepEnd );

		sim->getGeneStats().init( sim->GetMaxAgents() );

		FILE *f = createFile( "run/genome/genestats.txt" );

		fprintf( f, "%d\n", GenomeUtil::schema->getMutableSize() );
	}
}

//---------------------------------------------------------------------------
// Logs::GeneStatsLog::processEvent
//---------------------------------------------------------------------------
void Logs::GeneStatsLog::processEvent( const sim::StepEndEvent &e )
{
	float *mean = _simulation->getGeneStats().getMean();
	float *stddev = _simulation->getGeneStats().getStddev();
	int ngenes = GenomeUtil::schema->getMutableSize();

	FILE *f = getFile();

	fprintf( f, "%ld", getStep() );

	for( int i = 0; i < ngenes; i++ )
	{
		fprintf( f, " %.1f,%.1f", mean[i], stddev[i] );
	}

	fprintf( f, "\n" );
}


//===========================================================================
// GenomeLog
//===========================================================================

//---------------------------------------------------------------------------
// Logs::GenomeLog::init
//---------------------------------------------------------------------------
void Logs::GenomeLog::init( TSimulation *sim, Document *doc )
{
	if( doc->get("RecordGenomes") )
	{
		initRecording( sim, NullStateScope, sim::Event_Birth );
	}
}

//---------------------------------------------------------------------------
// Logs::GenomeLog::processEvent
//---------------------------------------------------------------------------
void Logs::GenomeLog::processEvent( const sim::AgentBirthEvent &birth )
{
	if( birth.reason != LifeSpan::BR_VIRTUAL )
	{
		char path[256];
		sprintf( path, "run/genome/agents/genome_%ld.txt", birth.a->Number() );

		AbstractFile *out = createFile( path );
		birth.a->Genes()->dump( out );
		delete out;
	}
}


//===========================================================================
// GenomeMetaLog
//===========================================================================

//---------------------------------------------------------------------------
// Logs::GenomeMetaLog::init
//---------------------------------------------------------------------------
void Logs::GenomeMetaLog::init( TSimulation *sim, Document *doc )
{
	initRecording( sim, NullStateScope, sim::Event_SimInited );
}

//---------------------------------------------------------------------------
// Logs::GenomeMetaLog::processEvent
//---------------------------------------------------------------------------
void Logs::GenomeMetaLog::processEvent( const sim::SimInitedEvent &birth )
{
	{
		FILE* f = createFile( "run/genome/meta/geneindex.txt" );
		
		GenomeUtil::schema->printIndexes( f );
		
		fclose( f );
	}
	{
		FILE* f = createFile( "run/genome/meta/genelayout.txt" );
		
		GenomeUtil::schema->printIndexes( f, GenomeUtil::layout );
		
		fclose( f );

		SYSTEM( "cat run/genome/meta/genelayout.txt | sort -n > run/genome/meta/genelayout-sorted.txt" );
	}
	{
		FILE* f = createFile( "run/genome/meta/genetitle.txt" );
		
		GenomeUtil::schema->printTitles( f );
		
		fclose( f );
	}
	{
		FILE* f = createFile( "run/genome/meta/generange.txt" );
		
		GenomeUtil::schema->printRanges( f );
		
		fclose( f );		
	}
}


//===========================================================================
// GenomeSubsetLog
//===========================================================================

//---------------------------------------------------------------------------
// Logs::GenomeSubsetLog::init
//---------------------------------------------------------------------------
void Logs::GenomeSubsetLog::init( TSimulation *sim, Document *doc )
{
	if( doc->get("GenomeSubsetLog").get("Record") )
	{
		initRecording( sim, SimulationStateScope, sim::Event_Birth );

		Property &propGenomeSubsetLog = doc->get( "GenomeSubsetLog" );

		Property &propGeneNames = propGenomeSubsetLog.get( "GeneNames" );
		vector<string> geneNames;
		for( int i = 0; i < (int)propGeneNames.size(); i++ )
		{
			geneNames.push_back( propGeneNames.get(i) );
		}

		GenomeUtil::schema->getIndexes( geneNames, _geneIndexes );
		for( int i = 0; i < (int)geneNames.size(); i++ )
		{
			if( _geneIndexes[i] < 0 )
			{
				cerr << "Invalid gene name for GenomeSubsetLog: " << geneNames[i] << endl;
				exit( 1 );
			}
		}

		vector<string> colnames;
		vector<datalib::Type> coltypes;

		colnames.push_back( "Agent" );
		coltypes.push_back( datalib::INT );

		itfor( vector<string>, geneNames, it )
		{
			colnames.push_back( *it );
			coltypes.push_back( datalib::INT );
		}

		DataLibWriter *writer = createWriter( "run/genome/subset.log" );
		writer->beginTable( "GenomeSubset",
							colnames,
							coltypes );
	}
}

//---------------------------------------------------------------------------
// Logs::GenomeSubsetLog::processEvent
//---------------------------------------------------------------------------
void Logs::GenomeSubsetLog::processEvent( const sim::AgentBirthEvent &birth )
{
	if( birth.reason != LifeSpan::BR_VIRTUAL )
	{
		agent *a = birth.a;

		Variant values[ 1 + _geneIndexes.size() ];
		values[0] = a->Number();

		for( int i = 0; i < (int)_geneIndexes.size(); i++ )
		{
			values[ i + 1 ] = (int)a->Genes()->get_raw_uint( _geneIndexes[i] );
		}

		getWriter()->addRow( values );
	}
}


//===========================================================================
// LifeSpanLog
//===========================================================================

//---------------------------------------------------------------------------
// Logs::LifeSpanLog::init
//---------------------------------------------------------------------------
void Logs::LifeSpanLog::init( TSimulation *sim, Document *doc )
{
	initRecording( sim, SimulationStateScope, sim::Event_Death );

	createWriter( "run/lifespans.txt" );

	const char *colnames[] =
		{
			"Agent",
			"BirthStep",
			"BirthReason",
			"DeathStep",
			"DeathReason",
			NULL
		};
	const datalib::Type coltypes[] =
		{
			datalib::INT,
			datalib::INT,
			datalib::STRING,
			datalib::INT,
			datalib::STRING
		};

	getWriter()->beginTable( "LifeSpans",
							  colnames,
							  coltypes );
}

//---------------------------------------------------------------------------
// Logs::LifeSpanLog::processEvent
//---------------------------------------------------------------------------
void Logs::LifeSpanLog::processEvent( const sim::AgentDeathEvent &death )
{
	agent *a = death.a;
	LifeSpan *ls = a->GetLifeSpan();

	getWriter()->addRow( a->Number(),
						 ls->birth.step,
						 LifeSpan::BR_NAMES[ls->birth.reason],
						 ls->death.step,
						 LifeSpan::DR_NAMES[ls->death.reason] );
}


//===========================================================================
// SeparationLog
//===========================================================================

//---------------------------------------------------------------------------
// Logs::SeparationLog::init
//---------------------------------------------------------------------------
void Logs::SeparationLog::init( TSimulation *sim, Document *doc )
{
	if( (string)doc->get("RecordSeparations") != "False" )
	{
		string mode = doc->get( "RecordSeparations" );
		if( mode == "Contact" )
		{
			_mode = Contact;
			initRecording( sim,
						   SimulationStateScope,
						   sim::Event_Death | sim::Event_ContactBegin );
		}
		else if( mode == "All" )
		{
			_mode = All;
			initRecording( sim,
						   SimulationStateScope,
						   sim::Event_Death | sim::Event_Birth | sim::Event_StepEnd );
		}
		else
			assert(false);


		createWriter( "run/genome/separations.txt" );
	}
}

//---------------------------------------------------------------------------
// Logs::SeparationLog::processEvent
//---------------------------------------------------------------------------
void Logs::SeparationLog::processEvent( const sim::AgentBirthEvent &birth )
{
	assert( _mode == All );

	_births.push_back( birth.a );
}

//---------------------------------------------------------------------------
// Logs::SeparationLog::processEvent
//---------------------------------------------------------------------------
void Logs::SeparationLog::processEvent( const sim::AgentContactBeginEvent &e )
{
	assert( _mode == Contact );

	SeparationCache::createEntry( e.c.a, e.d.a );
}

//---------------------------------------------------------------------------
// Logs::SeparationLog::processEvent
//---------------------------------------------------------------------------
void Logs::SeparationLog::processEvent( const sim::AgentDeathEvent &death )
{
	SeparationCache::AgentEntries &entries = SeparationCache::getEntries( death.a );
	
	if( entries.size() > 0 )
	{
		char buf[16];
		sprintf( buf, "%ld", death.a->Number() );

		static const char *colnames[] =
			{
				"Agent",
				"Separation",
				NULL
			};

		static const datalib::Type coltypes[] =
			{
				datalib::INT,
				datalib::FLOAT
			};

		DataLibWriter *writer = getWriter();

		writer->beginTable( buf,
							colnames,
							coltypes );


		itfor( SeparationCache::AgentEntries, entries, it )
		{
			writer->addRow( it->first, it->second );
		}

		writer->endTable();
	}
}

//---------------------------------------------------------------------------
// Logs::SeparationLog::processEvent
//---------------------------------------------------------------------------
void Logs::SeparationLog::processEvent( const sim::StepEndEvent &e )
{
	assert( _mode == All );

	if( !_births.empty() )
	{
		itfor( list<agent *>, _births, it )
		{
			agent *a_born = *it;

			xfor( AGENTTYPE, agent, a_other )
			{
				if( a_born != a_other )
					SeparationCache::createEntry( a_born, a_other );
			}
		}

		_births.clear();
	}
}
