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
Logs::Logs( TSimulation *sim, Document *doc )
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

//---------------------------------------------------------------------------
// Logs::getMaxOpenFiles
//---------------------------------------------------------------------------
int Logs::getMaxOpenFiles()
{
	int maxOpenFiles = 0;

	itfor( LoggerList, _installedLoggers, it )
	{
		maxOpenFiles += (*it)->getMaxOpenFiles();
	}

	return maxOpenFiles;
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

		initRecording( sim,
					   NullStateScope,
					   sim::Event_StepEnd );
	}
}

//---------------------------------------------------------------------------
// Logs::AdamiComplexityLog::getMaxOpenFiles
//---------------------------------------------------------------------------
int Logs::AdamiComplexityLog::getMaxOpenFiles()
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
	string mode = doc->get("RecordPosition");
	if( mode != "False" )
	{
		if( mode == "Precise" )
			_mode = Precise;
		else if( mode == "Approximate" )
			_mode = Approximate;
		else
			assert( false );

		initRecording( sim,
					   AgentStateScope,
					   sim::Event_AgentBirth
					   | sim::Event_BodyUpdated
					   | sim::Event_AgentDeath );
	}
}

//---------------------------------------------------------------------------
// Logs::AgentPositionLog::processEvent
//---------------------------------------------------------------------------
void Logs::AgentPositionLog::processEvent( const sim::AgentBirthEvent &e )
{
	if( e.reason == LifeSpan::BR_VIRTUAL )
		return;

	char path[512];
	sprintf( path,
			 "run/motion/position/agents/position_%ld.txt",
			 e.a->getTypeNumber() );

	switch( _mode )
	{
	case Precise:
		{
			DataLibWriter *writer = createWriter( e.a, path, true, false );

			static const char *colnames[] = {"Timestep", "x", "y", "z", NULL};
			static const datalib::Type coltypes[] = {datalib::INT, datalib::FLOAT, datalib::FLOAT, datalib::FLOAT};

			writer->beginTable( "Positions",
								colnames,
								coltypes );
		}
		break;
	case Approximate:
		{
			DataLibWriter *writer = createWriter( e.a, path );

			static const char *colnames[] = {"Timestep", "x", "z", NULL};
			static const datalib::Type coltypes[] = {datalib::INT, datalib::FLOAT, datalib::FLOAT};
			static const char *colformats[] = {"%d", "%.2f", "%.2f"};

			writer->beginTable( "Positions",
								colnames,
								coltypes,
								colformats );
		}
		break;
	default:
		assert( false );
	}
}

//---------------------------------------------------------------------------
// Logs::AgentPositionLog::processEvent
//---------------------------------------------------------------------------
void Logs::AgentPositionLog::processEvent( const AgentBodyUpdatedEvent &e )
{
	switch( _mode )
	{
	case Precise:
		getWriter( e.a )->addRow( getStep(),
								  e.a->x(),
								  e.a->y(),
								  e.a->z() );
		break;
	case Approximate:
		getWriter( e.a )->addRow( getStep(),
								  e.a->x(),
								  e.a->z() );
		break;
	default:
		assert( false );
	}
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
		initRecording( sim,
					   SimulationStateScope,
					   sim::Event_AgentBirth
					   | sim::Event_AgentDeath );

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
// BrainAnatomyLog
//===========================================================================

//---------------------------------------------------------------------------
// Logs::BrainAnatomyLog::init
//---------------------------------------------------------------------------
void Logs::BrainAnatomyLog::init( TSimulation *sim, Document *doc )
{
	if( doc->get("RecordBrain") )
	{
		_recordRecent = doc->get( "RecordBrainRecent" );
		_recordBestRecent = doc->get( "RecordBrainBestRecent" );
		_recordBestSoFar = doc->get( "RecordBrainBestSoFar" );

		if( _recordBestRecent || _recordBestSoFar )
		{
			initRecording( sim,
						   NullStateScope,
						   sim::Event_BrainGrown
						   | sim::Event_AgentGrown
						   | sim::Event_BrainAnalysisBegin
						   | sim::Event_EpochEnd );
		}
		else
		{
			initRecording( sim,
						   NullStateScope,
						   sim::Event_BrainGrown
						   | sim::Event_AgentGrown
						   | sim::Event_BrainAnalysisBegin );
		}
	}
}

//---------------------------------------------------------------------------
// Logs::BrainAnatomyLog::processEvent
//---------------------------------------------------------------------------
void Logs::BrainAnatomyLog::processEvent( const BrainGrownEvent &e )
{
	createAnatomyFile( e.a, "incept", 0.0 );
}

//---------------------------------------------------------------------------
// Logs::BrainAnatomyLog::processEvent
//---------------------------------------------------------------------------
void Logs::BrainAnatomyLog::processEvent( const AgentGrownEvent &e )
{
	createAnatomyFile( e.a, "birth", 0.0 );
}

//---------------------------------------------------------------------------
// Logs::BrainAnatomyLog::processEvent
//---------------------------------------------------------------------------
void Logs::BrainAnatomyLog::processEvent( const BrainAnalysisBeginEvent &e )
{
	createAnatomyFile( e.a, "death", e.a->CurrentHeuristicFitness() );
}

//---------------------------------------------------------------------------
// Logs::BrainAnatomyLog::processEvent
//---------------------------------------------------------------------------
void Logs::BrainAnatomyLog::processEvent( const EpochEndEvent &e )
{
	if( _recordBestRecent )
		recordEpochFittest( e.epoch, FS_RECENT, "bestRecent" );

	if( _recordBestSoFar )
		recordEpochFittest( e.epoch, FS_OVERALL, "bestSoFar" );
}

//---------------------------------------------------------------------------
// Logs::BrainAnatomyLog::createAnatomyFile
//---------------------------------------------------------------------------
void Logs::BrainAnatomyLog::createAnatomyFile( agent *a, const char *suffix, float fitness )
{
	char path[256];
	sprintf( path, "run/brain/anatomy/brainAnatomy_%ld_%s.txt", a->Number(), suffix );

	AbstractFile *file = createFile( path );
	a->GetBrain()->dumpAnatomical( file, a->Number(), fitness );
	delete file;
}

//---------------------------------------------------------------------------
// Logs::BrainAnatomyLog::recordEpochFittest
//---------------------------------------------------------------------------
void Logs::BrainAnatomyLog::recordEpochFittest( long step, sim::FitnessScope scope, const char *scopeName )
{
	char s[256];
	FittestList *fittest = _simulation->getFittest( scope );

	sprintf( s, "run/brain/%s/%ld", scopeName, step );
	makeDirs( s );
	for( int i = 0; i < fittest->size(); i++ )
	{
		static const char *prefixes[] = { "incept", "birth", "death", NULL };

		for( const char **prefix = prefixes; *prefix; prefix++ )
		{
			char t[256];	// target (use s for source)
			sprintf( s, "run/brain/anatomy/brainAnatomy_%ld_%s.txt", fittest->get(i)->agentID, *prefix );
			sprintf( t, "run/brain/%s/%ld/%d_brainAnatomy_%ld_%s.txt", scopeName, step, i, fittest->get(i)->agentID, *prefix );
			AbstractFile::link( s, t );
		}
	}
}


//===========================================================================
// BrainComplexityLog
//===========================================================================

//---------------------------------------------------------------------------
// Logs::BrainComplexityLog::~BrainComplexityLog
//---------------------------------------------------------------------------
Logs::BrainComplexityLog::~BrainComplexityLog()
{
	if( _record )
	{
		// These won't take any action if the complexity maps are empty.
		// Chances are we have already written our data and the maps are empty.
		writeComplexityFile( 0, _seedComplexity );
		writeComplexityFile( _simulation->getEpoch(), _recentComplexity );
	}
}

//---------------------------------------------------------------------------
// Logs::BrainComplexityLog::init
//---------------------------------------------------------------------------
void Logs::BrainComplexityLog::init( TSimulation *sim, Document *doc )
{
	if( doc->get("RecordComplexity") )
	{
		initRecording( sim,
					   NullStateScope,
					   sim::Event_BrainAnalysisEnd
					   | sim::Event_EpochEnd );

		_nseeds = doc->get( "InitAgents" );
		_seedsRemaining = _nseeds;
		_complexityType = (string)doc->get( "ComplexityType" );
		_recordBestRecent = doc->get( "RecordBrainBestRecent" );

		sim->enableComplexityCalculations();
	}
}

//---------------------------------------------------------------------------
// Logs::BrainComplexityLog::processEvent
//---------------------------------------------------------------------------
void Logs::BrainComplexityLog::processEvent( const sim::BrainAnalysisEndEvent &e )
{
	// Analysis is performed on different threads, so we must do the following in a mutex.
#pragma omp critical(BrainComplexityLog)
	{
		if( _seedsRemaining && (e.a->Number() < _nseeds) )
		{
			if( e.a->Complexity() > 0.0 )
				_seedComplexity[ e.a->Number() ] = e.a->Complexity();

			if( --_seedsRemaining == 0 )
				writeComplexityFile( 0, _seedComplexity );
		}

		if( e.a->Complexity() > 0.0 )
			_recentComplexity[ e.a->Number() ] = e.a->Complexity();
	}
}

//---------------------------------------------------------------------------
// Logs::BrainComplexityLog::processEvent
//---------------------------------------------------------------------------
void Logs::BrainComplexityLog::processEvent( const sim::EpochEndEvent &e )
{
	writeComplexityFile( e.epoch, _recentComplexity );

	if( _recordBestRecent )
		writeBestRecent( e.epoch );
}

//---------------------------------------------------------------------------
// Logs::BrainComplexityLog::writeComplexityFile
//
// Writes complexities to file if non-empty. Clears complexities at end.
//---------------------------------------------------------------------------
void Logs::BrainComplexityLog::writeComplexityFile( long epoch, ComplexityMap &complexities )
{
	if( complexities.empty() )
		return;

	const char *colnames[] =
		{
			"AgentNumber",
			"Complexity",
			NULL
		};
	const datalib::Type coltypes[] =
		{
			datalib::INT,
			datalib::FLOAT
		};

	char path[256];
	sprintf( path, "run/brain/Recent/%ld/complexity_%s.plt", epoch, _complexityType.c_str() );

	DataLibWriter *writer = createWriter( path );

	writer->beginTable( _complexityType.c_str(),
						colnames,
						coltypes );

	itfor( ComplexityMap, complexities, it )
	{
		writer->addRow( it->first, it->second );
	}

	delete writer;

	complexities.clear();
}

//---------------------------------------------------------------------------
// Logs::BrainComplexityLog::writeBestRecent
//---------------------------------------------------------------------------
void Logs::BrainComplexityLog::writeBestRecent( long epoch )
{
	FittestList *fittest = _simulation->getFittest( FS_RECENT );

	double mean=0;
	double stddev=0;	//Complexity: Time to Average and StdDev the BestRecent List
	int count=0;		//Keeps a count of the number of entries in fittest.
			
	for( int i = 0; i < fittest->size(); i++ )
	{
		mean += fittest->get(i)->complexity;		// Get Sum of all Complexities
		count++;
	}
		
	mean = mean / count;			// Divide by count to get the average
		

	if( ! (mean >= 0) )			// If mean is 'nan', make it zero instead of 'nan'  -- Only true before any agents have died.
	{
		mean = 0;
		stddev = 0;
	}
	else						// Calculate the stddev (You'll do this except when before an agent has died)
	{
		for( int i = 0; i < fittest->size(); i++ )
		{
			stddev += pow(fittest->get(i)->complexity - mean, 2);		// Get Sum of all Complexities
		}
	}
		
	stddev = sqrt(stddev / (count-1) );		// note that this stddev is divided by N-1 (MATLAB default)
	double StandardError = stddev / sqrt(count);
			
	const char *path = "run/brain/bestRecent/complexity.txt";
	makeParentDir( path );
	FILE *cFile = fopen( path, "a" );
	if( !cFile ) { perror( path ); exit( 1 ); }

	// print to complexity.txt
	fprintf( cFile, "%ld %f %f %f %d\n", epoch, mean, stddev, StandardError, count );
	fclose( cFile );		
}


//===========================================================================
// BrainFunctionLog
//===========================================================================

//---------------------------------------------------------------------------
// Logs::BrainFunctionLog::init
//---------------------------------------------------------------------------
void Logs::BrainFunctionLog::init( TSimulation *sim, Document *doc )
{
	if( doc->get("RecordBrain") )
	{
		_recordRecent = doc->get( "RecordBrainRecent" );
		_recordBestRecent = doc->get( "RecordBrainBestRecent" );
		_recordBestSoFar = doc->get( "RecordBrainBestSoFar" );
		_nseeds = doc->get( "InitAgents" );

		if( _recordBestRecent || _recordBestSoFar )
		{
			initRecording( sim,
						   AgentStateScope,
						   sim::Event_AgentGrown
						   | sim::Event_BrainUpdated
						   | sim::Event_BrainAnalysisBegin
						   | sim::Event_EpochEnd );
		}
		else
		{
			initRecording( sim,
						   AgentStateScope,
						   sim::Event_AgentGrown
						   | sim::Event_BrainUpdated
						   | sim::Event_BrainAnalysisBegin );
		}
	}
}

//---------------------------------------------------------------------------
// Logs::BrainFunctionLog::processEvent
//
// Once agent is grown, begin recording brain function.
//---------------------------------------------------------------------------
void Logs::BrainFunctionLog::processEvent( const AgentGrownEvent &e )
{
	char path[256];
	sprintf( path, "run/brain/function/incomplete_brainFunction_%ld.txt", e.a->Number() );	

	AbstractFile *file = createFile( e.a, path );
	e.a->GetBrain()->startFunctional( file, e.a->Number() );
}

//---------------------------------------------------------------------------
// Logs::BrainFunctionLog::processEvent
//
// When brain has executed a step, record its function state.
//---------------------------------------------------------------------------
void Logs::BrainFunctionLog::processEvent( const BrainUpdatedEvent &e )
{
	AbstractFile *file = getFile( e.a );
	e.a->GetBrain()->writeFunctional( file );
}

//---------------------------------------------------------------------------
// Logs::BrainFunctionLog::processEvent
//
// Agent is dead, and we must finalize our log for analysis.
//---------------------------------------------------------------------------
void Logs::BrainFunctionLog::processEvent( const BrainAnalysisBeginEvent &e )
{
	AbstractFile *file = getFile( e.a );

	e.a->GetBrain()->endFunctional( file, e.a->CurrentHeuristicFitness() );
	delete file;

	char s[256];
	char t[256];
	sprintf( s, "run/brain/function/incomplete_brainFunction_%ld.txt", e.a->Number() );
	sprintf( t, "run/brain/function/brainFunction_%ld.txt", e.a->Number() );
	AbstractFile::rename( s, t );

	// Simulation needs this path for calculating complexity.
	e.a->brainAnalysisParms.functionPath = t;

	if( _recordRecent )
	{
		sprintf( s, "run/brain/Recent/%ld/brainFunction_%ld.txt", _simulation->getEpoch(), e.a->Number() );
		makeParentDir( s );
		AbstractFile::link( t, s );
			
		if( e.a->Number() <= _nseeds )
		{
			sprintf( s, "run/brain/Recent/0/brainFunction_%ld.txt", e.a->Number() );
			makeParentDir( s );
			AbstractFile::link( t, s );
		}
	}
}

//---------------------------------------------------------------------------
// Logs::BrainFunctionLog::processEvent
//---------------------------------------------------------------------------
void Logs::BrainFunctionLog::processEvent( const EpochEndEvent &e )
{
	if( _recordBestRecent )
		recordEpochFittest( e.epoch, FS_RECENT, "bestRecent" );

	if( _recordBestSoFar )
		recordEpochFittest( e.epoch, FS_OVERALL, "bestSoFar" );
}

//---------------------------------------------------------------------------
// Logs::BrainFunctionLog::recordEpochFittest
//---------------------------------------------------------------------------
void Logs::BrainFunctionLog::recordEpochFittest( long step, sim::FitnessScope scope, const char *scopeName )
{
	char s[256];
	FittestList *fittest = _simulation->getFittest( scope );

	sprintf( s, "run/brain/%s/%ld", scopeName, step );
	makeDirs( s );
	for( int i = 0; i < fittest->size(); i++ )
	{
		char t[256];	// target (use s for source)
		sprintf( s, "run/brain/function/brainFunction_%ld.txt", fittest->get(i)->agentID );
		sprintf( t, "run/brain/%s/%ld/%d_brainFunction_%ld.txt", scopeName, step, i, fittest->get(i)->agentID );
		AbstractFile::link( s, t );
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
		initRecording( sim,
					   SimulationStateScope,
					   sim::Event_Carry );

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
		initRecording( sim,
					   SimulationStateScope,
					   sim::Event_ContactEnd );

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
		initRecording( sim,
					   SimulationStateScope,
					   sim::Event_Energy );

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
			sprintf( buf, "Energy%d", i );
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
		initRecording( sim,
					   SimulationStateScope,
					   sim::Event_StepEnd );

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
		initRecording( sim,
					   NullStateScope,
					   sim::Event_AgentBirth );
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
	initRecording( sim,
				   NullStateScope,
				   sim::Event_SimInited );
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
		initRecording( sim,
					   SimulationStateScope,
					   sim::Event_AgentBirth );

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
	initRecording( sim,
				   SimulationStateScope,
				   sim::Event_AgentDeath );

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
						   sim::Event_AgentDeath
						   | sim::Event_ContactBegin );
		}
		else if( mode == "All" )
		{
			_mode = All;
			initRecording( sim,
						   SimulationStateScope,
						   sim::Event_AgentDeath
						   | sim::Event_AgentBirth
						   | sim::Event_StepEnd );
		}
		else
			assert(false);


		createWriter( "run/genome/separations.txt" );
	}
}

//---------------------------------------------------------------------------
// Logs::SeparationLog::processEvent
//---------------------------------------------------------------------------
void Logs::SeparationLog::processEvent( const sim::AgentBirthEvent &e )
{
	if( e.reason == LifeSpan::BR_VIRTUAL )
		return;

	assert( _mode == All );

	_births.push_back( e.a );
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
