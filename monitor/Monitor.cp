#include "Monitor.h"

#include <iostream>
#include <sstream>

#include "AgentTracker.h"
#include "CameraController.h"
#include "MovieController.h"
#include "objectxsortedlist.h"
#include "SceneRenderer.h"
#include "Simulation.h"

using namespace std;

//===========================================================================
// Monitor
//===========================================================================
Monitor::Monitor( Type _type,
				  TSimulation *_sim,
				  string _id,
				  string _name,
				  string _title )
	: sim(_sim)
	, type(_type)
	, id(_id)
	, name(_name)
	, title(_title)
{
}

Monitor::~Monitor()
{
}

Monitor::Type Monitor::getType()
{
	return type;
}

const char *Monitor::getId()
{
	return id.c_str();
}

const char *Monitor::getName()
{
	return name.c_str();
}

const char *Monitor::getTitle()
{
	return title.c_str();
}

TSimulation *Monitor::getSimulation()
{
	return sim;
}

void Monitor::dump( ostream &out )
{
}

//===========================================================================
// ChartMonitor
//===========================================================================
ChartMonitor::ChartMonitor( TSimulation *_sim,
							string _id,
							string _name,
							string _title )
	: Monitor(CHART, _sim, _id, _name, _title)
{
}

ChartMonitor::~ChartMonitor() {}

void ChartMonitor::defineCurve( float rmin, float rmax, float r, float g, float b)
{
	CurveDef c;
	c.id = (int)curves.size();
	c.range[0] = rmin; c.range[1] = rmax;
	c.color[0] = r; c.color[1] = g; c.color[2] = b;
	curves.push_back( c );
}

//===========================================================================
// BirthRateMonitor
//===========================================================================
BirthRateMonitor::BirthRateMonitor( TSimulation *_sim )
	: ChartMonitor(_sim, "birthrate", "Birth Rate", "born / (born + created)")
	, prevBorn(0),
	prevCreated(0)
{
	if( _sim->isLockstep()
		|| (_sim->getFitnessWeight(FWT__COMPLEXITY) != 0.0)
		|| (_sim->getFitnessWeight(FWT__HEURISTIC) != 0.0) )
	{
		birthType = ABT__BORN_VIRTUAL;
	}
	else
	{
		birthType = ABT__BORN;
	}

	defineCurve( 0.0, 1.0 );
}

BirthRateMonitor::~BirthRateMonitor()
{
}

void BirthRateMonitor::step( long timestep )
{
	long numBorn = sim->getNumBorn( birthType );
	long numCreated = sim->getNumBorn( ABT__CREATED );

	if( (numBorn != prevBorn) || (numCreated != prevCreated) )
	{
		prevBorn = numBorn;
		prevCreated = numCreated;

		emit curveUpdated( 0, float(numBorn) / float(numBorn + numCreated) );
	}
}

//===========================================================================
// FitnessMonitor
//===========================================================================
FitnessMonitor::FitnessMonitor( TSimulation *_sim )
	: ChartMonitor(_sim, "fitness", "Fitness", "maxfit, curmaxfit, avgfit")
{
	defineCurve( 0.0, 1.0,
				 1.0, 1.0, 1.0 );
	defineCurve( 0.0, 1.0,
				 1.0, 0.3, 0.0 );
	defineCurve( 0.0, 1.0,
				 0.0, 1.0, 1.0 );
}

FitnessMonitor::~FitnessMonitor()
{
}

void FitnessMonitor::step( long timestep )
{
	emit curveUpdated( 0, sim->getFitnessStat(FST__MAX_FITNESS) );
	emit curveUpdated( 1, sim->getFitnessStat(FST__CURRENT_MAX_FITNESS) );
	emit curveUpdated( 2, sim->getFitnessStat(FST__AVERAGE_FITNESS) );
}

//===========================================================================
// FoodEnergyMonitor
//===========================================================================
FoodEnergyMonitor::FoodEnergyMonitor( TSimulation *_sim )
	: ChartMonitor(_sim, "foodenergy", "Food Energy", "energy in, total, avg")
{
	defineCurve( -1.0, 1.0 );
	defineCurve( -1.0, 1.0 );
	defineCurve( -1.0, 1.0 );					 
}

FoodEnergyMonitor::~FoodEnergyMonitor()
{
}

void FoodEnergyMonitor::step( long timestep )
{
	updateCurve( 0, FESS__STEP );
	updateCurve( 1, FESS__TOTAL );
	updateCurve( 2, FESS__AVERAGE );
}

void FoodEnergyMonitor::updateCurve( int curve, FoodEnergyStatScope scope )
{
	float in = sim->getFoodEnergyStat( FEST__IN, scope );
	float out = sim->getFoodEnergyStat( FEST__OUT, scope );

	emit curveUpdated( curve, (in - out) / (in + out) );
}

//===========================================================================
// PopulationMonitor
//===========================================================================
PopulationMonitor::PopulationMonitor( TSimulation *_sim )
	: ChartMonitor(_sim, "population", "Population", "Population")
{
	float colors[][3] =
		{
			{ 1.0, 0.0, 0.0 },
			{ 0.0, 1.0, 0.0 },
			{ 0.0, 0.0, 1.0 },
			{ 0.0, 1.0, 1.0 },
			{ 1.0, 0.0, 1.0 },
			{ 1.0, 1.0, 0.0 },
			{ 1.0, 1.0, 1.0 }
		};
	const int ncolors = sizeof(colors) / (3*sizeof(float));
	int npops = (sim->GetNumDomains() < 2) ? 1 : (sim->GetNumDomains() + 1);
	assert( ncolors >= npops );

	for( int i = 0; i < npops; i++ )
		defineCurve(0, sim->GetMaxAgents(),
					colors[i][0], colors[i][1], colors[i][2]);
}

PopulationMonitor::~PopulationMonitor()
{
}

void PopulationMonitor::step( long timestep )
{
	emit curveUpdated( 0, sim->getNumAgents() );
	if( sim->GetNumDomains() > 1 )
	{
		for( short domain = 0; domain < sim->GetNumDomains(); domain++ )
			emit curveUpdated( domain + 1, sim->getNumAgents(domain) );
	}
}

//===========================================================================
// BrainMonitor
//===========================================================================
BrainMonitor::BrainMonitor( TSimulation *_sim, int _frequency, AgentTracker *_tracker )
	: Monitor(BRAIN, _sim, "brainmonitor", "Brain Monitor", "Brain Monitor")
	, frequency(_frequency)
	, tracker(_tracker)
{
}

AgentTracker *BrainMonitor::getTracker()
{
	return tracker;
}

void BrainMonitor::step( long timestep )
{
	if( (timestep % frequency) == 0 )
		emit update();
}

//===========================================================================
// PovMonitor
//===========================================================================
PovMonitor::PovMonitor( TSimulation *_sim )
	: Monitor(POV, _sim, "pov", "POV", "POV")
{
}

AgentPovRenderer *PovMonitor::getRenderer()
{
	return sim->GetAgentPovRenderer();
}

void PovMonitor::step( long timestep )
{
	// noop
}

//===========================================================================
// StatusTextMonitor
//===========================================================================
StatusTextMonitor::StatusTextMonitor( TSimulation *_sim,
									  int _frequencyDisplay,
									  int _frequencyStore,
									  bool _storePerformance )
	: Monitor(STATUS_TEXT, _sim, "textstatus", "Text Status", "Text Status")
	, frequencyDisplay( _frequencyDisplay )
	, frequencyStore( _frequencyStore )
	, storePerformance( _storePerformance )
{
}

StatusText &StatusTextMonitor::getStatusText()
{
	return statusText;
}

void StatusTextMonitor::step( long timestep )
{
	bool doDisplay =
		((timestep == 1) || (timestep % frequencyDisplay == 0))
		&& (receivers( SIGNAL(update()) ) > 0);
	bool doStore =
		((timestep == 1) || (timestep % frequencyStore == 0));

	if( doDisplay || doStore )
	{
		itfor( StatusText, statusText, it )
		{
			free( *it );
		}
		statusText.clear();

		getSimulation()->getStatusText( statusText, frequencyStore );

		if( doDisplay )
		{
			emit update();
		}

		if( doStore )
		{
			char statusFileName[256];
		
			sprintf( statusFileName, "run/stats/stat.%ld", timestep );
			makeParentDir( statusFileName );

			FILE *statusFile = fopen( statusFileName, "w" );
			Q_CHECK_PTR( statusFile );

			StatusText::const_iterator iter = statusText.begin();
			for( ; iter != statusText.end(); ++iter )
			{
				// filter out performance stats
				if( storePerformance || (0 != strncmp( *iter, "Rate", 4 )) )
					fprintf( statusFile, "%s\n", *iter );
			}

			fclose( statusFile );
		}
	}
}

//===========================================================================
// FarmMonitor
//===========================================================================
bool FarmMonitor::isFarmEnv()
{
	return getenv("PWFARM_STATUS") != NULL;
}

FarmMonitor::FarmMonitor( TSimulation *sim,
						  int frequency,
						  const vector<Property> &properties )
: Monitor(FARM, sim, "farm", "Farm", "Farm")
, _frequency( frequency )
, _properties( properties )
{
	int nmetadata;
	proplib::CppProperties::PropertyMetadata *metadata;
	proplib::CppProperties::getMetadata( &metadata, &nmetadata );

	itfor( vector<Property>, _properties, it )
	{
		for( int i = 0; i < nmetadata; i++ )
		{
			if( it->name == metadata[i].name )
			{
				it->metadata = metadata + i;
				break;
			}
		}
	}
}

void FarmMonitor::step( long timestep )
{
	if( (timestep == 1) || (timestep % _frequency == 0) )
	{
		stringstream cmd;

		cmd << "bash -c 'PWFARM_STATUS Polyworld \"[";

		bool first = true;
		itfor( vector<Property>, _properties, it )
		{
			if( it->metadata )
			{
				if( !first )
				{
					cmd << " ";
				}
				else
					first = false;

				cmd << it->title << "=" << it->metadata->toString();
			}
		}

		cmd << "]\"'";

		int rc = system( cmd.str().c_str() );
		if( rc ) cerr << "Failed executing PWFARM_STATUS" << endl;
	}
}

//===========================================================================
// SceneMonitor
//===========================================================================
SceneMonitor::SceneMonitor( TSimulation *_sim,
							string _id,
							string _name,
							SceneRenderer *_renderer,
							CameraController *_cameraController,
							const MovieSettings &_movieSettings )
	: Monitor(SCENE, _sim, _id, _name, _name)
	, renderer(_renderer)
	, cameraController(_cameraController)
	, movieController(NULL)
{
	if( _movieSettings.shouldRecord() )
		movieController = new SceneMovieController( renderer, _movieSettings );
}

SceneMonitor::~SceneMonitor()
{
	if( movieController ) delete movieController;
}

SceneRenderer *SceneMonitor::getRenderer()
{
	return renderer;
}

CameraController *SceneMonitor::getCameraController()
{
	return cameraController;
}

void SceneMonitor::step( long timestep )
{
	if( movieController )
		movieController->step( timestep );

	cameraController->step();
	renderer->render();
}
