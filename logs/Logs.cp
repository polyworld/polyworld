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

Logs::LoggerList Logs::_loggers;

//---------------------------------------------------------------------------
// Logs::Logs
//---------------------------------------------------------------------------
Logs::Logs( TSimulation *sim, proplib::Document *doc )
{
	assert( logs == NULL );

	itfor( LoggerList, _loggers, it )
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
	_loggers.clear();
	logs = NULL;
}

//---------------------------------------------------------------------------
// Logs::birth
//---------------------------------------------------------------------------
void Logs::birth( const sim::AgentBirthEvent &birth )
{
	itfor( LoggerList, _loggers, it )
	{
		(*it)->birth( birth );
	}
}

//---------------------------------------------------------------------------
// Logs::death
//---------------------------------------------------------------------------
void Logs::death( const sim::AgentDeathEvent &death )
{
	itfor( LoggerList, _loggers, it )
	{
		(*it)->death( death );
	}
}


//===========================================================================
// AdamiComplexityLog
//===========================================================================

//---------------------------------------------------------------------------
// Logs::AdamiComplexityLog::update
//---------------------------------------------------------------------------
void Logs::AdamiComplexityLog::update()
{
	if( !_record ) return;

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

//---------------------------------------------------------------------------
// Logs::AdamiComplexityLog::init
//---------------------------------------------------------------------------
void Logs::AdamiComplexityLog::init( TSimulation *sim, Document *doc )
{
	if( doc->get("RecordAdamiComplexity") )
	{
		_frequency = doc->get( "AdamiComplexityRecordFrequency" );

		initRecording( NullStateScope, sim );
	}
}

//---------------------------------------------------------------------------
// Logs::AdamiComplexityLog::getMaxFileCount
//---------------------------------------------------------------------------
int Logs::AdamiComplexityLog::getMaxFileCount()
{
	return 4;
}


//===========================================================================
// AgentPositionLog
//===========================================================================

//---------------------------------------------------------------------------
// Logs::AgentPositionLog::update
//---------------------------------------------------------------------------
void Logs::AgentPositionLog::update( agent *a )
{
	if( !_record ) return;

	getWriter( a )->addRow( getStep(),
							a->x(),
							a->y(),
							a->z() );
}

//---------------------------------------------------------------------------
// Logs::AgentPositionLog::init
//---------------------------------------------------------------------------
void Logs::AgentPositionLog::init( TSimulation *sim, Document *doc )
{
	if( doc->get("RecordPosition") )
	{
		initRecording( AgentStateScope, sim );
	}
}

//---------------------------------------------------------------------------
// Logs::AgentPositionLog::birth
//---------------------------------------------------------------------------
void Logs::AgentPositionLog::birth( const sim::AgentBirthEvent &birth )
{
	if( !_record ) return;

	char path[512];
	sprintf( path,
			 "run/motion/position/agents/position_%ld.txt",
			 birth.a->getTypeNumber() );

	DataLibWriter *writer = createWriter( birth.a, path, true, false );

	static const char *colnames[] = {"Timestep", "x", "y", "z", NULL};
	static const datalib::Type coltypes[] = {datalib::INT, datalib::FLOAT, datalib::FLOAT, datalib::FLOAT};

	writer->beginTable( "Positions",
						colnames,
						coltypes );
}

//---------------------------------------------------------------------------
// Logs::AgentPositionLog::death
//---------------------------------------------------------------------------
void Logs::AgentPositionLog::death( const sim::AgentDeathEvent &death )
{
	if( !_record ) return;

	delete getWriter( death.a );
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
		initRecording( SimulationStateScope, sim );

		createFile( "run/BirthsDeaths.log" );
		fprintf( getFile(), "%% Timestep Event Agent# Parent1 Parent2\n" );
	}
}

//---------------------------------------------------------------------------
// Logs::BirthsDeathsLog::birth
//---------------------------------------------------------------------------
void Logs::BirthsDeathsLog::birth( const sim::AgentBirthEvent &birth )
{
	if( !_record ) return;

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
// Logs::BirthsDeathsLog::death
//---------------------------------------------------------------------------
void Logs::BirthsDeathsLog::death( const sim::AgentDeathEvent &death )
{
	if( !_record ) return;

	if( death.reason != LifeSpan::DR_SIMEND )
	{
		fprintf( getFile(), "%ld DEATH %ld\n", getStep(), death.a->Number() );
	}
}


//===========================================================================
// CarryLog
//===========================================================================

//---------------------------------------------------------------------------
// Logs::CarryLog::update
//---------------------------------------------------------------------------
void Logs::CarryLog::update( agent *a,
						  gobject *obj,
						  CarryAction action )
{
	if( !_record ) return;

	static const char *actions[] = {"P",
									"D",
									"Do"};

	const char *objectType;
	switch(obj->getType())
	{
	case AGENTTYPE: objectType = "A"; break;
	case FOODTYPE: objectType = "F"; break;
	case BRICKTYPE: objectType = "B"; break;
	default: assert(false);
	}

	getWriter()->addRow( getStep(),
						 a->Number(),
						 actions[action],
						 objectType,
						 obj->getTypeNumber() );
}

//---------------------------------------------------------------------------
// Logs::CarryLog::init
//---------------------------------------------------------------------------
void Logs::CarryLog::init( TSimulation *sim, Document *doc )
{
	if( doc->get("RecordCarry") )
	{
		initRecording( SimulationStateScope, sim );

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


//===========================================================================
// CollisionLog
//===========================================================================

//---------------------------------------------------------------------------
// Logs::CollisionLog::update
//---------------------------------------------------------------------------
void Logs::CollisionLog::update( agent *a,
								 ObjectType ot )
{
	if( !_record ) return;

	// We store type as a string since this data will most likely
	// be processed by a script.
	static const char *names[] = {"agent",
								  "food",
								  "brick",
								  "barrier",
								  "edge"};

	getWriter()->addRow( getStep(),
						 a->Number(),
						 names[ot] );
}

//---------------------------------------------------------------------------
// Logs::CollisionLog::init
//---------------------------------------------------------------------------
void Logs::CollisionLog::init( TSimulation *sim, Document *doc )
{
	if( doc->get("RecordCollisions") )
	{
		initRecording( SimulationStateScope, sim );

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


//===========================================================================
// ContactLog
//===========================================================================

//---------------------------------------------------------------------------
// Logs::ContactLog::update
//---------------------------------------------------------------------------
void Logs::ContactLog::update( ContactEntry &entry )
{
	if( !_record ) return;

	char buf[32];
	char *b = buf;

	entry.c.encode( &b );
	*(b++) = 'C';
	entry.d.encode( &b );
	*(b++) = 0;

	getWriter()->addRow( entry.step,
						 entry.c.number,
						 entry.d.number,
						 buf );
}

//---------------------------------------------------------------------------
// Logs::ContactLog::init
//---------------------------------------------------------------------------
void Logs::ContactLog::init( TSimulation *sim, Document *doc )
{
	if( doc->get("RecordContacts") )
	{
		initRecording( SimulationStateScope, sim );

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


//===========================================================================
// EnergyLog
//===========================================================================

//---------------------------------------------------------------------------
// Logs::EnergyLog::update
//---------------------------------------------------------------------------
void Logs::EnergyLog::update( agent *c,
							  gobject *obj,
							  float neuralActivation,
							  const Energy &energy,
							  EventType elet )
{
	if( !_record ) return;

	static const char *eventTypes[] = {"G",
									   "F",
									   "E"};
	const char *eventType = eventTypes[elet];

	Variant coldata[ 5 + globals::numEnergyTypes ];
	coldata[0] = getStep();
	coldata[1] = c->Number();
	coldata[2] = eventType;
	coldata[3] = (long)obj->getTypeNumber();
	coldata[4] = neuralActivation;
	for( int i = 0; i < globals::numEnergyTypes; i++ )
	{
		coldata[5 + i] = energy[i];
	}

	getWriter()->addRow( coldata );
}

//---------------------------------------------------------------------------
// Logs::EnergyLog::init
//---------------------------------------------------------------------------
void Logs::EnergyLog::init( TSimulation *sim, Document *doc )
{
	if( doc->get("RecordEnergy") )
	{
		initRecording( SimulationStateScope, sim );

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
		initRecording( NullStateScope, sim );
	}
}

//---------------------------------------------------------------------------
// Logs::GenomeLog::birth
//---------------------------------------------------------------------------
void Logs::GenomeLog::birth( const sim::AgentBirthEvent &birth )
{
	if( !_record ) return;

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
// GenomeSubsetLog
//===========================================================================

//---------------------------------------------------------------------------
// Logs::GenomeSubsetLog::init
//---------------------------------------------------------------------------
void Logs::GenomeSubsetLog::init( TSimulation *sim, Document *doc )
{
	if( doc->get("GenomeSubsetLog").get("Record") )
	{
		initRecording( SimulationStateScope, sim );

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
// Logs::GenomeSubsetLog::birth
//---------------------------------------------------------------------------
void Logs::GenomeSubsetLog::birth( const sim::AgentBirthEvent &birth )
{
	if( !_record ) return;

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
	initRecording( SimulationStateScope, sim );

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
// Logs::LifeSpanLog::death
//---------------------------------------------------------------------------
void Logs::LifeSpanLog::death( const sim::AgentDeathEvent &death )
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
// Logs::SeparationLog::update
//---------------------------------------------------------------------------
void Logs::SeparationLog::update( agent *a, agent *b )
{
	if( !_record ) return;

	// Force a separation calculation so it gets logged.
	SeparationCache::separation( a, b );
}

//---------------------------------------------------------------------------
// Logs::SeparationLog::init
//---------------------------------------------------------------------------
void Logs::SeparationLog::init( TSimulation *sim, Document *doc )
{
	if( doc->get("RecordSeparations") )
	{
		initRecording( SimulationStateScope, sim );

		createWriter( "run/genome/separations.txt" );
	}
}

//---------------------------------------------------------------------------
// Logs::SeparationLog::death
//---------------------------------------------------------------------------
void Logs::SeparationLog::death( const sim::AgentDeathEvent &death )
{
	if( !_record ) return;

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


