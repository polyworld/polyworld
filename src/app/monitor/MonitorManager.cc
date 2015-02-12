#include "MonitorManager.h"

#include <map>
#include <vector>

#include "AgentTracker.h"
#include "CameraController.h"
#include "globals.h"
#include "Monitor.h"
#include "proplib.h"
#include "Resources.h"
#include "SceneRenderer.h"
#include "Simulation.h"


using namespace std;


//===========================================================================
// MonitorManager
//===========================================================================

MonitorManager::MonitorManager( TSimulation *_simulation,
								string monitorPath )
	: simulation( _simulation )
{
	proplib::DocumentBuilder builder;
	proplib::SchemaDocument *pschema = builder.buildSchemaDocument( "./etc/monitors.mfs" );
	proplib::Document *pdoc = builder.buildDocument( monitorPath );
	pschema->apply( pdoc );
	proplib::Document &doc = *pdoc;

	// ---
	// --- Charts
	// ---
	if( (bool)doc.get("BirthRate").get("Enabled") )
	{
		addMonitor( new BirthRateMonitor(simulation) );
	}
	if( (bool)doc.get("Fitness").get("Enabled") )
	{
		addMonitor( new FitnessMonitor(simulation) );
	}
	if( (bool)doc.get("FoodEnergy").get("Enabled") )
	{
		addMonitor( new FoodEnergyMonitor(simulation) );
	}
	if( (bool)doc.get("Population").get("Enabled") )
	{
		addMonitor( new PopulationMonitor(simulation) );
	}

	// ---
	// --- Agent Trackers
	// ---
	size_t ntrackers = doc.get( "AgentTrackers" ).size();
	for( size_t i = 0; i < ntrackers; i++ )
	{
		proplib::Property &propTracker = doc.get( "AgentTrackers" ).get( i );
		string name = propTracker.get( "Name" );
		string trackMode = propTracker.get( "TrackMode" );
		string selectionMode = propTracker.get( "SelectionMode" );

		AgentTracker::Parms parms;

		if( selectionMode == "Fitness" )
		{
			int rank = propTracker.get( "Fitness" ).get( "Rank" );
			bool trackTilDeath = trackMode == "Agent";
			
			parms = AgentTracker::Parms::createFitness( rank, trackTilDeath );
		}
		else
		{
			assert( false );
		}

		addAgentTracker( new AgentTracker(name, parms) );
	}

	// ---
	// --- Brain
	// ---
	if( (bool)doc.get("Brain").get("Enabled") )
	{
		int frequency = doc.get( "Brain" ).get( "Frequency" );
		string trackerName = doc.get( "Brain" ).get( "AgentTracker" );
		AgentTracker *tracker = findAgentTracker( trackerName );

		addMonitor( new BrainMonitor(simulation, frequency, tracker) );
	}


	// ---
	// --- POV
	// ---
	if( (bool)doc.get("POV").get("Enabled") )
	{
		addMonitor( new PovMonitor(simulation) );
	}

	// ---
	// --- Status Text
	// ---
	if( (bool)doc.get("StatusText").get("Enabled") )
	{
		addMonitor( new StatusTextMonitor(simulation,
										  doc.get("StatusText").get("FrequencyDisplay"),
										  doc.get("StatusText").get("FrequencyStore"),
										  doc.get("StatusText").get("StorePerformance")) );
	}

	// ---
	// --- Farm
	// ---
	if( FarmMonitor::isFarmEnv() && (bool)doc.get("Farm").get("Enabled") )
	{
		vector<FarmMonitor::Property> properties;

		itfor( proplib::PropertyMap, doc.get("Farm").get("Properties").elements(), it )
		{
			properties.push_back( FarmMonitor::Property(it->second->get("Name"),
														it->second->get("Title")) );
		}

		addMonitor( new FarmMonitor(simulation,
									doc.get("Farm").get("Frequency"),
									properties) );
	}

	// ---
	// --- Scenes
	// ---
	{
		const char *scenes[] = { "MainScene", "OverheadScene", "FittestPOVScene" };
		int nscenes = sizeof(scenes) / sizeof(char*);

		for( int iscene = 0; iscene < nscenes; iscene++ )
		{
			proplib::Property &propScene = doc.get( scenes[iscene] );

			// ---
			// --- Enabled
			// ---
			if( !(bool)propScene.get("Enabled") )
				continue;

			// ---
			// --- Camera Settings
			// ---
			SceneRenderer::CameraProperties cameraProperties;
			{
				string cameraSettingsName = propScene.get( "CameraSettings" );
				size_t nsettings = doc.get( "CameraSettings" ).size();
				bool found = false;
				for( size_t i = 0; i < nsettings; i++ )
				{
					proplib::Property &propSettings = doc.get( "CameraSettings" ).get( i );
			
					string name = propSettings.get( "Name" );
					if( name == cameraSettingsName )
					{
						found = true;
						Color color = propSettings.get( "Color" );
						float fov = propSettings.get( "FieldOfView" );
			
						cameraProperties = SceneRenderer::CameraProperties( color, fov );
						break;
					}
				}

				if( !found )
				{
					cerr << "Invalid CameraSettings Name: " << cameraSettingsName << endl;
					exit( 1 );
				}
			}

			// ---
			// --- Buffer
			// ---
			int bufferWidth = propScene.get( "Buffer" ).get( "Width" );
			int bufferHeight = propScene.get( "Buffer" ).get( "Height" );

			// ---
			// --- Construct Renderer
			// ---
			SceneRenderer *renderer = new SceneRenderer( simulation->getStage(),
														 cameraProperties,
														 bufferWidth,
														 bufferHeight );

			// ---
			// --- Camera Controller
			// ---
			CameraController *cameraController = NULL;
			{
				string cameraControllerSettingsName = propScene.get( "CameraControllerSettings" );
				size_t nsettings = doc.get( "CameraControllerSettings" ).size();
				for( size_t i = 0; (i < nsettings) && (cameraController == NULL); i++ )
				{
					proplib::Property &propSettings = doc.get( "CameraControllerSettings" ).get( i );

					string name = propSettings.get( "Name" );
					if( name == cameraControllerSettingsName )
					{
						string mode = propSettings.get( "Mode" );

						if( mode == "Rotate" )
						{
							proplib::Property &propMode = propSettings.get( "Rotate" );

							float radius = propMode.get( "Radius" );
							float height = propMode.get( "Height" );
							float rate = propMode.get( "Rate" );
							float angleStart = propMode.get( "AngleStart" );
							proplib::Property &propFixation = propMode.get( "Fixation" );
							float fixX = propFixation.get( "X" );
							float fixY = propFixation.get( "Y" );
							float fixZ = propFixation.get( "Z" );
				
							cameraController = new CameraController( renderer->getCamera() );

							CameraController::RotationParms rotationParms( radius,
																		   height,
																		   rate,
																		   angleStart,
																		   fixX * globals::worldsize,
																		   fixY,
																		   -1 * fixZ * globals::worldsize );
							cameraController->initRotation( rotationParms );
						}
						else if( mode == "AgentTracking" )
						{
							proplib::Property &propMode = propSettings.get( "AgentTracking" );
							AgentTracker *tracker = findAgentTracker( propMode.get("AgentTracker") );

							string perspectiveName = propMode.get( "Perspective" );
							CameraController::AgentTrackingParms::Perspective perspective;
							if( perspectiveName == "Overhead" )
								perspective = CameraController::AgentTrackingParms::OVERHEAD;
							else if( perspectiveName == "POV" )
								perspective = CameraController::AgentTrackingParms::POV;
							else
								assert(false);

							cameraController = new CameraController( renderer->getCamera() );

							CameraController::AgentTrackingParms agentTrackingParms( tracker,
																					 perspective );

							cameraController->initAgentTracking( agentTrackingParms );
						}
					}
				}

				if( cameraController == NULL )
				{
					cerr << "Invalid CameraControllerSettings Name: " << cameraControllerSettingsName << endl;
					exit( 1 );
				}
			}

			// ---
			// --- Movie Settings
			// ---
			bool recordMovie = propScene.get( "Movie" ).get( "Record" );
			string moviePath = string("run/") + (string)propScene.get( "Movie" ).get( "Path" );
			int sampleFrequency = propScene.get( "Movie" ).get( "SampleFrequency" );
			int sampleDuration = propScene.get( "Movie" ).get( "SampleDuration" );

			MovieSettings movieSettings = MovieSettings( recordMovie, moviePath, sampleFrequency, sampleDuration);


			// ---
			// --- Create Scene Monitor
			// ---
			string name = propScene.get( "Name" );
			string title = propScene.get( "Title" );

			addMonitor( new SceneMonitor(simulation, name, title, renderer, cameraController, movieSettings) );
		}
	}

#if false
	// Gene separation
	fGeneSeparationWindow = new TBinChartWindow( "gene separation", "GeneSeparation" );
//	fGeneSeparationWindow->setWindowTitle( "gene separation" );
	fGeneSeparationWindow->SetRange(0.0, 1.0);
	fGeneSeparationWindow->SetExponent(0.5);
	fGeneSeparationWindow->show();
#endif
}

MonitorManager::~MonitorManager()
{
	itfor( Monitors, monitors, it )
	{
		delete *it;
	}

	itfor( AgentTrackers, agentTrackers, it )
	{
		delete *it;
	}
}

const Monitors &MonitorManager::getMonitors()
{
	return monitors;
}

const AgentTrackers &MonitorManager::getAgentTrackers()
{
	return agentTrackers;
}

AgentTracker *MonitorManager::findAgentTracker( string name )
{
	itfor( AgentTrackers, agentTrackers, it )
	{
		if( name == (*it)->getName() )
		{
			return *it;
		}
	}

	cerr << "Invalid AgentTracker name: " << name << endl;
	exit( 1 );
}

void MonitorManager::step()
{
	itfor( AgentTrackers, agentTrackers, it )
	{
		AgentTracker *tracker = *it;
		const AgentTracker::Parms &parms = tracker->getParms();
		agent *target = tracker->getTarget();

		if( (target == NULL) || !parms.trackTilDeath )
		{
			switch( parms.mode )
			{
			case AgentTracker::FITNESS:
				tracker->setTarget( simulation->getCurrentFittest(parms.fitness.rank) );
				break;
			default:
				assert( false );
			}
		}
	}

	itfor( Monitors, monitors, it )
	{
		(*it)->step( simulation->getStep() );
	}
}

void MonitorManager::dump( ostream &out )
{
	itfor( Monitors, monitors, it )
	{
		(*it)->dump( out );
	}
}

void MonitorManager::addMonitor( Monitor *monitor )
{
	monitors.push_back( monitor );
}

void MonitorManager::addAgentTracker( AgentTracker *tracker )
{
	agentTrackers.push_back( tracker );
}

