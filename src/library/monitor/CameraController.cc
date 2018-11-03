#include "CameraController.h"

#include <assert.h>

#include "AgentTracker.h"
#include "agent/agent.h"
#include "sim/globals.h"

CameraController::CameraController( gcamera &_camera )
: camera( _camera )
{
	mode = MODE__UNDEFINED;
}

CameraController::~CameraController()
{
}

AgentTracker *CameraController::getAgentTracker()
{
	return mode == MODE__AGENT_TRACKING
		? agentTrackingState.parms.tracker
		: NULL;
}

void CameraController::step()
{
	switch( mode )
	{
	case MODE__ROTATE:
		setRotationAngle( rotationState.angle + rotationState.parms.rate );
		break;
	case MODE__AGENT_TRACKING:
		setAgentTrackingTarget();
		break;
	case MODE__STATIC:
		break;
	default:
		assert(false);
		break;
	}
}

void CameraController::initRotation( const RotationParms &parms )
{
	mode = MODE__ROTATE;
	rotationState.parms = parms;

	camera.SetFixationPoint( parms.fixationPoint[0], parms.fixationPoint[1], parms.fixationPoint[2] );
	camera.SetRotation( 0.0, 90.0, 0.0 );

	setRotationAngle( parms.angleStart );
}

void CameraController::initAgentTracking( const AgentTrackingParms &parms )
{
	mode = MODE__AGENT_TRACKING;
	agentTrackingState.parms = parms;

	setAgentTrackingTarget();
}

void CameraController::initStatic( const StaticParms &parms )
{
	mode = MODE__STATIC;

	camera.SetRotation( 0.0, -90, 0.0 );
	camera.settranslation( 0.5 * globals::worldsize,
						   parms.height * globals::worldsize,
						   -0.5 * globals::worldsize );
}

void CameraController::setRotationAngle( float angle )
{
	RotationParms &parms = rotationState.parms;
	rotationState.angle = angle;
	float camrad = rotationState.angle * DEGTORAD;
	camera.settranslation( (0.5 + parms.radius * sin(camrad)) * globals::worldsize,
						   parms.height * globals::worldsize,
						   (-0.5 + parms.radius * cos(camrad)) * globals::worldsize );
}

void CameraController::setAgentTrackingTarget()
{
	agent *target = agentTrackingState.parms.tracker->getTarget();

	if( target )
	{
		switch( agentTrackingState.parms.perspective )
		{
		case AgentTrackingParms::OVERHEAD:
			camera.SetRotation( 0.0, -90, 0.0 );
			camera.settranslation( target->x(), 0.2 * globals::worldsize, target->z() );
			break;
		case AgentTrackingParms::POV:
			{
				camera.AttachTo( target );

				gcamera &agentCamera = target->getCamera();
				camera.settranslation( agentCamera.x(), agentCamera.y(), agentCamera.z() );
				camera.SetRotation( agentCamera.getyaw(),
									agentCamera.getpitch(),
									agentCamera.getroll() );
			}
			break;
		default:
			assert( false );
		}
				
	}
	else
	{
		camera.SetRotation( 0.0, 90, 0.0 );
		camera.settranslation( 0.0, 100.0, 0.0 );
	}
}
