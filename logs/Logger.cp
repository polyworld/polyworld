// todo prune includes

#include "Logger.h"
#include "Logs.h"

#include <stdio.h>
#include <stdlib.h>
#include <iostream>

#include "AbstractFile.h"
#include "agent.h"
#include "datalib.h"
#include "globals.h"
#include "misc.h"
#include "proplib.h"
#include "Simulation.h"


using namespace datalib;
using namespace proplib;
using namespace std;


//===========================================================================
// Logger
//===========================================================================

//---------------------------------------------------------------------------
// Logger::Logger
//---------------------------------------------------------------------------
Logger::Logger()
	: _simulation( NULL )
	, _record( false )
{
	Logs::installLogger( const_cast<Logger *>(this) );
}

//---------------------------------------------------------------------------
// Logger::~Logger
//---------------------------------------------------------------------------
Logger::~Logger()
{
}

//---------------------------------------------------------------------------
// Logger::getMaxOpenFiles
//---------------------------------------------------------------------------
int Logger::getMaxOpenFiles()
{
	if( !_record ) return 0;

	assert( _simulation );

	switch( _scope )
	{
	case AgentStateScope:
		return _simulation->GetMaxAgents();
	default:
		return 1;
	}
}

//---------------------------------------------------------------------------
// Logger::initRecording
//---------------------------------------------------------------------------
void Logger::initRecording( TSimulation *sim, StateScope scope, sim::EventType events )
{
	_scope = scope;
	_record = true;
	_simulation = sim;

	switch( scope )
	{
	case NullStateScope:
		// no-op
		break;
	case SimulationStateScope:
		_simulationScope.data = NULL;
		break;
	case AgentStateScope:
		_agentScope.slotHandle = AgentAttachedData::createSlot();
		break;
	default:
		assert( false );
	}

	Logs::registerEvents( this, events );
}

//---------------------------------------------------------------------------
// Logger::getStep
//---------------------------------------------------------------------------
long Logger::getStep()
{
	return _simulation->getStep();
}

//---------------------------------------------------------------------------
// Logger::getSimulationState
//---------------------------------------------------------------------------
void *Logger::getSimulationState()
{
	assert( _scope == SimulationStateScope );

	return _simulationScope.data;
}

//---------------------------------------------------------------------------
// Logger::getAgentState
//---------------------------------------------------------------------------
void *Logger::getAgentState( agent *a )
{
	assert( _scope == AgentStateScope );

	return AgentAttachedData::get( a, _agentScope.slotHandle );
}

//---------------------------------------------------------------------------
// Logger::setSimulationState
//---------------------------------------------------------------------------
void Logger::setSimulationState( void *state )
{
	assert( _scope == SimulationStateScope );

	_simulationScope.data = state;
}

//---------------------------------------------------------------------------
// Logger::setAgentState
//---------------------------------------------------------------------------
void Logger::setAgentState( agent *a, void *state )
{
	AgentAttachedData::set( a, _agentScope.slotHandle, state );
}


//===========================================================================
// FileLogger
//===========================================================================

//---------------------------------------------------------------------------
// FileLogger::FileLogger
//---------------------------------------------------------------------------
FileLogger::FileLogger()
	: Logger()
{
}

//---------------------------------------------------------------------------
// FileLogger::~FileLogger
//---------------------------------------------------------------------------
FileLogger::~FileLogger()
{
	if( (_scope == SimulationStateScope) && _record )
	{
		fclose( getFile() );
	}
}

//---------------------------------------------------------------------------
// FileLogger::createFile
//---------------------------------------------------------------------------
FILE *FileLogger::createFile( const string &path, const char *mode )
{
	makeParentDir( path );

	FILE *file = fopen( path.c_str(), mode );

	if( _scope == SimulationStateScope )
		setSimulationState( file );

	return file;
}

//---------------------------------------------------------------------------
// FileLogger::getFile
//---------------------------------------------------------------------------
FILE *FileLogger::getFile()
{
	return (FILE *)getSimulationState();
}

//---------------------------------------------------------------------------
// FileLogger::createFile
//---------------------------------------------------------------------------
FILE *FileLogger::createFile( agent *a, const string &path, const char *mode )
{
	makeParentDir( path );

	FILE *file = fopen( path.c_str(), mode );
	setAgentState( a, file );

	return file;
}

//---------------------------------------------------------------------------
// FileLogger::getFile
//---------------------------------------------------------------------------
FILE *FileLogger::getFile( agent *a )
{
	return (FILE *)getAgentState( a );
}


//===========================================================================
// AbstractFileLogger
//===========================================================================

//---------------------------------------------------------------------------
// AbstractFileLogger::AbstractFileLogger
//---------------------------------------------------------------------------
AbstractFileLogger::AbstractFileLogger()
	: Logger()
{
}

//---------------------------------------------------------------------------
// AbstractFileLogger::~AbstractFileLogger
//---------------------------------------------------------------------------
AbstractFileLogger::~AbstractFileLogger()
{
	if( (_scope == SimulationStateScope) && _record )
	{
		delete( getFile() );
	}
}

//---------------------------------------------------------------------------
// AbstractFileLogger::createFile
//---------------------------------------------------------------------------
AbstractFile *AbstractFileLogger::createFile( const string &path )
{
	makeParentDir( path );

	AbstractFile *file = AbstractFile::open( globals::recordFileType, path.c_str(), "w" );

	if( _scope == SimulationStateScope )
		setSimulationState( file );

	return file;
}

//---------------------------------------------------------------------------
// AbstractFileLogger::getFile
//---------------------------------------------------------------------------
AbstractFile *AbstractFileLogger::getFile()
{
	return (AbstractFile *)getSimulationState();
}

//---------------------------------------------------------------------------
// AbstractFileLogger::createFile
//---------------------------------------------------------------------------
AbstractFile *AbstractFileLogger::createFile( agent *a, const string &path )
{
	makeParentDir( path );

	AbstractFile *file = AbstractFile::open( globals::recordFileType, path.c_str(), "w" );
	setAgentState( a, file );

	return file;
}

//---------------------------------------------------------------------------
// AbstractFileLogger::getFile
//---------------------------------------------------------------------------
AbstractFile *AbstractFileLogger::getFile( agent *a )
{
	return (AbstractFile *)getAgentState( a );
}


//===========================================================================
// DataLibLogger
//===========================================================================

//---------------------------------------------------------------------------
// DataLibLogger::DataLibLogger
//---------------------------------------------------------------------------
DataLibLogger::DataLibLogger()
	: Logger()
{
}

//---------------------------------------------------------------------------
// DataLibLogger::~DataLibLogger
//---------------------------------------------------------------------------
DataLibLogger::~DataLibLogger()
{
	if( (_scope == SimulationStateScope) && _record )
	{
		delete getWriter();
	}
}

//---------------------------------------------------------------------------
// DataLibLogger::createWriter
//---------------------------------------------------------------------------
DataLibWriter *DataLibLogger::createWriter( const string &path,
											bool randomAccess,
											bool singleSchema )
{
	makeParentDir( path );
	DataLibWriter *writer = new DataLibWriter( path.c_str(), randomAccess, singleSchema );

	if( _scope == SimulationStateScope )
		setSimulationState( writer );

	return writer;
}

//---------------------------------------------------------------------------
// DataLibLogger::getWriter
//---------------------------------------------------------------------------
DataLibWriter *DataLibLogger::getWriter()
{
	return (DataLibWriter *)getSimulationState();
}

//---------------------------------------------------------------------------
// DataLibLogger::createWriter
//---------------------------------------------------------------------------
DataLibWriter *DataLibLogger::createWriter( agent *a,
											const string &path,
											bool randomAccess,
											bool singleSchema )
{
	makeParentDir( path );
	DataLibWriter *writer = new DataLibWriter( path.c_str(), randomAccess, singleSchema );
	setAgentState( a, writer );

	return writer;
}

//---------------------------------------------------------------------------
// DataLibLogger::getWriter
//---------------------------------------------------------------------------
DataLibWriter *DataLibLogger::getWriter( agent *a )
{
	return (DataLibWriter *)getAgentState( a );
}
