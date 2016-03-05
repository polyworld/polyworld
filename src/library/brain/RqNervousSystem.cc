#include "RqNervousSystem.h"

#include <string.h>

#include "Nerve.h"
#include "agent/agent.h"
#include "agent/RqSensor.h"
#include "genome/Genome.h"
#include "utils/misc.h"

using namespace genome;


RqNervousSystem::~RqNervousSystem()
{
	itfor( SensorList, sensors, it )
		delete *it;
}

void RqNervousSystem::grow( Genome *g )
{
	createInput( "Random" );
	createInput( "Energy" );
	if( agent::config.enableMateWaitFeedback )
	{
		createInput( "MateWaitFeedback" );
	}
	if( agent::config.enableSpeedFeedback )
	{
		createInput( "SpeedFeedback" );
	}
	if( agent::config.enableCarry )
	{
		createInput( "Carrying" );
		createInput( "BeingCarried" );
	}
	createInput( "Red" );
	createInput( "Green" );
	createInput( "Blue" );

	createOutput( "Eat" );
	createOutput( "Mate" );
	createOutput( "Fight" );
	createOutput( "Speed" );
	createOutput( "Yaw" );
	if( agent::config.yawEncoding == agent::YE_OPPOSE )
	{
		createOutput( "YawOppose" );
	}
	if( agent::config.hasLightBehavior )
	{
		createOutput( "Light" );
	}
	createOutput( "Focus" );
	if( agent::config.enableVisionPitch )
	{
		createOutput( "VisionPitch" );
	}
	if( agent::config.enableVisionYaw )
	{
		createOutput( "VisionYaw" );
	}
	if( agent::config.enableGive )
	{
		createOutput( "Give" );
	}
	if( agent::config.enableCarry )
	{
		createOutput( "Pickup" );
		createOutput( "Drop" );
	}

	NervousSystem::grow( g );
}

void RqNervousSystem::setMode( Mode mode )
{
	itfor( SensorList, sensors, it )
	{
		RqSensor *sensor = (RqSensor *)(*it);
		sensor->setMode( mode );
	}
}

void RqNervousSystem::createInput( const std::string &name )
{
	createNerve( Nerve::INPUT, name );
	RqSensor *sensor = new RqSensor( name, getRNG() );
	addSensor( sensor );
}

void RqNervousSystem::createOutput( const std::string &name )
{
	createNerve( Nerve::OUTPUT, name );
}
