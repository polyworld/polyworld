#pragma once

#include <iostream>
#include <string>
#include <vector>

#include <QObject>

#include "datalib.h"
#include "MovieController.h"
#include "proplib.h"
#include "simconst.h"
#include "simtypes.h"


//===========================================================================
// Monitor
//===========================================================================
class Monitor : public QObject
{
	Q_OBJECT

 public:
	enum Type
	{
		CHART,
		BRAIN,
		POV,
		STATUS_TEXT,
		FARM,
		SCENE
	};

	Monitor( Type _type,
			 class TSimulation *_sim,
			 std::string _id,
			 std::string _name,
			 std::string _title );
	virtual ~Monitor();

	Type getType();
	const char *getId();
	const char *getName();
	const char *getTitle();
	class TSimulation *getSimulation();

	virtual void step( long timestep ) = 0;

	virtual void dump( std::ostream &out );

 protected:
	class TSimulation *sim;

 private:
	Type type;
	std::string id;
	std::string name;
	std::string title;
};

//===========================================================================
// ChartMonitor
//===========================================================================
class ChartMonitor : public Monitor
{
	Q_OBJECT

 public:
	virtual ~ChartMonitor();

	class CurveDef
	{
	public:
		int id;
		float range[2];
		float color[3];
	}; 
	typedef std::vector<CurveDef> CurveDefs;
	
	const CurveDefs &getCurveDefs() { return curves; }

 signals:
	void curveUpdated( short curve, float data );

 protected:
	ChartMonitor( class TSimulation *_sim,
				  std::string _id,
				  std::string _name,
				  std::string _title );

	void defineCurve( float rmin, float rmax, float r = -1, float g = -1, float b = -1);

 private:
	CurveDefs curves;
};

//===========================================================================
// BirthRateMonitor
//===========================================================================
class BirthRateMonitor : public ChartMonitor
{
 public:
	BirthRateMonitor( class TSimulation *_sim );
	virtual ~BirthRateMonitor();

	virtual void step( long timestep );

 private:
	sim::AgentBirthType birthType;
	long prevBorn;
	long prevCreated;
};

//===========================================================================
// FitnessMonitor
//===========================================================================
class FitnessMonitor : public ChartMonitor
{
 public:
	FitnessMonitor( class TSimulation *_sim );
	virtual ~FitnessMonitor();

	virtual void step( long timestep );
};

//===========================================================================
// FoodEnergyMonitor
//===========================================================================
class FoodEnergyMonitor : public ChartMonitor
{
 public:
	FoodEnergyMonitor( class TSimulation *_sim );
	virtual ~FoodEnergyMonitor();

	virtual void step( long timestep );

	void updateCurve( int curve, sim::FoodEnergyStatScope scope );
};

//===========================================================================
// PopulationMonitor
//===========================================================================
class PopulationMonitor : public ChartMonitor
{
 public:
	PopulationMonitor( class TSimulation *_sim );
	virtual ~PopulationMonitor();

	virtual void step( long timestep );
};

//===========================================================================
// BrainMonitor
//===========================================================================
class BrainMonitor : public Monitor
{
	Q_OBJECT

 public:
	BrainMonitor( class TSimulation *_sim, int _frequency, class AgentTracker *_tracker );

	class AgentTracker *getTracker();

	virtual void step( long timestep );

 signals:
	void update();

 private:
	int frequency;
	class AgentTracker *tracker;
};

//===========================================================================
// PovMonitor
//===========================================================================
class PovMonitor : public Monitor
{
 public:
	PovMonitor( class TSimulation *_sim );

	class AgentPovRenderer *getRenderer();

	virtual void step( long timestep );
};

//===========================================================================
// StatusTextMonitor
//===========================================================================
class StatusTextMonitor : public Monitor
{
	Q_OBJECT

 public:
	StatusTextMonitor( class TSimulation *sim,
					   int frequencyDisplay,
					   int frequencyStore,
					   bool storePerformance );

	sim::StatusText &getStatusText();

	virtual void step( long timestep );

 signals:
	void update();

 private:
	sim::StatusText statusText;
	int frequencyDisplay;
	int frequencyStore;
	bool storePerformance;
};

//===========================================================================
// FarmMonitor
//===========================================================================
class FarmMonitor : public Monitor
{
	Q_OBJECT

 public:
	static bool isFarmEnv();

	class Property
	{
	public:
		Property( std::string name_, std::string title_ ) : name(name_), title(title_), metadata(NULL) {}

		std::string name;
		std::string title;
		proplib::CppProperties::PropertyMetadata *metadata;
	};

	FarmMonitor( class TSimulation *sim,
				 int frequency,
				 const std::vector<Property> &properties );

	virtual void step( long timestep );

 private:
	int _frequency;
	std::vector<Property> _properties;
};

//===========================================================================
// SceneMonitor
//===========================================================================
class SceneMonitor : public Monitor
{
 public:
	SceneMonitor( class TSimulation *sim,
				  std::string id,
				  std::string name,
				  class SceneRenderer *renderer,
				  class CameraController *cameraController,
				  const MovieSettings &movieSettings );
	virtual ~SceneMonitor();

	class SceneRenderer *getRenderer();
	class CameraController *getCameraController();

	virtual void step( long timestep );

 private:
	class SceneRenderer *renderer;
	class CameraController *cameraController;
	class SceneMovieController *movieController;
};
