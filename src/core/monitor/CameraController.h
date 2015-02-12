#pragma once

#include "gcamera.h"

class CameraController
{
 public:
	class RotationParms
	{
	public:
		RotationParms( float _radius,
					   float _height,
					   float _rate,
					   float _angleStart,
					   float _fixationPointX,
					   float _fixationPointY,
					   float _fixationPointZ )
		: radius(_radius), height(_height), rate(_rate), angleStart(_angleStart)
		{
			fixationPoint[0] = _fixationPointX;
			fixationPoint[1] = _fixationPointY;
			fixationPoint[2] = _fixationPointZ;
		}

	private:
		friend class CameraController;

		float radius;
		float height;
		float rate;
		float angleStart;
		float fixationPoint[3];

		RotationParms() {}
	};

	class AgentTrackingParms
	{
	public:
		enum Perspective
		{
			OVERHEAD,
			POV
		};

		AgentTrackingParms( class AgentTracker *_tracker, Perspective _perspective )
			: tracker(_tracker), perspective(_perspective)
		{}
		
	private:
		friend class CameraController;
							
		class AgentTracker *tracker;
		Perspective perspective;

		AgentTrackingParms() {}
	};

	CameraController( gcamera &camera );
	virtual ~CameraController();

	class AgentTracker *getAgentTracker();

	void step();

	void initRotation( const RotationParms &parms );
	void initAgentTracking( const AgentTrackingParms &parms );

 private:
	void setRotationAngle( float angle );
	void setAgentTrackingTarget();

 private:
	gcamera &camera;

	enum Mode
	{
		MODE__UNDEFINED,
		MODE__ROTATE,
		MODE__AGENT_TRACKING
	} mode;

	struct RotationState
	{
		RotationParms parms;
		float angle;
	} rotationState;

	struct AgentTrackingState
	{
		AgentTrackingParms parms;
	} agentTrackingState;
};
