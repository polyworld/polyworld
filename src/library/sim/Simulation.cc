// The first batch of Debug flags below are the common ones to enable for figuring out problems in the Interact()
#define TEXTTRACE 0
#define DebugShowSort 0
#define DebugShowBirths 0
#define DebugShowEating 0
#define DebugGeneticAlgorithm 0
#define DebugSmite 0

#define DebugMaxFitness 0
#define DebugLinksEtAl 0
#define DebugDomainFoodBands 0
#define DebugLockStep 0

#include <limits>

#define MinDebugStep 0
#define MaxDebugStep INT_MAX

#define CurrentWorldfileVersion 55

// CompatibilityMode makes the new code with a single x-sorted list behave *almost* identically to the old code.
// Discrepancies still arise due to the old food list never being re-sorted and agents at the exact same x location
// not always ending up sorted in the same order.  [Food centers remain sorted as long as they exist, but these lists
// are actually sorted on x at the left edge (x increases from left to right) of the objects, which changes as the
// food is eaten and shrinks.]
#define CompatibilityMode 1

// Self
#include "Simulation.h"

// System
#include <fstream>
#include <functional>
#include <iostream>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <assert.h>

// Local
#include "debug.h"
#include "globals.h"

#include "agent/AgentPovRenderer.h"
#include "agent/Metabolism.h"
#include "brain/Brain.h"
#include "brain/groups/GroupsBrain.h"
#include "brain/sheets/SheetsBrain.h"
#include "complexity/complexity.h"
#include "environment/barrier.h"
#include "environment/brick.h"
#include "environment/BrickPatch.h"
#include "environment/food.h"
#include "environment/FoodPatch.h"
#include "environment/Patch.h"
#include "genome/Genome.h"
#include "genome/GenomeUtil.h"
#include "logs/Logs.h"
#include "proplib/proplib.h"
#include "utils/AbstractFile.h"
#include "utils/objectxsortedlist.h"
#include "utils/PwMovieUtils.h"
#include "utils/RandomNumberGenerator.h"
#include "utils/Resources.h"


using namespace genome;
using namespace std;


// Define directory mode mask the same, except you need execute privileges to use as a directory (go fig)
#define	PwDirMode ( S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH )

struct SheetSynapseType { sheets::Sheet::Type from, to; };
static vector<SheetSynapseType> SheetSynapseTypes =
	{
		{ sheets::Sheet::Input, sheets::Sheet::Internal },
		{ sheets::Sheet::Input, sheets::Sheet::Output },
		{ sheets::Sheet::Internal, sheets::Sheet::Internal },
		{ sheets::Sheet::Internal, sheets::Sheet::Output },
		{ sheets::Sheet::Output, sheets::Sheet::Internal },
		{ sheets::Sheet::Output, sheets::Sheet::Output },
	};
//===========================================================================
// TSimulation
//===========================================================================

static long numglobalcreated = 0;    // needs to be static so we only get warned about influence of global creations once ever

long TSimulation::fMaxNumAgents;
long TSimulation::fStep;
double TSimulation::fFramesPerSecondOverall;
double TSimulation::fSecondsPerFrameOverall;
double TSimulation::fFramesPerSecondRecent;
double TSimulation::fSecondsPerFrameRecent;
double TSimulation::fFramesPerSecondInstantaneous;
double TSimulation::fSecondsPerFrameInstantaneous;
double TSimulation::fTimeStart;

//---------------------------------------------------------------------------
// Prototypes
//---------------------------------------------------------------------------

float AverageAngles( float a, float b );

//---------------------------------------------------------------------------
// Inline functions
//---------------------------------------------------------------------------

// Average two angles ranging from 0.0 to 360.0, dealing with the fact that you can't
// just sum and divide by two when they are separated numerically by more than 180.0,
// and making sure the result stays less than 360 degrees.
inline float AverageAngles( float a, float b )
{
	float c;

	if( fabs( a - b ) > 180.0 )
	{
		c = 0.5 * (a + b)  +  180.0;
		if( c >= 360.0 )
			c -= 360.0;
	}
	else
		c = 0.5 * (a + b);

	return( c );
}


//---------------------------------------------------------------------------
// Macros
//---------------------------------------------------------------------------

#if TEXTTRACE
	#define ttPrint( x... ) { if( (fStep >= MinDebugStep) && (fStep <= MaxDebugStep) ) printf( x ); }
#else
	#define ttPrint( x... )
#endif

#if DebugSmite
	#define smPrint( x... ) { if( (fStep >= MinDebugStep) && (fStep <= MaxDebugStep) ) printf( x ); }
#else
	#define smPrint( x... )
#endif

#if DebugLinksEtAl
#define link( x... ) { if( (fStep >= MinDebugStep) && (fStep <= MaxDebugStep) ) { ( printf( "%lu link %s to %s (%s/%d)\n", fStep, s, t, __FUNCTION__, __LINE__ ), AbstractFile::link( x ) ) } }
	#define unlink( x... ) { if( (fStep >= MinDebugStep) && (fStep <= MaxDebugStep) ) { ( printf( "%lu unlink %s (%s/%d)\n", fStep, s, __FUNCTION__, __LINE__ ), AbstractFile::unlink( x ) ) } }
	#define rename( x... ) { if( (fStep >= MinDebugStep) && (fStep <= MaxDebugStep) ) { ( printf( "%lu rename %s to %s (%s/%d)\n", fStep, s, t, __FUNCTION__, __LINE__ ), AbstractFile::rename( x ) ) } }
#else
#define link( x... ) AbstractFile::link( x )
	#define unlink( x... ) AbstractFile::unlink( x )
	#define rename( x... ) AbstractFile::rename( x )
#endif

#if DebugShowBirths
	#define birthPrint( x... ) { if( (fStep >= MinDebugStep) && (fStep <= MaxDebugStep) ) printf( x ); }
#else
	#define birthPrint( x... )
#endif

#if DebugShowEating
	#define eatPrint( x... ) { if( (fStep >= MinDebugStep) && (fStep <= MaxDebugStep) ) printf( x ); }
#else
	#define eatPrint( x... )
#endif

#if DebugGeneticAlgorithm
	#define gaPrint( x... ) { if( (fStep >= MinDebugStep) && (fStep <= MaxDebugStep) ) printf( x ); }
#else
	#define gaPrint( x... )
#endif

#if DebugLockStep
	FILE* fdls = NULL;
	#define lsPrint( x... ) { if( (fStep >= MinDebugStep) && (fStep <= MaxDebugStep) ) fprintf( fdls, x ); }
#else
	#define lsPrint( x... )
#endif

#define IS_PREVENTED_BY_CARRY( ACTION, AGENT ) \
	( (fCarryPrevents##ACTION != 0) && ((AGENT)->NumCarries() > 0) && (randpw() < fCarryPrevents##ACTION) )


//---------------------------------------------------------------------------
// TSimulation::TSimulation
//---------------------------------------------------------------------------
TSimulation::TSimulation( string worldfilePath, proplib::ParameterMap parameters )
	:
		fLockStepWithBirthsDeathsLog(false),
		fLockstepFile(NULL),

		fMaxPopulationPenaltyFraction(0.0),
		fLowPopulationAdvantageFactor(1.0),
		fPopulationPenaltyFraction(0.0),
		fGlobalEnergyScaleFactor(1.0),

		fEvents(NULL),

		fLoadState(false),

		fCalcFoodPatchAgentCounts(true),
		fCalcComplexity(false),

		fFitI(0),
		fFitJ(1),
		fMaxFitness(0.),
		fAverageFitness(0.),

		fNumberAlive(0),
		fNumberBorn(0),
		fNumberBornVirtual(0),
		fNumberDied(0),
		fNumberDiedAge(0),
		fNumberDiedEnergy(0),
		fNumberDiedFight(0),
		fNumberDiedEat(0),
		fNumberDiedEdge(0),
		fNumberDiedSmite(0),
		fNumberDiedPatch(0),
		fNumberCreated(0),
		fNumberCreatedRandom(0),
		fNumberCreated1Fit(0),
		fNumberCreated2Fit(0),
		fNumberFights(0),
		fBirthDenials(0),
		fMiscDenials(0),
		fLastCreated(0),
		fMaxGapCreate(0),
		fNumBornSinceCreated(0),

		agentPovRenderer(NULL)
{
	fStep = 0;
	memset( fNumberAliveWithMetabolism, 0, sizeof(fNumberAliveWithMetabolism) );

	fCurrentBrainStats.sheets.synapseCount = new Stat *[ sheets::Sheet::__NTYPES ];
	for( int i = 0; i < sheets::Sheet::__NTYPES; i++ )
		fCurrentBrainStats.sheets.synapseCount[i] = new Stat[ sheets::Sheet::__NTYPES ];

    srand(1);

	// ---
	// --- Create the run directory
	// ---
	{
		char s[256];
		char t[256];

		// First save the old directory, if it exists
		sprintf( s, "run" );
		sprintf( t, "run_%ld", time(NULL) );
		(void) rename( s, t );

		if( mkdir("run", PwDirMode) )
		{
			eprintf( "Error making run directory (%d)\n", errno );
			exit( 1 );
		}
	}

	// ---
	// --- Process the Worldfile
	// ---
	proplib::SchemaDocument *schema;
	proplib::Document *worldfile;
	{
		proplib::DocumentBuilder builder;
		schema = builder.buildSchemaDocument( "./etc/worldfile.wfs" );
		worldfile = builder.buildWorldfileDocument( schema, worldfilePath, parameters );

		{
			ofstream out( "run/converted.wf" );
			proplib::DocumentWriter writer( out );
			writer.write( worldfile );
		}

		schema->apply( worldfile );
	}
	processWorldFile( worldfile );
	agent::processWorldfile( *worldfile );
	GenomeSchema::processWorldfile( *worldfile );
	Brain::processWorldfile( *worldfile );

	// If this is a lockstep run, then we need to force certain parameter values (and warn the user)
	if( fLockStepWithBirthsDeathsLog )
	{
		initLockstepMode();
	}
	// If this is a steady-state GA run, then we need to force certain parameter values (and warn the user)
	if( (fHeuristicFitnessWeight != 0.0) || (fComplexityFitnessWeight != 0) )
	{
		initFitnessMode();
	}

	// ---
	// --- General Init
	// ---
	Brain::init();
    agent::agentinit();
	SeparationCache::init();

	GenomeUtil::createSchema();

	// ---
	// --- Init Cpp Properties
	// ---
	InitCppProperties( worldfile );


	 // Following is part of one way to speed up the graphics
	 // Note:  this code must agree with the agent sizing in agent::grow()
	 // and the food sizing in food::initlen().

	float maxagentlenx = agent::config.maxAgentSize / sqrt(agent::config.minmaxspeed);
	float maxagentlenz = agent::config.maxAgentSize * sqrt(agent::config.maxmaxspeed);
	float maxagentradius = 0.5 * sqrt(maxagentlenx*maxagentlenx + maxagentlenz*maxagentlenz);
	float maxfoodlen = 0.75 * food::gMaxFoodEnergy / food::gSize2Energy;
	float maxfoodradius = 0.5 * sqrt(maxfoodlen * maxfoodlen * 2.0);
	food::gMaxFoodRadius = maxfoodradius;
	agent::config.maxRadius = maxagentradius > maxfoodradius ?
						  maxagentradius : maxfoodradius;

	InitFittest();

	if( fLockStepWithBirthsDeathsLog )
	{

		cout << "*** Running in LOCKSTEP MODE with file 'LOCKSTEP-BirthsDeaths.log' ***" << endl;

		if( (fLockstepFile = fopen("LOCKSTEP-BirthsDeaths.log", "r")) == NULL )
		{
			cerr << "ERROR/Init(): Could not open 'LOCKSTEP-BirthsDeaths.log' for reading. Exiting." << endl;
			exit(1);
		}

		char LockstepLine[512];
		int currentpos=0;			// current position in fLockstepFile.

		// bypass any header information
		while( (fgets(LockstepLine, sizeof(LockstepLine), fLockstepFile)) != NULL )
		{
			if( LockstepLine[0] == '#' || LockstepLine[0] == '%' ) { currentpos = ftell( fLockstepFile ); continue; 	}				// if the line begins with a '#' or '%' (implying it is a header line), skip it.
			else { fseek( fLockstepFile, currentpos, 0 ); break; }
		}

		if( feof(fLockstepFile) )	// this should never happen, but lets make sure.
		{
			cout << "ERROR/Init(): Did not find any data lines in 'LOCKSTEP-BirthsDeaths.log'.  Exiting." << endl;
			exit(1);
		}

		SYSTEM( "cp LOCKSTEP-BirthsDeaths.log run/" );		// copy the LOCKSTEP file into the run/ directory.
		SetNextLockstepEvent();								// setup for the first timestep in which Birth/Death events occurred.

	}

    // Pass ownership of the cast to the stage [TODO] figure out ownership issues
    fStage.SetCast(&fWorldCast);

	fFoodEnergyIn = 0.0;
	fFoodEnergyOut = 0.0;
	fEnergyEaten.zero();

	srand48(fGenomeSeed);

	agentPovRenderer = AgentPovRenderer::create( fMaxNumAgents,
                                                 Brain::config.retinaWidth,
                                                 Brain::config.retinaHeight );

	// ---
	// --- Init Logs
	// ---
	logs = new Logs( this, worldfile );

	// ---
	// --- Set Maximum Open Files
	// ---
	{
		int maxOpenFiles = 100; // just a fudge

		maxOpenFiles += logs->getMaxOpenFiles();

		// If we're going to be saving info on all these files, must increase the number allowed open
		if( SetMaximumFiles( maxOpenFiles ) )
		{
		    eprintf( "Error setting maximum files to %d (%d) -- consult ulimit\n", maxOpenFiles, errno );
		}
	}

	// ---
	// --- Init Ground
	// ---
	InitGround();

	// ---
	// --- Init Agents, Food, Bricks, and Barriers
	// ---
	if (!fLoadState)
	{
		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		// !!! EXEC MASTER
		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		fScheduler.execMasterTask( [=]() { InitAgents(); },
								   !fParallelInitAgents );

		InitFood();
		InitBricks();
		InitBarriers();
	}

	fEatStatistics.Init();

    fTotalFoodEnergyIn = fFoodEnergyIn;
    fTotalFoodEnergyOut = fFoodEnergyOut;
    fTotalEnergyEaten = fEnergyEaten;
    fAverageFoodEnergyIn = 0.0;
    fAverageFoodEnergyOut = 0.0;

    fStage.SetSet(&fWorldSet);

	// ---
	// --- Init Event Filtering
	// ---
	if( fCalcComplexity )
	{
		bool eventFiltering = false;
		for( unsigned int i = 0; i < fComplexityType.size(); i++ )
		{
			if( islower( fComplexityType[i] ) )
			{
				eventFiltering = true;
				break;
			}
		}
		if( eventFiltering )
			fEvents = new Events( fMaxSteps );
	}

	// ---
	// --- Save worldfile data to run/ and dispose documents
	// ---
	{
		SYSTEM( ("cp " + worldfile->getPath() + " run/original.wf").c_str() );
		SYSTEM( ("cp " + schema->getPath() + " run/original.wfs").c_str() );

		{
			ofstream out( "run/normalized.wf" );
			proplib::DocumentWriter writer( out );
			writer.write( worldfile );
		}

		delete worldfile;
		delete schema;
	}

#if DebugLockStep
	fdls = fopen( "LockStep.log", "w" );
	if( !fdls )
	{
		fprintf( stderr, "Unable to open LockStep.log\n" );
		exit( 1 );
	}
#endif

	logs->postEvent( SimInitedEvent() );
}


//---------------------------------------------------------------------------
// TSimulation::~TSimulation
//---------------------------------------------------------------------------
TSimulation::~TSimulation()
{
	for( int i = 0; i < sheets::Sheet::__NTYPES; i++ )
		delete [] fCurrentBrainStats.sheets.synapseCount[i];
	delete [] fCurrentBrainStats.sheets.synapseCount;

	agent *a;

	objectxsortedlist::gXSortedObjects.reset();
	while (objectxsortedlist::gXSortedObjects.nextObj(AGENTTYPE, (gobject**)&a))
	{
		Kill( a, LifeSpan::DR_SIMEND );
	}

	// ---
	// --- Dispose Logs
	// ---
	delete logs;

	if( fLockstepFile )
		fclose( fLockstepFile );

	{
		barrier* b;
		barrier::gXSortedBarriers.reset();
		while( barrier::gXSortedBarriers.next( b ) )
			delete b;
	}

	// delete all objects
	gobject* gob = NULL;

	// delete all non-agents
	objectxsortedlist::gXSortedObjects.reset();
	while (objectxsortedlist::gXSortedObjects.next(gob))
		if (gob->getType() != AGENTTYPE)
		{
			//delete gob;	// ??? why aren't these being deleted?  do we need to delete them by type?  make the destructor virtual?  what???
		}
	// delete all agents
	// all the agents are deleted in agentdestruct
	// rather than cycling through them in xsortedagents here
	objectxsortedlist::gXSortedObjects.clear();


	// TODO who owns items on stage?
	fStage.Clear();

	for (short id = 0; id < fNumDomains; id++)
	{
		if (fDomains[id].fittest)
			delete fDomains[id].fittest;

		if( fDomains[id].fLeastFit )
			delete[] fDomains[id].fLeastFit;
	}


	if (fFittest != NULL)
		delete fFittest;

	if( fRecentFittest != NULL )
		delete fRecentFittest;

	agent::agentdestruct();

	delete agentPovRenderer;

	printf( "Simulation stopped after step %ld\n", fStep );

	{
		ofstream fout( "run/endStep.txt" );
		fout << fStep << endl;
		fout.close();
	}
}


//---------------------------------------------------------------------------
// TSimulation::Step
//---------------------------------------------------------------------------
void TSimulation::Step()
{
#define RecentSteps 10
	static double	sTimePrevious[RecentSteps];
	static unsigned long frame = 0;
	double			timeNow;

	if( (frame == 0) && (fSimulationSeed != 0) )
	{
		srand48(fSimulationSeed);
	}

	frame++;
//	printf( "%s: frame = %lu\n", __FUNCTION__, frame );

	if( fMaxSteps && ((fStep+1) > fMaxSteps) )
	{
		End( "MaxSteps" );
		return;
	}
	else if( fEndOnPopulationCrash &&
			 (objectxsortedlist::gXSortedObjects.getCount(AGENTTYPE) <= fMinNumAgents) )
	{
		cerr << "Population crash at step " << fStep << endl;
		End( "PopulationCrash" );
		return;
	}

	fStep++;

	debugcheck( "beginning of step %ld", fStep );

	// compute some frame rates
	timeNow = hirestime();
	if( fStep == 1 )
	{
		fFramesPerSecondOverall = 0.;
		fSecondsPerFrameOverall = 0.;

		fFramesPerSecondRecent = 0.;
		fSecondsPerFrameRecent = 0.;

		fFramesPerSecondInstantaneous = 0.;
		fSecondsPerFrameInstantaneous = 0.;

		fTimeStart = timeNow;
	}
	else
	{
		fFramesPerSecondOverall = fStep / (timeNow - fTimeStart);
		fSecondsPerFrameOverall = 1. / fFramesPerSecondOverall;

		if( fStep > RecentSteps )
		{
			fFramesPerSecondRecent = RecentSteps / (timeNow - sTimePrevious[RecentSteps-1]);
			fSecondsPerFrameRecent = 1. / fFramesPerSecondRecent;
		}

		fFramesPerSecondInstantaneous = 1. / (timeNow - sTimePrevious[0]);
		fSecondsPerFrameInstantaneous = 1. / fFramesPerSecondInstantaneous;

		int numSteps = fStep < RecentSteps ? fStep : RecentSteps;
		for( int i = numSteps-1; i > 0; i-- )
			sTimePrevious[i] = sTimePrevious[i-1];
	}
	sTimePrevious[0] = timeNow;

	if (((fStep - fLastCreated) > fMaxGapCreate) && (fLastCreated > 0) )
		fMaxGapCreate = fStep - fLastCreated;

	if (fNumDomains > 1)
	{
		for (short id = 0; id < fNumDomains; id++)
		{
			if (((fStep - fDomains[id].lastcreate) > fDomains[id].maxgapcreate)
				  && (fDomains[id].lastcreate > 0))
			{
				fDomains[id].maxgapcreate = fStep - fDomains[id].lastcreate;
			}
		}
	}

	// Set up some per-step values
	fFoodEnergyIn = 0.0;
	fFoodEnergyOut = 0.0;
	fEnergyEaten.zero();

	// Update dynamic properties
	proplib::CppProperties::update();

	// Update the barriers, since they can be dynamic
	barrier* b;
	barrier::gXSortedBarriers.reset();
	while( barrier::gXSortedBarriers.next( b ) )
		b->update();
	barrier::gXSortedBarriers.xsort();

	MaintainEnergyCosts();

	// Update all agents, using their neurally controlled behaviors
	{
		agentPovRenderer->beginStep();

		if( fStaticTimestepGeometry )
		{
			UpdateAgents_StaticTimestepGeometry();
		}
		else
		{
			UpdateAgents();
		}

		// Swap buffers for the agent POV window when they're all done
		agentPovRenderer->endStep();
	}

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// !!! EXEC MASTER
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	fScheduler.execMasterTask( [=]() { Interact(); },
							   !fParallelInteract );

	assert( fNumberAlive == objectxsortedlist::gXSortedObjects.getCount(AGENTTYPE) );

	debugcheck( "after Interact() in step %ld", fStep );

	if( fNumAverageFitness > 0 )
		fAverageFitness /= fNumAverageFitness * fTotalHeuristicFitness;

#if DebugMaxFitness
	printf( "At age %ld (c,n,fit,c->fit) =", fStep );
	for( i = 0; i < fCurrentFittestCount; i++ )
		printf( " (%08lx,%ld,%5.2f,%5.2f)", (unsigned long) fCurrentFittestAgent[i], fCurrentFittestAgent[i]->Number(), fCurrentMaxFitness[i], fCurrentFittestAgent[i]->HeuristicFitness() );
	printf( "\n" );
#endif

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// !!! EXEC MASTER
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	fScheduler.execMasterTask( [=]() { CreateAgents(); },
							   !fParallelCreateAgents );

	// -------------------------
	// ---- Maintain Bricks ----
	// -------------------------
	// maintain bricks, which may be in dynamic patches...
	MaintainBricks();

	// -----------------------
	// ---- Maintain Food ----
	// -----------------------
	// finally, maintain the world's food supply...
	MaintainFood();

	fTotalFoodEnergyIn += fFoodEnergyIn;
	fTotalFoodEnergyOut += fFoodEnergyOut;
	fTotalEnergyEaten += fEnergyEaten;

	fAverageFoodEnergyIn = (float(fStep - 1) * fAverageFoodEnergyIn + fFoodEnergyIn) / float(fStep);
	fAverageFoodEnergyOut = (float(fStep - 1) * fAverageFoodEnergyOut + fFoodEnergyOut) / float(fStep);

	// ---------------------------------------------------
	// ---- Step Ending Signal (e.g. update monitors) ----
	// ---------------------------------------------------
    stepEnding();

	// ---------------
	// ---- Epoch ----
	// ---------------
	if( fEpochFrequency && ((fStep % fEpochFrequency) == 0) )
	{
		logs->postEvent( EpochEndEvent(fStep) );

		fEpoch += fEpochFrequency;

		fRecentFittest->clear();
	}

	logs->postEvent( StepEndEvent() );
}

//---------------------------------------------------------------------------
// TSimulation::End
//---------------------------------------------------------------------------
void TSimulation::End( const string &reason )
{
	{
		ofstream fout( "run/endReason.txt" );
		fout << reason << endl;
		fout.close();
	}

	ended();
}

//---------------------------------------------------------------------------
// TSimulation::EndAt
//
// Returns non-empty string on error.
//---------------------------------------------------------------------------
string TSimulation::EndAt( long timestep )
{
	if( timestep < fStep )
		return "Invalid end timestep. Simulation already beyond.";

	fMaxSteps = timestep;

	cout << "End At " << timestep << endl;

	return "";
}

//-------------------------------------------------------------------------------------------
// TSimulation::InitCppProperties()
//-------------------------------------------------------------------------------------------
void TSimulation::InitCppProperties( proplib::Document *docWorldFile )
{
	proplib::CppProperties::UpdateContext *context = new proplib::CppProperties::UpdateContext( this );
	proplib::CppProperties::init( docWorldFile, context );

}

//---------------------------------------------------------------------------
// TSimulation::InitFittest
//---------------------------------------------------------------------------
void TSimulation::InitFittest()
{
	if( fSmiteFrac > 0.0 )
	{
        for( int id = 0; id < fNumDomains; id++ )
        {
			fDomains[id].fNumLeastFit = 0;
			fDomains[id].fMaxNumLeastFit = lround( fSmiteFrac * fDomains[id].maxNumAgents );

			smPrint( "for domain %d fMaxNumLeastFit = %d\n", id, fDomains[id].fMaxNumLeastFit );

			if( fDomains[id].fMaxNumLeastFit > 0 )
			{
				fDomains[id].fLeastFit = new agent*[fDomains[id].fMaxNumLeastFit];

				for( int i = 0; i < fDomains[id].fMaxNumLeastFit; i++ )
					fDomains[id].fLeastFit[i] = NULL;
			}
        }
	}
}

//---------------------------------------------------------------------------
// TSimulation::InitGround
//---------------------------------------------------------------------------
void TSimulation::InitGround()
{
	Resources::loadPolygons( &fGround, "ground" );

    fGround.sety(-fGroundClearance);
    fGround.setscale(globals::worldsize);
    fGround.setcolor(fGroundColor);
    fWorldSet.Add(&fGround);
}

//---------------------------------------------------------------------------
// TSimulation::InitAgents
//---------------------------------------------------------------------------
void TSimulation::InitAgents()
{
	long numSeededDomain;
	long numSeededTotal = 0;
    agent* c = NULL;
	int id;

	// first handle the individual domains
	for (id = 0; id < fNumDomains; id++)
	{
		numSeededDomain = 0;	// reset for each domain

		int limit = min((fMaxNumAgents - (long)objectxsortedlist::gXSortedObjects.getCount(AGENTTYPE)), fDomains[id].initNumAgents);
		for (int i = 0; i < limit; i++)
		{
			bool isSeed = false;

			c = agent::getfreeagent(this, &fStage);
			assert(c != NULL);

			fNumberCreated++;
			fNumberCreatedRandom++;
			fDomains[id].numcreated++;

			if( numSeededDomain < fDomains[id].numberToSeed )
			{
				isSeed = true;

				SeedGenome( c->Number(),
							c->Genes(),
							fDomains[id].probabilityOfMutatingSeeds,
							numSeededDomain + numSeededTotal );
				numSeededDomain++;
			}
			else
				c->Genes()->randomize();

			c->setGenomeReady();

			// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
			// !!! POST PARALLEL
			// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            fScheduler.postParallel([=]() {
                        c->grow( fMateWait, true );
                });

			fStage.AddObject(c);

			float x, z;
			fDomains[id].initAgentsPatch->setPoint( &x, &z );
			float y = 0.5 * agent::config.agentHeight;
#if TestWorld
			// evenly distribute the agents
			x = fDomains[id].xleft  +  0.666 * fDomains[id].xsize;
			z = - globals::worldsize * ((float) (i+1) / (fDomains[id].initNumAgents + 1));
#endif
			if( isSeed )
			{
				SetSeedPosition( c, numSeededDomain + numSeededTotal - 1, x, y, z );
			}
			else
			{
				c->settranslation(x, y, z);
			}
			c->SaveLastPosition();

			float yaw =  360.0 * randpw();
#if TestWorld
			// point them all the same way
			yaw = 95.0;
#endif
			c->setyaw(yaw);

			objectxsortedlist::gXSortedObjects.add(c);	// stores c->listLink

			c->Domain(id);
			fDomains[id].numAgents++;

			// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
			// !!! POST SERIAL
			// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            fScheduler.postSerial( [=]() {
                    FoodEnergyIn( c->GetFoodEnergy() );
                });

			Birth( c, LifeSpan::BR_SIMINIT );
		}

		numSeededTotal += numSeededDomain;
	}

	// Handle global initial creations, if necessary
	assert( fInitNumAgents <= fMaxNumAgents );

	while( (int)objectxsortedlist::gXSortedObjects.getCount(AGENTTYPE) < fInitNumAgents )
	{
		bool isSeed = true;

		c = agent::getfreeagent( this, &fStage );

		fNumberCreated++;
		fNumberCreatedRandom++;

		if( numSeededTotal < fNumberToSeed )
		{
			isSeed = true;

			SeedGenome( c->Number(),
						c->Genes(),
						fProbabilityOfMutatingSeeds,
						numSeededTotal );
			numSeededTotal++;
		}
		else
			c->Genes()->randomize();

		c->setGenomeReady();

		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		// !!! POST PARALLEL
		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        fScheduler.postParallel( [=]() {
                c->grow( fMateWait, true );
            });

		fStage.AddObject(c);

		float x =  0.01 + randpw() * (globals::worldsize - 0.02);
		float z = -0.01 - randpw() * (globals::worldsize - 0.02);
		float y = 0.5 * agent::config.agentHeight;
		if( isSeed )
		{
			SetSeedPosition( c, numSeededTotal - 1, x, y, z );
		}
		else
		{
			c->settranslation(x, y, z);
		}

		float yaw =  360.0 * randpw();
		c->setyaw(yaw);

		objectxsortedlist::gXSortedObjects.add(c);	// stores c->listLink

		id = WhichDomain(x, z, 0);
		c->Domain(id);
		fDomains[id].numAgents++;

		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		// !!! POST SERIAL
		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        fScheduler.postSerial( [=]() {
                FoodEnergyIn( c->GetFoodEnergy() );
            });

		Birth(c, LifeSpan::BR_SIMINIT );
	}
}

//---------------------------------------------------------------------------
// TSimulation::InitFood
//---------------------------------------------------------------------------
void TSimulation::InitFood()
{
	// Add food to the food patches until they each have their initFoodCount number of food pieces
	for( int domainNumber = 0; domainNumber < fNumDomains; domainNumber++ )
	{
		fDomains[domainNumber].numFoodPatchesGrown = 0;

		for( int foodPatchNumber = 0; foodPatchNumber < fDomains[domainNumber].numFoodPatches; foodPatchNumber++ )
		{
			if( fDomains[domainNumber].fFoodPatches[foodPatchNumber].isOn() )
			{
				for( int j = 0; j < fDomains[domainNumber].fFoodPatches[foodPatchNumber].initFoodCount; j++ )
				{
					if( fDomains[domainNumber].foodCount < fDomains[domainNumber].maxFoodCount )
					{
						AddFood( domainNumber, foodPatchNumber );
					}
				}
				fDomains[domainNumber].fFoodPatches[foodPatchNumber].initFoodGrown( true );
				fDomains[domainNumber].numFoodPatchesGrown++;
			}
		}
	}
}

//---------------------------------------------------------------------------
// TSimulation::InitBricks
//---------------------------------------------------------------------------
void TSimulation::InitBricks()
{
	for( int domainNumber = 0; domainNumber < fNumDomains; domainNumber++ )
	{
		for( int brickPatchNumber = 0; brickPatchNumber < fDomains[domainNumber].numBrickPatches; brickPatchNumber++ )
		{
			fDomains[domainNumber].fBrickPatches[brickPatchNumber].updateOn();
		}
	}

}

//---------------------------------------------------------------------------
// TSimulation::InitBarriers
//---------------------------------------------------------------------------
void TSimulation::InitBarriers()
{
	// Add barriers
	barrier* b = NULL;
	barrier::gXSortedBarriers.reset();
	while( barrier::gXSortedBarriers.next(b) )
		fWorldSet.Add(b);
}

//---------------------------------------------------------------------------
// TSimulation::SeedGenome
//---------------------------------------------------------------------------

void TSimulation::SeedGenome( long agentNumber,
							  genome::Genome *genes,
							  float probabilityOfMutatingSeeds,
							  long numSeeded )
{
	if( fSeedFromFile )
	{
		SeedGenomeFromFile( agentNumber, genes, numSeeded );
	}
	else
	{
		GenomeUtil::seed( genes );
	}
	if( randpw() < probabilityOfMutatingSeeds )
	{
		genes->mutate();
	}
}

//---------------------------------------------------------------------------
// TSimulation::SeedGenomeFromFile
//---------------------------------------------------------------------------

void TSimulation::SeedGenomeFromFile( long agentNumber,
									  genome::Genome *genes,
									  long numSeeded )
{
	if( fSeedFilePaths.size() == 0 )
	{
		ReadSeedFilePaths();
	}

	const string &path = fSeedFilePaths[ numSeeded % fSeedFilePaths.size() ];
	AbstractFile *in = AbstractFile::open( path.c_str(), "r" );
	if( in == NULL )
	{
		cerr << "Could not open seed file " << path << endl;
		exit( 1 );
	}

	cout << "seeding agent #" << agentNumber << " genome from " << path << endl;

	genes->load( in );

	delete in;
}

//---------------------------------------------------------------------------
// TSimulation::ReadSeedFilePaths
//---------------------------------------------------------------------------

void TSimulation::ReadSeedFilePaths()
{
	ifstream in("genomeSeeds.txt");

	if( in.fail() )
	{
		cerr << "Could not open genomeSeeds.txt" << endl;
		exit( 1 );
	}

	SYSTEM( "cp genomeSeeds.txt run/genome" );

	char buf[1024 * 4];
	while( !in.eof() )
	{
		in.getline( buf, sizeof(buf) );

		if( strlen(buf) )
		{
			fSeedFilePaths.push_back( string(buf) );
		}
	}

	if( fSeedFilePaths.size() == 0 )
	{
		cerr << "genomeSeeds.txt is empty!" << endl;
		exit( 1 );
	}
}

//---------------------------------------------------------------------------
// TSimulation::SetSeedPosition
//---------------------------------------------------------------------------

void TSimulation::SetSeedPosition( agent *a, long numSeeded, float x, float y, float z )
{
	if( fPositionSeedsFromFile )
	{
		if( fSeedPositions.size() == 0 )
		{
			ReadSeedPositionsFromFile();
		}

		if( numSeeded < (long)fSeedPositions.size() )
		{
			Position &pos = fSeedPositions[numSeeded];
			x = pos.x;
			if( pos.y >= 0 ) // negative value means disregard
				y = pos.y;
			z = pos.z;
		}
	}

	a->settranslation(x, y, z);
}

//---------------------------------------------------------------------------
// TSimulation::ReadSeedPositionsFromFile
//---------------------------------------------------------------------------

void TSimulation::ReadSeedPositionsFromFile()
{
	ifstream in("seedPositions.txt");

	if( in.fail() )
	{
		cerr << "Could not open seedPositions.txt" << endl;
		exit( 1 );
	}

	SYSTEM( "mkdir -p run/motion/position && cp seedPositions.txt run/motion/position" );

	while( !in.eof() )
	{
		Position pos;

		struct local
		{
			static float read_dim( istream &in )
			{
				float val;
				string raw;
				in >> raw;

				if( raw[0] == 'r' )
				{
					string ratiostr = raw.substr(1);
					float ratio = atof( ratiostr.c_str() );
					val = ratio * globals::worldsize;
				}
				else
				{
					val = atof( raw.c_str() );
				}

				return val;
			}
		};

		pos.x = local::read_dim( in );
		pos.y = local::read_dim( in );
		pos.z = local::read_dim( in );

		fSeedPositions.push_back( pos );
	}

	if( fSeedPositions.size() == 0 )
	{
		cerr << "seedPositions.txt is empty!" << endl;
		exit( 1 );
	}
}

//---------------------------------------------------------------------------
// TSimulation::PickParentsUsingTournament
//---------------------------------------------------------------------------

void TSimulation::PickParentsUsingTournament(int numInPool, int* iParent, int* jParent)
{
	*iParent = numInPool-1;
	for (int z = 0; z < fTournamentSize; z++)
	{
		int r = (int)floor(randpw()*numInPool);
		if (*iParent > r)
			*iParent = r;
	}
	do
	{
		*jParent = numInPool-1;
		for (int z = 0; z < fTournamentSize; z++)
		{
			int r = (int)floor(randpw()*numInPool);
			if (*jParent > r)
				*jParent = r;
		}
	} while (*jParent == *iParent);
}


//---------------------------------------------------------------------------
// TSimulation::EnergyScaleFactor
//---------------------------------------------------------------------------
double TSimulation::EnergyScaleFactor( long minAgents, long maxAgents, long numAgents )
{
	double scaleFactor = 1.0;

	long topFixedRange = minAgents + lround( fPopControlMaxFixedRange * (maxAgents - minAgents) );
	long botFixedRange = minAgents + lround( fPopControlMinFixedRange * (maxAgents - minAgents) );

	if( numAgents < botFixedRange )
	{
		double fraction = (botFixedRange - numAgents) / (float) (botFixedRange - minAgents);
		scaleFactor = 1.0 - (1.0 - fPopControlMinScaleFactor) * fraction;
		if( scaleFactor < 0.0 )
			scaleFactor = 0.0;
	}
	else if( numAgents > topFixedRange )
	{
		double fraction = (numAgents - topFixedRange) / (float) (maxAgents - topFixedRange);
		double fractionReduced = pow( fraction, 4.0 );
		scaleFactor = 1.0 + (fPopControlMaxScaleFactor - 1.0) * fractionReduced;
		// don't need to cap, the way we did with 0.0,
		// because it'll be close enough and not suffer a sign change
		//printf( "%ld: a=%ld gsf=%g f=%g fr=%g\n", fStep, numAgents, scaleFactor, fraction, fractionReduced );
	}

	return( scaleFactor );
}


//---------------------------------------------------------------------------
// TSimulation::MaintainEnergyCosts
//---------------------------------------------------------------------------
void TSimulation::MaintainEnergyCosts()
{
	if( fEnergyBasedPopulationControl )
	{
		if( fPopControlGlobal )
		{
			long numAgents = objectxsortedlist::gXSortedObjects.getCount(AGENTTYPE);
			fGlobalEnergyScaleFactor = EnergyScaleFactor( fMinNumAgents, fMaxNumAgents, numAgents );
			//printf( "%ld: a=%ld gsf=%g\n", fStep, numAgents, fGlobalEnergyScaleFactor );
		}

		if( fPopControlDomains )
		{
			// We make the assumption that if there is only one domain
			// it occupies the full global space, so we only want one
			// of the domain-specific or global population controls.
			// If there are multiple domains, then we assume none
			// of them is identical to the global domain, so we use
			// both, if the user requested both.
			if( fNumDomains > 1  ||  ! fPopControlGlobal )
			{
				for( int i = 0; i < fNumDomains; i++ )
					fDomains[i].energyScaleFactor = EnergyScaleFactor( fDomains[i].minNumAgents,
																	   fDomains[i].maxNumAgents,
																	   fDomains[i].numAgents );
			}
		}
	}
	else if( fApplyLowPopulationAdvantage || fNumDepletionSteps )
	{
		// These are the agent counts to be used in applying either the LowPopulationAdvantage or the (high) PopulationPenalty
		// Assume global settings apply, until we know better
		long numAgents = objectxsortedlist::gXSortedObjects.getCount(AGENTTYPE);
		long initNumAgents = fInitNumAgents;
		long minNumAgents = fMinNumAgents  +  lround( 0.1 * (fInitNumAgents - fMinNumAgents) );	// 10% buffer, to help prevent reaching actual min value and invoking GA
		long maxNumAgents = fMaxNumAgents;
		long excess = numAgents - fInitNumAgents;	// global excess

		// Use the lowest excess value to produce the most help or the least penalty
		if( fNumDomains > 1 )
		{
			for( int id = 0; id < fNumDomains; id++ )
			{
				long domainExcess = fDomains[id].numAgents - fDomains[id].initNumAgents;
				if( domainExcess < excess )	// This is the domain that is in the worst shape
				{
					numAgents = fDomains[id].numAgents;
					initNumAgents = fDomains[id].initNumAgents;
					minNumAgents = fDomains[id].minNumAgents  +  lround( 0.1 * (fDomains[id].initNumAgents - fDomains[id].minNumAgents) );	// 10% buffer, to help prevent reaching actual min value and invoking GA
					maxNumAgents = fDomains[id].maxNumAgents;
					excess = domainExcess;
				}
			}
		}

		// If the population is too low, globally or in any domain, then either help it or leave it alone
		if( excess < 0 )
		{
			fPopulationPenaltyFraction = 0.0;	// we're not currently applying the high population penalty

			// If we want to help it, apply the low population advantage
			if( fApplyLowPopulationAdvantage )
			{
				fLowPopulationAdvantageFactor = 1.0 - (float) (initNumAgents - numAgents) / (initNumAgents - minNumAgents);
				if( fLowPopulationAdvantageFactor < 0.0 )
					fLowPopulationAdvantageFactor = 0.0;
				if( fLowPopulationAdvantageFactor > 1.0 )
					fLowPopulationAdvantageFactor = 1.0;
			}
		}
		else if( excess > 0 )	// population is greater than initial level everywhere
		{
			fLowPopulationAdvantageFactor = 1.0;	// we're not currently applying the low population advantage

			// apply the high population penalty (if the max-penalty is non-zero)
			fPopulationPenaltyFraction = fMaxPopulationPenaltyFraction * (numAgents - initNumAgents) / (maxNumAgents - initNumAgents);
			if( fPopulationPenaltyFraction < 0.0 )
				fPopulationPenaltyFraction = 0.0;
			if( fPopulationPenaltyFraction > fMaxPopulationPenaltyFraction )
				fPopulationPenaltyFraction = fMaxPopulationPenaltyFraction;
		}

		//printf( "step=%4ld, pop=%3d, initPop=%3ld, minPop=%2ld, maxPop=%3ld, maxPopPenaltyFraction=%g, popPenaltyFraction=%g, lowPopAdvantageFactor=%g\n",
		//		fStep, numAgents, initNumAgents, minNumAgents, maxNumAgents,
		//		fMaxPopulationPenaltyFraction, fPopulationPenaltyFraction, fLowPopulationAdvantageFactor );
	}
}


//---------------------------------------------------------------------------
// TSimulation::UpdateAgents
//---------------------------------------------------------------------------
void TSimulation::UpdateAgents()
{
	// In this version of UpdateAgents*() the world changes as each agent is
	// processed, so we can't do our stage compilation or parallelization
	// optimizations

#if DEBUGCHECK
	int pass = 0;
#endif
	agent* a;
	objectxsortedlist::gXSortedObjects.reset();
	while (objectxsortedlist::gXSortedObjects.nextObj(AGENTTYPE, (gobject**)&a))
	{
	#if DEBUGCHECK
		debugcheck( "in agent loop at age %ld, pass = %d, agent = %lu", fStep, pass, a->Number() );
		pass++;
	#endif

		a->UpdateVision();
		a->UpdateBrain();
		if( !a->BeingCarried() )
			fFoodEnergyOut += a->UpdateBody(fMoveFitnessParameter,
											agent::config.speed2DPosition,
											fSolidObjects,
											NULL);
	}
}


//---------------------------------------------------------------------------
// TSimulation::UpdateAgents_StaticTimestepGeometry
//---------------------------------------------------------------------------
void TSimulation::UpdateAgents_StaticTimestepGeometry()
{
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    // !!! EXEC MASTER
    // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    fScheduler.execMasterTask([=]() {
            fStage.Compile();
            objectxsortedlist::gXSortedObjects.reset();

            agent *a = NULL;
            while (objectxsortedlist::gXSortedObjects.nextObj(AGENTTYPE, (gobject**)&a))
            {
                // ---
                // --- Update POV (3D rendering... expensive)
                // ---
                a->UpdateVision();

                fScheduler.postParallel([=]() {
                        // ---
                        // --- Execute Neural Net
                        // ---
                        a->UpdateBrain();
                    });
            }

            fStage.Decompile();
        },
        !fParallelBrains);

	// ---
	// --- Body
	// ---
	{
		agent *a;

		objectxsortedlist::gXSortedObjects.reset();
		while( objectxsortedlist::gXSortedObjects.nextObj( AGENTTYPE, (gobject**)&a) )
		{
			if( !a->BeingCarried() )
				fFoodEnergyOut += a->UpdateBody( fMoveFitnessParameter,
												 agent::config.speed2DPosition,
												 fSolidObjects,
												 NULL );
		}
	}
}


//---------------------------------------------------------------------------
// TSimulation::Interact
//---------------------------------------------------------------------------
void TSimulation::Interact()
{
    agent* c = NULL;
    agent* d = NULL;
	long i;
	bool cDied;

	fNewLifes = 0;
	fNewDeaths = 0;

	fEatStatistics.StepBegin();

	// first x-sort all the objects
	objectxsortedlist::gXSortedObjects.sort();

#if DebugShowSort
	if( fStep == 1 )
		printf( "food::gMaxFoodRadius = %g\n", food::gMaxFoodRadius );
	if( (fStep >= MinDebugStep) && (fStep <= MaxDebugStep) )
	{
		food* f;
		objectxsortedlist::gXSortedObjects.reset();
		printf( "********** agents at step %ld **********\n", fStep );
		while( objectxsortedlist::gXSortedObjects.nextObj( AGENTTYPE, (gobject**) &c ) )
		{
			printf( "  # %ld edge=%g at (%g,%g) rad=%g\n", c->Number(), c->x() - c->radius(), c->x(), c->z(), c->radius() );
			fflush( stdout );
		}
		objectxsortedlist::gXSortedObjects.reset();
		printf( "********** food at step %ld **********\n", fStep );
		while( objectxsortedlist::gXSortedObjects.nextObj( FOODTYPE, (gobject**) &f ) )
		{
			printf( "  edge=%g at (%g,%g) rad=%g\n", f->x() - f->radius(), f->x(), f->z(), f->radius() );
			fflush( stdout );
		}
	}
#endif

	fCurrentFittestCount = 0;
	smPrint( "setting fCurrentFittestCount to 0\n" );
	fPrevAvgFitness = fAverageFitness; // used in smite code, to limit agents that are smitable
    fAverageFitness = 0.0;
	fNumAverageFitness = 0;	// need this because we'll only count agents that have lived at least a modest portion of their lifespan (fSmiteAgeFrac)
//	fNumLeastFit = 0;
//	fNumSmited = 0;
	for( i = 0; i < fNumDomains; i++ )
	{
		fDomains[i].fNumLeastFit = 0;
		fDomains[i].fNumSmited = 0;
	}

	// -----------------------
	// -------- Death --------
	// -----------------------
	// Take care of deaths first, plus least-fit determinations
	// Also use this as a convenient place to compute some stats
	DeathAndStats();

#if DebugSmite
	if( (fStep >= MinDebugStep) && (fStep <= MaxDebugStep) )
	{
		for( short id = 0; id < fNumDomains; id++ )
		{
			printf( "At age %ld in domain %d (c,n,c->fit) =", fStep, id );
			for( i = 0; i < fDomains[id].fNumLeastFit; i++ )
				printf( " (%08lx,%ld,%5.2f)", (unsigned long) fDomains[id].fLeastFit[i], fDomains[id].fLeastFit[i]->Number(), fDomains[id].fLeastFit[i]->HeuristicFitness() );
			printf( "\n" );
		}
	}
#endif

#if DebugMaxFitness
	if( (fStep >= MinDebugStep) && (fStep <= MaxDebugStep) )
	{
		objectxsortedlist::gXSortedObjects.reset();
		objectxsortedlist::gXSortedObjects.nextObj(AGENTTYPE, c);
		agent* lastAgent;
		objectxsortedlist::gXSortedObjects.lastObj(AGENTTYPE, (gobject**) &lastAgent );
		printf( "%s: at age %ld about to process %ld agents, %ld pieces of food, starting with agent %08lx (%4ld), ending with agent %08lx (%4ld)\n", __FUNCTION__, fStep, objectxsortedlist::gXSortedObjects.getCount(AGENTTYPE), objectxsortedlist::gXSortedObjects.getCount(FOODTYPE), (unsigned long) c, c->Number(), (unsigned long) lastAgent, lastAgent->Number() );
	}
#endif

	// -----------------------
	// -------- Heal ---------
	// -----------------------
	if( fHealing )	// if healing is turned on...
	{
		objectxsortedlist::gXSortedObjects.reset();
    	while( objectxsortedlist::gXSortedObjects.nextObj( AGENTTYPE, (gobject**) &c) )	// for every agent...
			c->Heal( fAgentHealingRate, 0.0 );										// heal it if FoodEnergy > 2ndParam.
	}

	// -----------------------
	// --- Mate (Lockstep) ---
	//------------------------
	if( fLockStepWithBirthsDeathsLog )
	{
		if( fLockstepTimestep == fStep )
		{
			MateLockstep();
			SetNextLockstepEvent();		// set the timestep, NumBirths, and NumDeaths for the next Lockstep events.
		}
	}

	// Now go through the list, and use the influence radius to determine
	// all possible interactions

	objectxsortedlist::gXSortedObjects.reset();
    while( objectxsortedlist::gXSortedObjects.nextObj( AGENTTYPE, (gobject**) &c ) )
    {
		// Check for new agent.  If totally new (never updated), skip this agent.
		// This is because newly born agents get added directly to the main list,
		// and we might try to process them immediately after being born, but we
		// don't want to do that until the next time step.
		if( c->Age() <= 0 )
			continue;

		objectxsortedlist::gXSortedObjects.setMark( AGENTTYPE ); // so can point back to this agent later
        cDied = false;

		// See if there's an overlap with any other agents
        while( objectxsortedlist::gXSortedObjects.nextObj( AGENTTYPE, (gobject**) &d ) ) // to end of list or...
        {
			if( d == c )	// sanity check; shouldn't happen
			{
				printf( "***************** d == c **************\n" );
				continue;
			}

            if( (d->x() - d->radius()) >= (c->x() + c->radius()) )
                break;  // this guy (& everybody else in list) is too far away

            // so if we get here, then c & d are close enough in x to interact

			// We used to test only on delta z at this point, thereby using manhattan distance to permit interaction
			// now modified to use actual distances to tighten things up a little (particularly visible in "toy world"
			// simulations).  Since we are basing interactions on circumscribing circles, agents may still interact
			// without having an actual overlap of polygons, but using actual distances reduces the range over which
			// this may happen and should reduce the number of such incidents.
			if( sqrt( (d->x()-c->x())*(d->x()-c->x()) + (d->z()-c->z())*(d->z()-c->z()) ) <= (d->radius() + c->radius()) )
            {
                // and if we get here then they are also close enough in z,
                // so must actually worry about their interaction

				ttPrint( "age %ld: agents # %ld & %ld are close\n", fStep, c->Number(), d->Number() );

				AgentContactBeginEvent contactEvent( c, d );

				logs->postEvent( contactEvent );

				// -----------------------
				// ---- Mate (Normal) ----
				// -----------------------
                Mate( c, d, &contactEvent );

				// -----------------------
				// -------- Fight --------
				// -----------------------
				bool dDied = false;
                if (fPower2Energy > 0.0)
                {
					Fight( c, d, &contactEvent, &cDied, &dDied );
                }

				// -----------------------
				// -------- Give ---------
				// -----------------------
				if( agent::config.enableGive )
				{
					if( !cDied && !dDied )
					{
						Give( c, d, &contactEvent, &cDied, true );
						if(!cDied)
						{
							Give( d, c, &contactEvent, &dDied, false );
						}
					}
				}

				logs->postEvent( AgentContactEndEvent(contactEvent) );

				if( cDied )
					break;

            }  // if close enough
        }  // while (agent::config.xSortedAgents.next(d))

        debugcheck( "after all agent interactions" );

        if( cDied )
			continue; // nothing else to do with c, it's gone!

		// -----------------------
		// --------- Eat ---------
		// -----------------------
		// They finally get to eat (couldn't earlier to keep from conferring
		// a special advantage on agents early in the sorted list)
		Eat( c, &cDied );

		// It ate poison :-(
		if( cDied )
			continue;

		// -----------------------
		// -------- Carry --------
		// -----------------------
		// Have to do carry testing here instead of inside inner loop above,
		// because agents can carry any kind of object, not just other agents
		if( agent::config.enableCarry )
			Carry( c );

		// -----------------------
		// ------- Fitness -------
		// -----------------------
		// keep tabs of current and average fitness for surviving organisms
		Fitness( c );

    } // while loop on agents (c)

	fEatStatistics.StepEnd();

// 	if( fFittest->size() > 0 )
// 	{
// 		printf( "Step %ld fFittest list...\n", fStep );
// 		for( int i = 0; i < fFittest->size(); i++ )
// 		{
// 			FitStruct* fit = fFittest->get(i);
// 			printf( "  %d: a=%ld f=%5.2f c=%5.2f\n", i, fit->agentID, fit->fitness, fit->complexity );
//
// 		}
// 	}
}


//---------------------------------------------------------------------------
// TSimulation::DeathAndStats
//---------------------------------------------------------------------------
void TSimulation::DeathAndStats( void )
{
	agent *c;
	short id;

	// reset all the FoodPatch agent counts to 0
	if( fCalcFoodPatchAgentCounts )
	{
		for( int domainNumber = 0; domainNumber < fNumDomains; domainNumber++ )
		{
			for( int foodPatchNumber = 0; foodPatchNumber < fDomains[domainNumber].numFoodPatches; foodPatchNumber++ )
			{
				fDomains[domainNumber].fFoodPatches[foodPatchNumber].resetAgentCounts();
			}
		}
	}

	if( fLockStepWithBirthsDeathsLog )
	{
		// if we are running in Lockstep with a LOCKSTEP-BirthDeaths.log, we kill our agents here.
		if( fLockstepTimestep == fStep )
		{
			lsPrint( "t%ld: Triggering %d random deaths...\n", fStep, fLockstepNumDeathsAtTimestep );

			for( int count = 0; count < fLockstepNumDeathsAtTimestep; count++ )
			{
				int i = 0;
				int numagents = objectxsortedlist::gXSortedObjects.getCount( AGENTTYPE );
				agent* testAgent;
				agent* randAgent = NULL;
	//				int randomIndex = int( floor( randpw() * fDomains[kd].numagents ) );	// pick from this domain
				int randomIndex = int( floor( randpw() * numagents ) );
				gdlink<gobject*> *saveCurr = objectxsortedlist::gXSortedObjects.getcurr();	// save the state of the x-sorted list

				// As written, randAgent may not actually be the randomIndex-th agent in the domain, but it will be close,
				// and as long as there's a single legitimate agent for killing, we will find and kill it
				objectxsortedlist::gXSortedObjects.reset();
				while( objectxsortedlist::gXSortedObjects.nextObj( AGENTTYPE, (gobject**) &testAgent ) )
				{
					// no qualifications for this agent.  It doesn't even need to be old enough to smite.
					randAgent = testAgent;	// as long as there's a single legitimate agent for killing, randAgent will be non-NULL

					i++;

					if( i > randomIndex )	// don't need to test for non-NULL randAgent, as it is always non-NULL by the time we reach here
						break;
				}

				objectxsortedlist::gXSortedObjects.setcurr( saveCurr );	// restore the state of the x-sorted list  V???

				assert( randAgent != NULL );		// In we're in LOCKSTEP mode, we should *always* have a agent to kill.  If we don't kill a agent, then we are no longer in sync in the LOCKSTEP-BirthsDeaths.log

				Kill( randAgent, LifeSpan::DR_LOCKSTEP );

				lsPrint( "- Killed agent %ld, randomIndex = %d\n", randAgent->Number(), randomIndex );
			}	// end of for loop
		}	// end of if( fLockstepTimestep == fStep )
	}

	switch( Brain::config.architecture )
	{
	case Brain::Configuration::Groups:
		fCurrentBrainStats.groups.groupCount.reset();
		break;
	case Brain::Configuration::Sheets:
		fCurrentBrainStats.sheets.internalSheetCount.reset();
		fCurrentBrainStats.sheets.internalNeuronCount.reset();
		for( SheetSynapseType &type : SheetSynapseTypes )
			fCurrentBrainStats.sheets.synapseCount[ type.from ][ type.to ].reset();
		break;
	default:
		assert( false );
	}
	fCurrentBrainStats.neuronCount.reset();
	fCurrentBrainStats.synapseCount.reset();
	objectxsortedlist::gXSortedObjects.reset();
    while( objectxsortedlist::gXSortedObjects.nextObj( AGENTTYPE, (gobject**) &c ) )
    {
		switch( Brain::config.architecture )
		{
		case Brain::Configuration::Groups:
			{
				GroupsBrain *brain = dynamic_cast<GroupsBrain *>( c->GetBrain() );
				fCurrentBrainStats.groups.groupCount.add( brain->NumNeuronGroups() );
			}
			break;
		case Brain::Configuration::Sheets:
			{
				SheetsBrain *brain = dynamic_cast<SheetsBrain *>( c->GetBrain() );
				fCurrentBrainStats.sheets.internalSheetCount.add( brain->getNumInternalSheets() );
				fCurrentBrainStats.sheets.internalNeuronCount.add( brain->getNumInternalNeurons() );
				for( SheetSynapseType &type : SheetSynapseTypes )
					fCurrentBrainStats.sheets.synapseCount[type.from][type.to].add( brain->getNumSynapses(type.from, type.to) );
			}
			break;
		default:
			assert( false );
		}
		fCurrentBrainStats.neuronCount.add( c->GetBrain()->getNumNeurons() );
		fCurrentBrainStats.synapseCount.add( c->GetBrain()->getNumSynapses() );

        id = c->Domain();						// Determine the domain in which the agent currently is located

		if( ! fLockStepWithBirthsDeathsLog )
		{
			// If we're not running in LockStep mode, allow natural deaths
			// If we're not using either of the energy-based population controls
			// to prevent the population getting too low, or there are enough agents
			// that we can still afford to lose one (globally & in agent's domain)...
			if( (!fApplyLowPopulationAdvantage && !fEnergyBasedPopulationControl) ||
				((objectxsortedlist::gXSortedObjects.getCount( AGENTTYPE ) > fMinNumAgents)
				 && (fNumberAliveWithMetabolism[c->GetMetabolism()->index] > fMinNumAgentsWithMetabolism[c->GetMetabolism()->index])
				 && (fDomains[c->Domain()].numAgents > fDomains[c->Domain()].minNumAgents)) ||
				(fAllowMinDeaths && (randpw() > float(fNumberBorn)/float(fNumberCreated + fNumberBorn))) )

			{
				if ( c->GetEnergy().isDepleted() ||
					 ( fDieAtMaxAge && c->Age() >= c->MaxAge() ) ||
					 ( !globals::blockedEdges && !globals::wraparound &&
					 	(c->x() < 0.0 || c->x() >  globals::worldsize ||
						 c->z() > 0.0 || c->z() < -globals::worldsize) ) ||
					 c->GetDeathByPatch() )
				{
					LifeSpan::DeathReason reason = LifeSpan::DR_NATURAL;

					if (fDieAtMaxAge && c->Age() >= c->MaxAge())
						fNumberDiedAge++;
					else if ( c->GetEnergy().isDepleted() )
						fNumberDiedEnergy++;
					else if (c->GetDeathByPatch())
					{
						fNumberDiedPatch++;
						reason = LifeSpan::DR_PATCH;
					}
					else
						fNumberDiedEdge++;
					Kill( c, reason );
					continue; // nothing else to do for this poor schmo
				}
			}
		}

	#ifdef OF1
        if ( (id == 0) && (randpw() < fDeathProb) )
        {
            Kill( c, LifeSpan::DR_RANDOM );
            continue;
        }
	#endif

        debugcheck( "after a death" );

		// If we're saving agent-occupancy-of-food-band stats, compute them here
		if( fCalcFoodPatchAgentCounts )
		{
			// Count agents inside FoodPatches
			// Also: Count agents outside FoodPatches, but within fFoodPatchOuterRange
			for (int domainNumber = 0; domainNumber < fNumDomains; domainNumber++){
				for( int foodPatchNumber = 0; foodPatchNumber < fDomains[domainNumber].numFoodPatches; foodPatchNumber++ )
				{
					// if agent is inside, then update FoodPatch's agentInsideCount
					fDomains[domainNumber].fFoodPatches[foodPatchNumber].checkIfAgentIsInside(c->x(), c->z());

					// if agent is inside the outerrange, then update FoodPatch's agentOuterRangeCount
					fDomains[domainNumber].fFoodPatches[foodPatchNumber].checkIfAgentIsInsideNeighborhood(c->x(), c->z());
				}
			}
		}

		// Figure out who is least fit, if we're doing smiting to make room for births

		// Do the bookkeeping for the specific domain, if we're using domains
		// Note: I think we must have at least one domain these days - lsy 6/1/05

		// The test against average fitness is an attempt to keep fit organisms from being smited, in general,
		// but it also helps protect against the situation when there are so few potential low-fitness candidates,
		// due to the age constraint and/or population size, that agents can end up on both the highest fitness
		// and the lowest fitness lists, which can actually cause a crash (or at least used to).
		if( (fNumDomains > 0) && (fDomains[id].fMaxNumLeastFit > 0) )
		{
			if( ((fDomains[id].numAgents > (fDomains[id].maxNumAgents - fDomains[id].fMaxNumLeastFit))) &&	// if there are getting to be too many agents, and
				(c->Age() >= (fSmiteAgeFrac * c->MaxAge())) &&													// the current agent is old enough to consider for smiting, and
				(c->HeuristicFitness() < fPrevAvgFitness) &&																// the current agent has worse than average fitness,
				( (fDomains[id].fNumLeastFit < fDomains[id].fMaxNumLeastFit)	||								// (we haven't filled our quota yet, or
				  (c->HeuristicFitness() < fDomains[id].fLeastFit[fDomains[id].fNumLeastFit-1]->HeuristicFitness()) ) )			// the agent is bad enough to displace one already in the queue)
			{
				if( fDomains[id].fNumLeastFit == 0 )
				{
					// It's the first one, so just store it
					fDomains[id].fLeastFit[0] = c;
					fDomains[id].fNumLeastFit++;
					smPrint( "agent %ld added to least fit list for domain %d at position 0 with fitness %g\n", c->Number(), id, c->HeuristicFitness() );
				}
				else
				{
					int i;

					// Find the position to be replaced
					for( i = 0; i < fDomains[id].fNumLeastFit; i++ )
						if( c->HeuristicFitness() < fDomains[id].fLeastFit[i]->HeuristicFitness() )	// worse than the one in this slot
							break;

					if( i < fDomains[id].fNumLeastFit )
					{
						// We need to move some of the items in the list down

						// If there's room left, add a slot
						if( fDomains[id].fNumLeastFit < fDomains[id].fMaxNumLeastFit )
							fDomains[id].fNumLeastFit++;

						// move everything up one, from i to end
						for( int j = fDomains[id].fNumLeastFit-1; j > i; j-- )
							fDomains[id].fLeastFit[j] = fDomains[id].fLeastFit[j-1];

					}
					else
						fDomains[id].fNumLeastFit++;	// we're adding to the end of the list, so increment the count

					// Store the new i-th worst
					fDomains[id].fLeastFit[i] = c;
					smPrint( "agent %ld added to least fit list for domain %d at position %d with fitness %g\n", c->Number(), id, i, c->HeuristicFitness() );
				}
			}
		}
	}

	// Following debug output is accurate only when there is a single domain
//	if( fDomains[0].fNumLeastFit > 0 )
//		printf( "%ld numSmitable = %d out of %d, from %ld agents out of %ld\n", fStep, fDomains[0].fNumLeastFit, fDomains[0].fMaxNumLeastFit, fDomains[0].numAgents, fDomains[0].maxNumAgents );

	fGeneStats.compute( fScheduler );
}

//---------------------------------------------------------------------------
// TSimulation::MateLockstep
//---------------------------------------------------------------------------
void TSimulation::MateLockstep( void )
{
	lsPrint( "t%ld: Triggering %d random births...\n", fStep, fLockstepNumBirthsAtTimestep );

	for( int count = 0; count < fLockstepNumBirthsAtTimestep; count++ )
	{
		agent* c = NULL;		// mommy
		agent* d = NULL;		// daddy

		/* select mommy. */

		int i = 0;

		int numAgents = objectxsortedlist::gXSortedObjects.getCount(AGENTTYPE);
		assert( numAgents < fMaxNumAgents );			// Since we've already done all the deaths that occurred at this timestep, we should always have enough room to process the births that happened at this timestep.

		agent* testAgent = NULL;
		//int randomIndex = int( round( randpw() * fDomains[kd].numAgents ) );	// pick from this domain V???
		int randomIndex = int( round( randpw() * numAgents ) );

		// As written, randAgent may not actually be the randomIndex-th agent in the domain, but it will be close,
		// and as long as there's a single legitimate agent for mating (right domain, long enough since last mating,
		// and long enough since birth) we will find and use it
		objectxsortedlist::gXSortedObjects.reset();
		while( objectxsortedlist::gXSortedObjects.nextObj( AGENTTYPE, (gobject**) &testAgent ) )
		{
			// Make sure it wasn't just birthed
			if( testAgent->Age() > 0 )
				c = testAgent;	// as long as there's a single legitimate agent for mating, Mommy will be non-NULL

			i++;

			if( (i > randomIndex) && (c != NULL) )
				break;
		}

		/* select daddy. */

		i = 0;
		randomIndex = int( round( randpw() * numAgents ) );

		// As written, randAgent may not actually be the randomIndex-th agent in the domain, but it will be close,
		// and as long as there's a single legitimate agent for mating (right domain, long enough since last mating, and
		// has enough energy, plus not the same as the mommy), we will find and use it
		objectxsortedlist::gXSortedObjects.reset();
		while( objectxsortedlist::gXSortedObjects.nextObj( AGENTTYPE, (gobject**) &testAgent ) )
		{
			// If it was not just birthed and it's not the same as mommy, it'll do for daddy.
			if( (testAgent->Age() > 0) && (testAgent->Number() != c->Number()) )
				d = testAgent;	// as long as there's another single legitimate agent for mating, Daddy will be non-NULL

			i++;

			if( (i > randomIndex) && (d != NULL) )
				break;
		}

		assert( c != NULL && d != NULL );				// If for some reason we can't select parents, then we'll become out of sync with LOCKSTEP-BirthDeaths.log

		lsPrint( "* I have selected Mommy(%ld) and Daddy(%ld). Population size = %d\n", c->Number(), d->Number(), numAgents );

		if( c == d )	// shouldn't be possible, but...
		{
			fprintf( stderr, "%s: c == d (%p, %ld)\n", __FUNCTION__, c, c->Number() );
		#if DebugLockStep
			fprintf( fdls, "%s: c == d (%p, %ld)\n", __FUNCTION__, c, c->Number() );
			fclose( fdls );
		#endif
			assert( c != d );
		}

		/* === We've selected our parents, now time to birth our agent. */

		ttPrint( "age %ld: agents # %ld & %ld are mating randomly\n", fStep, c->Number(), d->Number() );

		agent* e = agent::getfreeagent( this, &fStage );

		e->Genes()->crossover(c->Genes(), d->Genes(), true);
		e->setGenomeReady();
		e->grow( fMateWait );
		Energy eenergy = c->mating( fMateFitnessParameter, fMateWait, /*lockstep*/ true )
					   + d->mating( fMateFitnessParameter, fMateWait, /*lockstep*/ true );
		Energy minenergy = fMinMateFraction * ( c->GetMaxEnergy() + d->GetMaxEnergy() ) * 0.5;	// just a modest, reasonable amount; this doesn't matter as much in lockstep mode, though since we count virtual births it does matter a tiny bit; this leaves the agent requiring food prior to mating if the parents don't contribute enough
		eenergy.constrain( minenergy, e->GetMaxEnergy() );
		e->SetEnergy(eenergy);
		e->SetFoodEnergy(eenergy);
		float x =  0.01 + randpw() * (globals::worldsize - 0.02);
		float z = -0.01 - randpw() * (globals::worldsize - 0.02);
		float y = 0.5 * agent::config.agentHeight;
		e->settranslation(x, y, z);
		float yaw =  360.0 * randpw();
		c->setyaw(yaw);
		short kd = WhichDomain(x, z, 0);
		e->Domain(kd);
		fStage.AddObject(e);
		objectxsortedlist::gXSortedObjects.add(e); // Add the new agent directly to the list of objects (no new agent list); the e->listLink that gets auto stored here should be valid immediately

		fNewLifes++;
		fDomains[kd].numAgents++;
		//fNumberBorn++;	// we are count virtual births rather than actual births now when running in lockstep mode
		fDomains[kd].numborn++;
		fNumBornSinceCreated++;
		fDomains[kd].numbornsincecreated++;
		ttPrint( "age %ld: agent # %ld is born\n", fStep, e->Number() );
		birthPrint( "step %ld: agent # %ld born to %ld & %ld, at (%g,%g,%g), yaw=%g, energy=%g, domain %d (%d & %d)\n",
			fStep, e->Number(), c->Number(), d->Number(), e->x(), e->y(), e->z(), e->yaw(), e->Energy(), kd, id, jd );

		Birth( e, LifeSpan::BR_LOCKSTEP, c, d );

	}	// end of loop 'for( int count=0; count<fLockstepNumBirthsAtTimestep; count++ )'
}

//---------------------------------------------------------------------------
// TSimulation::GetMatePotential
//---------------------------------------------------------------------------
int TSimulation::GetMatePotential( agent *x )
{
	int potential = MATE__NIL;

	bool desiresMate = x->Mate() > fMateThreshold;
	if( fProbabilisticMating && desiresMate )
		desiresMate = randpw() < x->Mate();

	if( desiresMate )
	{
		potential |= MATE__DESIRED;

		bool preventedByCarry = IS_PREVENTED_BY_CARRY( Mate, x );
		bool preventedByMateWait = (x->Age() - x->LastMate()) < fMateWait;
		bool preventedByEnergy = x->NormalizedEnergy() <= fMinMateFraction;
		bool preventedByEatMateSpan = (fEatMateSpan > 0) && ( (fStep - x->LastEat()) >= fEatMateSpan );
		bool preventedByEatMateMinDistance = (fEatMateMinDistance > 0) && (x->LastEat() > 0) && (x->LastEatDistance() < fEatMateMinDistance);
		bool preventedByMaxVelocity = x->NormalizedSpeed() > fMaxMateVelocity;

#define __SET(var,macro) if( preventedBy##var ) potential |= MATE__PREVENTED__##macro

		__SET( Carry, CARRY );
		__SET( MateWait, MATE_WAIT );
		__SET( Energy, ENERGY );
		__SET( EatMateSpan, EAT_MATE_SPAN );
		__SET( EatMateMinDistance, EAT_MATE_MIN_DISTANCE );
		__SET( MaxVelocity, MAX_VELOCITY );

#undef __SET

#if OF1
		bool preventedByOF1 = x->Domain() != 1;
		if( preventedByOF1 ) potential |= MATE__PREVENTED__OF1;
#endif

	}

	return potential;
}

//---------------------------------------------------------------------------
// TSimulation::GetMateStatus
//---------------------------------------------------------------------------
int TSimulation::GetMateStatus( int xPotential, int yPotential )
{
	int xStatus = xPotential;

	if( xPotential & MATE__DESIRED )
	{
		if( yPotential != MATE__DESIRED )
		{
			xStatus |= MATE__PREVENTED__PARTNER;
		}
	}

	return xStatus;
}

//---------------------------------------------------------------------------
// TSimulation::GetMateDenialStatus
//---------------------------------------------------------------------------
int TSimulation::GetMateDenialStatus( agent *x, int *xStatus,
									  agent *y, int *yStatus,
									  short domainID )
{
	int status = MATE__NIL;
	Domain &domain = fDomains[domainID];

	bool preventedByMaxDomain = domain.numAgents >= domain.maxNumAgents;
	bool preventedByMaxWorld = fNumberAlive >= fMaxNumAgents;

	bool preventedByMaxMetabolism;
	int nmetabolisms = Metabolism::getNumberOfDefinitions();
	if( nmetabolisms > 1 ) {
		preventedByMaxMetabolism =
			( fNumberAliveWithMetabolism[x->GetMetabolism()->index] >= (fMaxNumAgents / nmetabolisms) )
			|| (fNumberAliveWithMetabolism[y->GetMetabolism()->index] >= (fMaxNumAgents / nmetabolisms) ) ;
	} else {
		preventedByMaxMetabolism = false;
	}


#define __SET(var,macro) if( preventedBy##var ) status |= MATE__PREVENTED__##macro

	__SET( MaxDomain, MAX_DOMAIN );
	__SET( MaxWorld, MAX_WORLD );
	__SET( MaxMetabolism, MAX_METABOLISM );

	if( status == MATE__NIL )
	{
		// We only check misc if not rejected by max. We do this so the random number
		// generator state is the same as in older versions of the simulator.
		bool preventedByMisc =
			(fMiscAgents >= 0)
			&& (domain.numbornsincecreated >= fMiscAgents)
			&& (randpw() >= x->MateProbability(y));

		__SET( Misc, MISC );
	}

#undef __SET

	*xStatus |= status;
	*yStatus |= status;

	return status;
}

//---------------------------------------------------------------------------
// TSimulation::Mate
//---------------------------------------------------------------------------
void TSimulation::Mate( agent *c,
						agent *d,
						AgentContactBeginEvent *contactEvent )
{
	int cMatePotential = GetMatePotential( c );
	int dMatePotential = GetMatePotential( d );
	int cMateStatus = GetMateStatus( cMatePotential, dMatePotential );
	int dMateStatus = GetMateStatus( dMatePotential, cMatePotential );

	if( (cMateStatus == MATE__DESIRED) && (dMateStatus == MATE__DESIRED) )
	{
		// the agents are mate-worthy, so now deal with other conditions...

		// test for steady-state GA vs. natural selection
		if( (fHeuristicFitnessWeight != 0.0) || (fComplexityFitnessWeight != 0.0) || fLockStepWithBirthsDeathsLog )
		{
			// we're using the steady state GA (instead of natural selection) or we're in lockstep mode
			// count virtual offspring (offspring that would have resulted from
			// otherwise successful mating behaviors), but don't actually instantiate them

			// We send lockstep = false to agent::mating() even when fLockStepWithBirthsDeathsLog is true,
			// because this virtual birth was chosen by the agents, as opposed to the births foisted on
			// agents at random as part of the lockstep mode.
			(void) c->mating( fMateFitnessParameter, fMateWait, /*lockstep*/ false );
			(void) d->mating( fMateFitnessParameter, fMateWait, /*lockstep*/ false );

			//cout << "t=" << fStep sp "mating c=" << c->Number() sp "(m=" << c->Mate() << ",lm=" << c->LastMate() << ",e=" << c->Energy() << ",x=" << c->x() << ",z=" << c->z() << ",r=" << c->radius() << ")" nl;
			//cout << "          & d=" << d->Number() sp "(m=" << d->Mate() << ",lm=" << d->LastMate() << ",e=" << d->Energy() << ",x=" << d->x() << ",z=" << d->z() << ",r=" << d->radius() << ")" nl;
			//if( sqrt((d->x()-c->x())*(d->x()-c->x())+(d->z()-c->z())*(d->z()-c->z())) > (d->radius()+c->radius()) )
			//	cout << "            ***** no overlap *****" nl;

			fNumberBornVirtual++;
			Birth( NULL, LifeSpan::BR_VIRTUAL, c, d );
		}
		else
		{
			// we're using natural selection (or are lockstepped to a previous natural selection run),
			// so proceed with the normal mating process (attempt to mate for offspring production if there's room)
			short kd = WhichDomain(0.5*(c->x()+d->x()),
								   0.5*(c->z()+d->z()),
								   0);

			Smite( kd, c, d );

			int denialStatus = GetMateDenialStatus( c, &cMateStatus,
													d, &dMateStatus,
													kd );

			if( denialStatus != MATE__NIL )
			{
				fBirthDenials++;

				if( denialStatus & MATE__PREVENTED__MISC )
				{
					fMiscDenials++;
				}
			}
			else
			{
				ttPrint( "age %ld: agents # %ld & %ld are mating\n", fStep, c->Number(), d->Number() );

				fNumBornSinceCreated++;
				fDomains[kd].numbornsincecreated++;

				agent* e = agent::getfreeagent(this, &fStage);

				e->Genes()->crossover(c->Genes(), d->Genes(), true);
				e->setGenomeReady();

				Energy eenergy = c->mating( fMateFitnessParameter, fMateWait, /*lockstep*/ false )
							   + d->mating( fMateFitnessParameter, fMateWait, /*lockstep*/ false );

				float x = 0.5*(c->x() + d->x());
				float y = 0.5*(c->y() + d->y());
				float z = 0.5*(c->z() + d->z());
				float yaw = AverageAngles( c->yaw(), d->yaw() );
				if( fRandomBirthLocation )
				{
					float distance = globals::worldsize * fRandomBirthLocationRadius * randpw();
					float angle = 2*M_PI * randpw();

					x += distance * cosf( angle );
					z -= distance * sinf( angle );

					x = clamp( x, 0.01, globals::worldsize - 0.01 );
					z = clamp( z, -globals::worldsize + 0.01, -0.01 );
				}

				e->settranslation( x, y, z );
				e->setyaw( yaw );

				e->Domain(kd);
				fNewLifes++;
				fDomains[kd].numAgents++;
				fNumberBorn++;
				fDomains[kd].numborn++;
				//if( fStep > 50 )
				//	exit( 0 );

				ttPrint( "age %ld: agent # %ld is born\n", fStep, e->Number() );
				birthPrint( "step %ld: agent # %ld born to %ld & %ld, at (%g,%g,%g), yaw=%g, energy=%g, domain %d (%d & %d)\n",
							fStep, e->Number(), c->Number(), d->Number(), e->x(), e->y(), e->z(), e->yaw(), e->Energy(), kd, id, jd );

				Birth( e, LifeSpan::BR_NATURAL, c, d );

				// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
				// !!! POST PARALLEL
				// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                fScheduler.postParallel( [=]() {
                        e->grow( fMateWait );

                        e->SetEnergy(eenergy);
                        e->SetFoodEnergy(eenergy);
                    });                            

				// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
				// !!! POST SERIAL
				// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                fScheduler.postSerial( [=]() {
                        fStage.AddObject(e);
                        gdlink<gobject*> *saveCurr = objectxsortedlist::gXSortedObjects.getcurr();
                        objectxsortedlist::gXSortedObjects.add(e); // Add the new agent directly to the list of objects (no new agent list); the e->listLink that gets auto stored here should be valid immediately
                        objectxsortedlist::gXSortedObjects.setcurr( saveCurr );
                    });
			}
		}	// steady-state GA vs. natural selection
	}	// if agents are trying to mate

	contactEvent->mate( c, cMateStatus );
	contactEvent->mate( d, dMateStatus );

	debugcheck( "after all mating is complete" );
}

//---------------------------------------------------------------------------
// TSimulation::Smite
//---------------------------------------------------------------------------
void TSimulation::Smite( short kd,
						 agent *c,
						 agent *d )
{
	if( fSmiteMode == 'L' )		// smite the least fit
	{
		if( (fDomains[kd].numAgents >= fDomains[kd].maxNumAgents) &&	// too many agents to reproduce withing a bit of smiting
			(fDomains[kd].fNumLeastFit > fDomains[kd].fNumSmited) )			// we've still got some left that are suitable for smiting
		{
			while( (fDomains[kd].fNumSmited < fDomains[kd].fNumLeastFit) &&		// there are any left to smite
				   ((fDomains[kd].fLeastFit[fDomains[kd].fNumSmited] == c) ||	// trying to smite mommy
					(fDomains[kd].fLeastFit[fDomains[kd].fNumSmited] == d) ||	// trying to smite daddy
					((fCurrentFittestCount > 0) && (fDomains[kd].fLeastFit[fDomains[kd].fNumSmited]->HeuristicFitness() >= fCurrentMaxFitness[fCurrentFittestCount-1]))) )	// trying to smite one of the fittest
			{
				// We would have smited one of our mating pair, or one of the fittest, which wouldn't be prudent,
				// so just step over them and see if there's someone else to smite
				fDomains[kd].fNumSmited++;
			}
			if( fDomains[kd].fNumSmited < fDomains[kd].fNumLeastFit )	// we've still got someone to smite, so do it
			{
				smPrint( "About to smite least-fit agent #%d in domain %d\n", fDomains[kd].fLeastFit[fDomains[kd].fNumSmited]->Number(), kd );
				Kill( fDomains[kd].fLeastFit[fDomains[kd].fNumSmited], LifeSpan::DR_SMITE );
				fDomains[kd].fNumSmited++;
				fNumberDiedSmite++;
				//cout << "********************* SMITE *******************" nlf;	//dbg
			}
		}
	}
	else if( fSmiteMode == 'R' )				/// RANDOM SMITE
	{
		// If necessary, smite a random agent in this domain
		if( fDomains[kd].numAgents >= fDomains[kd].maxNumAgents )
		{
			int i = 0;
			agent* testAgent;
			agent* randAgent = NULL;
			int randomIndex = int( floor( randpw() * fDomains[kd].numAgents ) );	// pick from this domain

			gdlink<gobject*> *saveCurr = objectxsortedlist::gXSortedObjects.getcurr();	// save the state of the x-sorted list

			// As written, randAgent may not actually be the randomIndex-th agent in the domain, but it will be close,
			// and as long as there's a single legitimate agent for smiting (right domain, old enough, and not one of the
			// parents), we will find and smite it
			objectxsortedlist::gXSortedObjects.reset();
			while( (i <= randomIndex) && objectxsortedlist::gXSortedObjects.nextObj( AGENTTYPE, (gobject**) &testAgent ) )
			{
				// If it's from the right domain, it's old enough, and it's not one of the parents, allow it
				if( testAgent->Domain() == kd )
				{
					i++;	// if it's in the right domain, increment even if we're not allowed to smite it

					if( (testAgent->Age() > fSmiteAgeFrac*testAgent->MaxAge()) && (testAgent->Number() != c->Number()) && (testAgent->Number() != d->Number()) )
						randAgent = testAgent;	// as long as there's a single legitimate agent for smiting in this domain, randAgent will be non-NULL
				}

				if( (i > randomIndex) && (randAgent != NULL) )
					break;
			}

			objectxsortedlist::gXSortedObjects.setcurr( saveCurr );	// restore the state of the x-sorted list

			if( randAgent )	// if we found any legitimately smitable agent...
			{
				fDomains[kd].fNumSmited++;
				fNumberDiedSmite++;
				Kill( randAgent, LifeSpan::DR_SMITE );
			}
		}
	}
}

//---------------------------------------------------------------------------
// TSimulation::GetFightStatus
//---------------------------------------------------------------------------
int TSimulation::GetFightStatus( agent *x,
								 agent *y,
								 float *out_power )
{
	int status = FIGHT__NIL;

	bool desiresFight = x->Fight() > fFightThreshold;
	if( desiresFight )
	{
		status |= FIGHT__DESIRED;

		*out_power = fFightFraction * x->Strength() * x->SizeAdvantage() * x->Fight() * x->NormalizedEnergy();

		bool preventedByCarry = IS_PREVENTED_BY_CARRY( Fight, x );
		bool preventedByShield = y->IsCarrying( fShieldObjects );
		bool preventedByPower = *out_power <= 0.0;

#define __SET(var,macro) if( preventedBy##var ) status |= FIGHT__PREVENTED__##macro

		__SET( Carry, CARRY );
		__SET( Shield, SHIELD );
		__SET( Power, POWER );

#undef __SET

		if( status != FIGHT__DESIRED )
		{
			*out_power = 0.0;
		}
	}
	else
	{
		*out_power = 0.0;
	}

	return status;
}

//---------------------------------------------------------------------------
// TSimulation::Fight
//---------------------------------------------------------------------------
void TSimulation::Fight( agent *c,
						 agent *d,
						 AgentContactBeginEvent *contactEvent,
						 bool *cDied,
						 bool *dDied)
{
#if DEBUGCHECK
	unsigned long cnum = c->Number();
	unsigned long dnum = d->Number();
#endif
	float cpower;
	float dpower;
	int cstatus = GetFightStatus( c, d, &cpower );
	int dstatus = GetFightStatus( d, c, &dpower );

	contactEvent->fight( c, cstatus );
	contactEvent->fight( d, dstatus );

	if ( (cpower > 0.0) || (dpower > 0.0) )
	{
		ttPrint( "age %ld: agents # %ld & %ld are fighting\n", fStep, c->Number(), d->Number() );

		// somebody wants to fight
		fNumberFights++;

		if( cpower > 0.0 )
		{
			Energy ddamage = d->damage( cpower * fPower2Energy, fFightMode == FM_NULL );
			if( !ddamage.isZero() )
				logs->postEvent( EnergyEvent(c, d, c->Fight(), ddamage, EnergyEvent::Fight) );
		}

		if( dpower > 0.0 )
		{
			Energy cdamage = c->damage( dpower * fPower2Energy, fFightMode == FM_NULL );
			if( !cdamage.isZero() )
				logs->postEvent( EnergyEvent(d, c, d->Fight(), cdamage, EnergyEvent::Fight) );
		}

		if( !fLockStepWithBirthsDeathsLog )
		{
			// If we're not running in LockStep mode, allow natural deaths
			if (d->GetEnergy().isDepleted())
			{
				//cout << "before deaths2 "; agent::config.xSortedAgents.list();	//dbg
				Kill( d, LifeSpan::DR_FIGHT );
				fNumberDiedFight++;
				//cout << "after deaths2 "; agent::config.xSortedAgents.list();	//dbg

				*dDied = true;
			}
			if (c->GetEnergy().isDepleted())
			{
				objectxsortedlist::gXSortedObjects.toMark( AGENTTYPE ); // point back to c
				Kill( c, LifeSpan::DR_FIGHT );
				fNumberDiedFight++;

				// note: this leaves list pointing to item before c, and markedAgent set to previous agent
				//objectxsortedlist::gXSortedObjects.setMarkPrevious( AGENTTYPE );	// if previous object was a agent, this would step one too far back, I think - lsy
				//cout << "after deaths3 "; agent::config.xSortedAgents.list();	//dbg
				*cDied = true;
			}
		}
	}

	debugcheck( "after fight between agents %lu and %lu, %s", cnum, dnum, *cDied ? (*dDied ? "both died" : "c died") : (*dDied ? "d died" : "neither died") );
}

//---------------------------------------------------------------------------
// TSimulation::GetGiveStatus
//---------------------------------------------------------------------------
int TSimulation::GetGiveStatus( agent *x,
								Energy &out_energy )
{
	int status = GIVE__NIL;

	bool desiresGive = x->Give() > fGiveThreshold;
	if( desiresGive )
	{
		status |= GIVE__DESIRED;

		out_energy = x->GetEnergy() * x->Give() * fGiveFraction;

		bool preventedByCarry = IS_PREVENTED_BY_CARRY( Give, x );
		bool preventedByEnergy = out_energy.isDepleted();

#define __SET(var,macro) if( preventedBy##var ) status |= GIVE__PREVENTED__##macro

		__SET( Carry, CARRY );
		__SET( Energy, ENERGY );

#undef __SET

		if( status != GIVE__DESIRED )
		{
			out_energy = 0.0;
		}
	}
	else
	{
		out_energy = 0.0;
	}

	return status;
}

//---------------------------------------------------------------------------
// TSimulation::Give
//---------------------------------------------------------------------------
void TSimulation::Give( agent *x,
						agent *y,
						AgentContactBeginEvent *contactEvent,
						bool *xDied,
						bool toMarkOnDeath )
{
	Energy energy;
	int xstatus = GetGiveStatus( x, energy );

	contactEvent->give( x, xstatus );

#if DEBUGCHECK
	unsigned long xnum = x->Number();
#endif

	if( !energy.isDepleted() )
	{
		y->receive( x, energy );

		logs->postEvent( EnergyEvent(x, y, x->Give(), energy, EnergyEvent::Give) );

		if( !fLockStepWithBirthsDeathsLog )
		{
			if (x->GetEnergy().isDepleted())
			{
				if( toMarkOnDeath )
					objectxsortedlist::gXSortedObjects.toMark( AGENTTYPE ); // point back to x
				Kill( x, LifeSpan::DR_NATURAL );
#if GIVE_TODO
				fNumberDiedGive++;
#endif

				*xDied = true;
			}
		}
	}

	debugcheck( "after %lu gave %g energy to %lu%s", xnum, energy, y->Number(), energy == 0.0 ? " (did NOT give)" : "" );
}



//---------------------------------------------------------------------------
// TSimulation::Eat
//---------------------------------------------------------------------------
void TSimulation::Eat( agent *c, bool *cDied )
{
	bool ateBackwardFood;
	food* f = NULL;
	bool eatAllowed = true;
	bool eatFailedYaw = false;
	bool eatFailedVel = false;
	bool eatFailedMinAge = false;
	bool eatAttempted = false;

	// Just to be slightly more like the old multi-x-sorted list version of the code, look backwards first

	// set the list back to the agent mark, so we can look backward from that point
	objectxsortedlist::gXSortedObjects.toMark( AGENTTYPE ); // point list back to c

	if( IS_PREVENTED_BY_CARRY(Eat, c) )
	{
		eatAllowed = false;
	}
	if( c->NormalizedSpeed() > fMaxEatVelocity )
	{
		eatAllowed = false;
		eatFailedVel = true;
	}
	else if( c->NormalizedSpeed() < fMinEatVelocity )
	{
		eatAllowed = false;
		eatFailedVel = true;
	}
	if( fabs(c->NormalizedYaw()) > fMaxEatYaw )
	{
		eatAllowed = false;
		eatFailedYaw = true;
	}
	if( c->Age() < c->GetMetabolism()->minEatAge )
	{
		eatAllowed = false;
		eatFailedMinAge = true;
	}
	if( (fStep - c->LastEat()) < fEatWait )
	{
		eatAllowed = false;
	}

	// look for food in the -x direction
	ateBackwardFood = false;
#if CompatibilityMode
	// go backwards in the list until we reach a place where even the largest possible piece of food
	// would entirely precede our agent, and no smaller piece of food sorting after it, but failing
	// to reach the agent can prematurely terminate the scan back (hence the factor of 2.0),
	// so we can then search forward from there
	while( objectxsortedlist::gXSortedObjects.prevObj( FOODTYPE, (gobject**) &f ) )
		if( (f->x() + 2.0*food::gMaxFoodRadius) < (c->x() - c->radius()) )
			break;
#else // CompatibilityMode
	while( objectxsortedlist::gXSortedObjects.prevObj( FOODTYPE, (gobject**) &f ) )
	{
		if( (f->x() + f->radius()) < (c->x() - c->radius()) )
		{
			// end of food comes before beginning of agent, so there is no overlap
			// if we've gone so far back that the largest possible piece of food could not overlap us,
			// then we can stop searching for this agent's possible foods in the backward direction
			if( (f->x() + 2.0*food::gMaxFoodRadius) < (c->x() - c->radius()) )
				break;  // so get out of the backward food while loop
		}
		else
		{
			// beginning of food comes before end of agent, so there is overlap in x
			// time to check for overlap in z
			if( fabs( f->z() - c->z() ) < ( f->radius() + c->radius() ) )
			{
				eatAttempted = true;
				if( !eatAllowed )
					break;
				// also overlap in z, so they really interact
				ttPrint( "step %ld: agent # %ld is eating\n", fStep, c->Number() );
				Energy foodEnergyLost;
				Energy energyEaten;
				c->eat( f, fEatFitnessParameter, fEat2Consume, fEatThreshold, fStep, foodEnergyLost, energyEaten );
				logs->postEvent( EnergyEvent(c, f, c->Eat(), energyEaten, EnergyEvent::Eat) );
				if( fEvents )
					fEvents->AddEvent( fStep, c->Number(), 'e' );

				FoodEnergyOut( foodEnergyLost );
				fEnergyEaten += energyEaten;

				eatPrint( "at step %ld, agent %ld at (%g,%g) with rad=%g wasted %g units of food at (%g,%g) with rad=%g\n", fStep, c->Number(), c->x(), c->z(), c->radius(), foodEaten, f->x(), f->z(), f->radius() );

				if( f->isDepleted() || fFoodRemoveFirstEat )  // all gone
				{
					RemoveFood( f );
				}

				// but this guy only gets to eat from one food source
				ateBackwardFood = true;
				break;  // so get out of the backward food while loop
			}
		}
	}	// backward while loop on food
#endif // CompatibilityMode

	if( !ateBackwardFood && !eatAttempted )
	{
	#if ! CompatibilityMode
		// set the list back to the agent mark, so we can look forward from that point
		objectxsortedlist::gXSortedObjects.toMark( AGENTTYPE ); // point list back to c
	#endif

		// look for food in the +x direction
		while( objectxsortedlist::gXSortedObjects.nextObj( FOODTYPE, (gobject**) &f ) )
		{
			if( (f->x() - f->radius()) > (c->x() + c->radius()) )
			{
				// beginning of food comes after end of agent, so there is no overlap,
				// and we can stop searching for this agent's possible foods in the forward direction
				break;  // so get out of the forward food while loop
			}
			else
			{
	#if CompatibilityMode
				if( ((f->x() + f->radius()) > (c->x() - c->radius()))  &&		// end of food comes after beginning of agent, and
					(fabs( f->z() - c->z() ) < (f->radius() + c->radius())) )	// there is overlap in z
	#else
				// beginning of food comes before end of agent, so there is overlap in x
				// time to check for overlap in z
				if( fabs( f->z() - c->z() ) < (f->radius() + c->radius()) )
	#endif
				{
					eatAttempted = true;
					if( !eatAllowed )
						break;
					// also overlap in z, so they really interact
					ttPrint( "step %ld: agent # %ld is eating\n", fStep, c->Number() );
					Energy foodEnergyLost;
					Energy energyEaten;
					c->eat( f, fEatFitnessParameter, fEat2Consume, fEatThreshold, fStep, foodEnergyLost, energyEaten );
					logs->postEvent( EnergyEvent(c, f, c->Eat(), energyEaten, EnergyEvent::Eat) );
					if( fEvents )
						fEvents->AddEvent( fStep, c->Number(), 'e' );

					FoodEnergyOut( foodEnergyLost );
					fEnergyEaten += energyEaten;

					eatPrint( "at step %ld, agent %ld at (%g,%g) with rad=%g wasted %g units of food at (%g,%g) with rad=%g\n", fStep, c->Number(), c->x(), c->z(), c->radius(), foodEaten, f->x(), f->z(), f->radius() );

					if( f->isDepleted() || fFoodRemoveFirstEat )  // all gone
					{
						RemoveFood( f );
					}

					// but this guy only gets to eat from one food source
					break;  // so get out of the forward food while loop
				}
			}
		} // forward while loop on food
	} // if( !ateBackwardFood )

	if( eatAttempted )
	{
		fEatStatistics.AgentEatAttempt( eatAllowed, eatFailedYaw, eatFailedVel, eatFailedMinAge );
	}

	objectxsortedlist::gXSortedObjects.toMark( AGENTTYPE ); // point list back to c
	if( !fLockStepWithBirthsDeathsLog )
	{
		// If we're not running in LockStep mode, allow natural deaths
		if( c->GetEnergy().isDepleted() ||
			((c->IsSeed() || c->Age() >= agent::config.starvationWait) && c->GetFoodEnergy().isDepleted( c->GetStarvationFoodEnergy() )) )
		{
			// note: this leaves list pointing to item before c, and markedAgent set to previous agent
			Kill( c, LifeSpan::DR_EAT );
			fNumberDiedEat++;
			*cDied = true;
		}
	}
	debugcheck( "after all agents had a chance to eat" );
}

//---------------------------------------------------------------------------
// TSimulation::Carry
//---------------------------------------------------------------------------
void TSimulation::Carry( agent* c )
{
	// Look for objects for this agent to carry

	// Is the agent expressing its pickup behavior?
	if( c->Pickup() > fPickupThreshold )
	{
		// Don't pick up more than we can carry
		if( c->NumCarries() < agent::config.maxCarries )
		{
			Pickup( c );
		}
	}

	// Is the agent expressing its drop behavior?
	if( c->Drop() > fDropThreshold )
	{
		// Make sure there's something to put down
		if( c->NumCarries() > 0 )
		{
			Drop( c );
		}
	}
}

//---------------------------------------------------------------------------
// TSimulation::Pickup
//---------------------------------------------------------------------------
void TSimulation::Pickup( agent* c )
{
	gobject* o;

	// set the list back to the agent mark, so we can look backward from that point
	objectxsortedlist::gXSortedObjects.toMark( AGENTTYPE ); // point list back to c

	// look in the -x direction for something to carry
	while( objectxsortedlist::gXSortedObjects.prevObj( fCarryObjects, (gobject**) &o ) )
	{
		if( o->BeingCarried() || (o->NumCarries() > 0) )
			continue;	// already carrying or being carried, so nothing we can do with it

		if( ( o->x() + o->radius() ) < ( c->x() - c->radius() ) )
		{
			// end of object comes before beginning of agent, so there is no overlap
			// if we've gone so far back that the largest possible object could not overlap us,
			// then we can stop searching for this agent's possible pickups in the backward direction
			if( ( o->x() + 2.0 * max( max( food::gMaxFoodRadius, agent::config.maxRadius ), brick::gBrickRadius ) ) < ( c->x() - c->radius() ) )
				break;  // so get out of the backward object while loop
		}
		else
		{
			// beginning of object comes before end of agent, so there is overlap in x
			// time to check for overlap in z
			if( fabs( o->z() - c->z() ) < ( o->radius() + c->radius() ) )
			{
				// also overlap in z, so they really interact
				ttPrint( "step %ld: agent # %ld is picking up object of type %lu\n", fStep, c->Number(), o->getType() );

				c->PickupObject( o );

				if( c->NumCarries() >= agent::config.maxCarries )	// carrying as much as we can,
					break;								// so get out of the backward while loop
			}
		}
	}

	// can we pick up anything else?
	if( c->NumCarries() < agent::config.maxCarries )
	{
		// set the list back to the agent mark, so we can look forward from that point
		objectxsortedlist::gXSortedObjects.toMark( AGENTTYPE ); // point list back to c

		// look in the +x direction for something to pick up
		while( objectxsortedlist::gXSortedObjects.nextObj( fCarryObjects, (gobject**) &o ) )
		{
			if( o->BeingCarried() || (o->NumCarries() > 0) )
				continue;	// already carrying or being carried, so nothing we can do with it

			if( (o->x() - o->radius()) > (c->x() + c->radius()) )
			{
				// beginning of object comes after end of agent, so there is no overlap,
				// and we can stop searching for this agent's possible pickups in the forward direction
				break;  // so get out of the forward object while loop
			}
			else
			{
				// beginning of object comes before end of agent, so there is overlap in x
				// time to check for overlap in z
				if( fabs( o->z() - c->z() ) < (o->radius() + c->radius()) )
				{
					// also overlap in z, so they really interact
					ttPrint( "step %ld: agent # %ld is picking up object of type %d\n", fStep, c->Number(), o->getType() );

					c->PickupObject( o );

					if( c->NumCarries() >= agent::config.maxCarries )	// carrying as much as we can,
						break;								// so get out of the forward while loop
				}
			}
		}
	}

	objectxsortedlist::gXSortedObjects.toMark( AGENTTYPE ); // point list back to c

	debugcheck( "after all agents had a chance to pickup objects" );
}

//---------------------------------------------------------------------------
// TSimulation::Drop
//---------------------------------------------------------------------------
void TSimulation::Drop( agent* c )
{
	c->DropMostRecent();

	debugcheck( "after dropping most recent" );
}

//---------------------------------------------------------------------------
// TSimulation::Fitness
//---------------------------------------------------------------------------
void TSimulation::Fitness( agent *c )
{
	if( c->Age() >= (fSmiteAgeFrac * c->MaxAge()) )
	{
		fAverageFitness += c->HeuristicFitness();	// will divide by fTotalHeuristicFitness later
		fNumAverageFitness++;
	}
	if( (fCurrentFittestCount < MAXFITNESSITEMS) || (c->HeuristicFitness() > fCurrentMaxFitness[fCurrentFittestCount-1]) )
	{
		if( (fCurrentFittestCount == 0) || ((c->HeuristicFitness() <= fCurrentMaxFitness[fCurrentFittestCount-1]) && (fCurrentFittestCount < MAXFITNESSITEMS)) )	// just append
		{
			fCurrentMaxFitness[fCurrentFittestCount] = c->HeuristicFitness();
			fCurrentFittestAgent[fCurrentFittestCount] = c;
			fCurrentFittestCount++;
		#if DebugMaxFitness
			printf( "appended agent %08lx (%4ld) to fittest list at position %d with fitness %g, count = %d\n", (unsigned long) c, c->Number(), fCurrentFittestCount-1, c->HeuristicFitness(), fCurrentFittestCount );
		#endif
		}
		else	// must insert
		{
			int i;

			for( i = 0; i <  fCurrentFittestCount ; i++ )
			{
				if( c->HeuristicFitness() > fCurrentMaxFitness[i] )
					break;
			}

			for( int j = min( fCurrentFittestCount, MAXFITNESSITEMS-1 ); j > i; j-- )
			{
				fCurrentMaxFitness[j] = fCurrentMaxFitness[j-1];
				fCurrentFittestAgent[j] = fCurrentFittestAgent[j-1];
			}

			fCurrentMaxFitness[i] = c->HeuristicFitness();
			fCurrentFittestAgent[i] = c;
			if( fCurrentFittestCount < MAXFITNESSITEMS )
				fCurrentFittestCount++;
		#if DebugMaxFitness
			printf( "inserted agent %08lx (%4ld) into fittest list at position %ld with fitness %g, count = %d\n", (unsigned long) c, c->Number(), i, c->HeuristicFitness(), fCurrentFittestCount );
		#endif
		}
	}

	debugcheck( "after current fitness lists maintained" );
}

//---------------------------------------------------------------------------
// TSimulation::CreateAgents
//---------------------------------------------------------------------------
void TSimulation::CreateAgents( void )
{
	long maxToCreate = fMaxNumAgents - (long)(objectxsortedlist::gXSortedObjects.getCount(AGENTTYPE));
    if (maxToCreate > 0)
    {
        // provided there are less than the maximum number of agents already

		// Due to an imbalance in the number of agents in the various domains, and the fact that
		// maximum numbers are not enforced for domains as agents travel of their own accord,
		// the sum of domain would-be creations may exceed the possible number of creations,
		// so we "load balance" creations, favoring the domains that need agents the most, by
		// not creating agents in domains that need them the least.
		long numToCreate = 0;
		short id;
		for (id = 0; id < fNumDomains; id++)
		{
			fDomains[id].numToCreate = max(0L, fDomains[id].minNumAgents - fDomains[id].numAgents);
			numToCreate += fDomains[id].numToCreate;
		}
//		printf( "num[0]= %ld, num[1] = %ld, num[2] = %ld, num = %ld, max = %ld\n",
//				fDomains[0].numToCreate, fDomains[1].numToCreate, fDomains[2].numToCreate,
//				numToCreate, maxToCreate );
		while( numToCreate > maxToCreate )
		{
			int domainWithLeastNeed = -1;
			long leastAgentsNeeded = fMaxNumAgents+1;	// Has to be lest than this
			for (id = 0; id < fNumDomains; id++)
			{
				if (fDomains[id].numToCreate > 0 && fDomains[id].numToCreate < leastAgentsNeeded)
				{
					leastAgentsNeeded = fDomains[id].numToCreate;
					domainWithLeastNeed = id;
				}
			}
//			printf( "num[0]= %ld, num[1] = %ld, num[2] = %ld, num = %ld, max = %ld, least = %ld, domLeast = %d\n",
//					fDomains[0].numToCreate, fDomains[1].numToCreate, fDomains[2].numToCreate,
//					numToCreate, maxToCreate, leastAgentsNeeded, domainWithLeastNeed );
			fDomains[domainWithLeastNeed].numToCreate--;
			numToCreate--;
		}

		// first deal with the individual domains
        for (id = 0; id < fNumDomains; id++)
        {
			// create as many agents as we need (and are allowed) for this domain
            for (int ic = 0; ic < fDomains[id].numToCreate; ic++)
            {
                fNumberCreated++;
                fDomains[id].numcreated++;
                fNumBornSinceCreated = 0;
                fDomains[id].numbornsincecreated = 0;
                fLastCreated = fStep;
                fDomains[id].lastcreate = fStep;
                agent* newAgent = agent::getfreeagent(this, &fStage);

                if ( fDomains[id].fittest && fDomains[id].fittest->isFull() )
                {
                    // the list exists and is full
                    if (fFitness1Frequency
                    	&& ((fDomains[id].numcreated / fFitness1Frequency) * fFitness1Frequency == fDomains[id].numcreated) )
                    {
                        // revive 1 fittest
                        newAgent->Genes()->copyFrom(fDomains[id].fittest->get(0)->genes);
                        fNumberCreated1Fit++;
						gaPrint( "%5ld: domain %d creation from one fittest (%4lu) %4ld\n", fStep, id, fDomains[id].fittest->get(0)->agentID, fNumberCreated1Fit );
                    }
                    else if (fFitness2Frequency
                    		 && ((fDomains[id].numcreated / fFitness2Frequency) * fFitness2Frequency == fDomains[id].numcreated) )
                    {
                        // mate 2 from array of fittest
						if( fTournamentSize > 0 )
						{
							// using tournament selection
							int parent1, parent2;
							PickParentsUsingTournament(fDomains[id].fittest->size(), &parent1, &parent2);
							newAgent->Genes()->crossover(fDomains[id].fittest->get(parent1)->genes,
														 fDomains[id].fittest->get(parent2)->genes,
														 true);
							fNumberCreated2Fit++;
							gaPrint( "%5ld: domain %d creation from two (%d, %d) fittest (%4lu, %4lu) %4ld\n", fStep, id, parent1, parent2, fDomains[id].fittest->get(parent1)->agentID, fDomains[id].fittest->get(parent2)->agentID, fNumberCreated2Fit );
						}
						else
						{
							// by iterating through the array of fittest
							newAgent->Genes()->crossover(fDomains[id].fittest->get(fDomains[id].ifit)->genes,
														 fDomains[id].fittest->get(fDomains[id].jfit)->genes,
														 true);
							fNumberCreated2Fit++;
							gaPrint( "%5ld: domain %d creation from two (%d, %d) fittest (%4lu, %4lu) %4ld\n", fStep, id, fDomains[id].ifit, fDomains[id].jfit, fDomains[id].fittest->get(fDomains[id].ifit)->agentID, fDomains[id].fittest->get(fDomains[id].jfit)->agentID, fNumberCreated2Fit );
							ijfitinc(fDomains[id].fittest->size(), &(fDomains[id].ifit), &(fDomains[id].jfit));
                        }
                    }
                    else
                    {
                        // otherwise, just generate a random, hopeful monster
                        newAgent->Genes()->randomize();
                        fNumberCreatedRandom++;
						gaPrint( "%5ld: domain %d creation random (%4ld)\n", fStep, id, fNumberCreatedRandom );
                    }
                }
                else
                {
                    // otherwise, just generate a random, hopeful monster
                    newAgent->Genes()->randomize();
                    fNumberCreatedRandom++;
					gaPrint( "%5ld: domain %d creation random early (%4ld)\n", fStep, id, fNumberCreatedRandom );
                }

				newAgent->setGenomeReady();

				// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
				// !!! POST PARALLEL
				// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                fScheduler.postParallel( [=]() {
                        newAgent->grow( fMateWait );
                    });

				float x = randpw() * (fDomains[id].absoluteSizeX - 0.02) + fDomains[id].startX + 0.01;
				float z = randpw() * (fDomains[id].absoluteSizeZ - 0.02) + fDomains[id].startZ + 0.01;
				float y = 0.5 * agent::config.agentHeight;
				float yaw = randpw() * 360.0;
			#if TestWorld
				x = fDomains[id].xleft  +  0.666 * fDomains[id].xsize;
				z = - globals::worldsize * ((float) (i+1) / (fDomains[id].initNumAgents + 1));
				yaw = 95.0;
			#endif
                newAgent->settranslation( x, y, z );
                newAgent->setyaw( yaw );
                newAgent->Domain(id);
                fStage.AddObject(newAgent);
                fDomains[id].numAgents++;
				objectxsortedlist::gXSortedObjects.add(newAgent);
				fNewLifes++;


				// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
				// !!! POST SERIAL
				// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                fScheduler.postSerial( [=]() {
                        FoodEnergyIn( newAgent->GetFoodEnergy() );
                    });

				Birth( newAgent, LifeSpan::BR_CREATE );
            }
        }

        debugcheck( "after agent creations for domains" );

		// then deal with global creation if necessary

        while (((long)(objectxsortedlist::gXSortedObjects.getCount(AGENTTYPE))) < fMinNumAgents)
        {
            fNumberCreated++;
            numglobalcreated++;

            if ((numglobalcreated == 1) && (fNumDomains > 1))
                errorflash(0, "Possible global influence on domains due to minNumAgents");

            fNumBornSinceCreated = 0;
            fLastCreated = fStep;

            agent* newAgent = agent::getfreeagent(this, &fStage);

            if( fFittest && fFittest->isFull() )
            {
                if( fFitness1Frequency
                	&& ((numglobalcreated / fFitness1Frequency) * fFitness1Frequency == numglobalcreated) )
                {
                    // revive 1 fittest
                    newAgent->Genes()->copyFrom( fFittest->get(0)->genes );
                    fNumberCreated1Fit++;
					gaPrint( "%5ld: global creation from one fittest (%4lu) %4ld\n", fStep, fFittest->get(0)->agentID, fNumberCreated1Fit );
                }
                else if( fFitness2Frequency && ((numglobalcreated / fFitness2Frequency) * fFitness2Frequency == numglobalcreated) )
                {
                    // mate 2 from array of fittest
					if( fTournamentSize > 0 )
					{
						// using tournament selection
						int parent1, parent2;
						TSimulation::PickParentsUsingTournament(fFittest->size(), &parent1, &parent2);
						newAgent->Genes()->crossover( fFittest->get(parent1)->genes, fFittest->get(parent2)->genes, true );
						fNumberCreated2Fit++;
						gaPrint( "%5ld: global creation from two (%d, %d) fittest (%4lu, %4lu) %4ld\n", fStep, parent1, parent2, fFittest->get(parent1)->agentID, fFittest->get(parent2)->agentID, fNumberCreated2Fit );
					}
					else
					{
						// by iterating through the array of fittest
						newAgent->Genes()->crossover( fFittest->get(fFitI)->genes, fFittest->get(fFitJ)->genes, true );
						fNumberCreated2Fit++;
						gaPrint( "%5ld: global creation from two (%d, %d) fittest (%4lu, %4lu) %4ld\n", fStep, fFitI, fFitJ, fFittest->get(fFitI)->agentID, fFittest->get(fFitJ)->agentID, fNumberCreated2Fit );
						ijfitinc( fFittest->size(), &fFitI, &fFitJ );
                    }
                }
                else
                {
                    // otherwise, just generate a random, hopeful monster
                    newAgent->Genes()->randomize();
                    fNumberCreatedRandom++;
					gaPrint( "%5ld: global creation random (%4ld)\n", fStep, fNumberCreatedRandom );
                }
            }
            else
            {
                // otherwise, just generate a random, hopeful monster
                newAgent->Genes()->randomize();
                fNumberCreatedRandom++;
				gaPrint( "%5ld: global creation random early (%4ld)\n", fStep, fNumberCreatedRandom );
            }

			newAgent->setGenomeReady();

            newAgent->grow( fMateWait );
			FoodEnergyIn( newAgent->GetFoodEnergy() );

            newAgent->settranslation(randpw() * globals::worldsize, 0.5 * agent::config.agentHeight, randpw() * -globals::worldsize);
            newAgent->setyaw(randpw() * 360.0);
            id = WhichDomain(newAgent->x(), newAgent->z(), 0);
            newAgent->Domain(id);
            fDomains[id].numcreated++;
            fDomains[id].lastcreate = fStep;
            fDomains[id].numAgents++;
            fStage.AddObject(newAgent);
	    	objectxsortedlist::gXSortedObjects.add(newAgent); // add new agent to list of all objejcts; the newAgent->listLink that gets auto stored here should be valid immediately
	    	fNewLifes++;
            //newAgents.add(newAgent); // add it to the full list later; the e->listLink that gets auto stored here must be replaced with one from full list below

			Birth( newAgent, LifeSpan::BR_CREATE );
        }

        debugcheck( "after global agent creations" );
    }

    debugcheck( "after all agent creations" );
}

//---------------------------------------------------------------------------
// TSimulation::MaintainBricks
//---------------------------------------------------------------------------
void TSimulation::MaintainBricks()
{
	// Update on/off state of brick patches
	for( int domain = 0; domain < fNumDomains; domain++ )
	{
		for( int patch = 0; patch < fDomains[domain].numBrickPatches; patch++ )
		{
			fDomains[domain].fBrickPatches[patch].updateOn();
		}
	}
}

//---------------------------------------------------------------------------
// TSimulation::MaintainFood
//---------------------------------------------------------------------------
void TSimulation::MaintainFood()
{
	// Remove any food that has exceeded its lifespan
	if( food::gMaxLifeSpan > 0 )
	{
		while( !food::gAllFood.empty() )
		{
			// gAllFood is ordered by creation, so when we've encountered
			// a piece that is too young to be removed, we can stop looking.
			// Removing food takes it out of the "all food" list, so we can
			// look at the head of the list on each iteration.
			food *f = food::gAllFood.front();
			if( (f == NULL) || (f->getAge(fStep) < food::gMaxLifeSpan) )
			{
				break;
			}

			objectxsortedlist::gXSortedObjects.setcurr( f->GetListLink() );
			RemoveFood( f );
		}

		objectxsortedlist::gXSortedObjects.reset();
	}

	// Go through each of the food patches and bring them up to minFoodCount size
	// and create a new piece based on the foodRate probability
	if( (long)objectxsortedlist::gXSortedObjects.getCount(FOODTYPE) < fMaxFoodCount )
	{
		for( int domainNumber = 0; domainNumber < fNumDomains; domainNumber++ )
		{
			// If there are any dynamic food patches that have not yet had their initial growth,
			// see if they're ready to grow now
			if( fDomains[domainNumber].numFoodPatchesGrown < fDomains[domainNumber].numFoodPatches )
			{
				// If there are dynamic food patches that have not yet had their initial growth,
				// and they are now active, perform the initial growth now
				for( int patchNumber = 0; patchNumber < fDomains[domainNumber].numFoodPatches; patchNumber++ )
				{
					if( !fDomains[domainNumber].fFoodPatches[patchNumber].initFoodGrown() && fDomains[domainNumber].fFoodPatches[patchNumber].isOn() )
					{
						for( int j = 0; j < fDomains[domainNumber].fFoodPatches[patchNumber].initFoodCount; j++ )
						{
							if( fDomains[domainNumber].foodCount < fDomains[domainNumber].maxFoodCount )
							{
								AddFood( domainNumber, patchNumber );
							}
						}
						fDomains[domainNumber].fFoodPatches[patchNumber].initFoodGrown( true );
						fDomains[domainNumber].numFoodPatchesGrown++;
					}
				}
			}

			if( fUseProbabilisticFoodPatches )
			{
				if( fDomains[domainNumber].foodCount < fDomains[domainNumber].maxFoodGrownCount )
				{
					// Grow by a probability based on the decimal part of the foodRate of the domain
					float probAdd;
					if( fFoodGrowthModel == MaxRelative )
						probAdd = (fDomains[domainNumber].maxFoodGrownCount - fDomains[domainNumber].foodCount) * (fDomains[domainNumber].foodRate - floor(fDomains[domainNumber].foodRate));
					else
						probAdd = fDomains[domainNumber].foodRate - floor(fDomains[domainNumber].foodRate);
					if( randpw() < probAdd )
					{
						// Add food to a patch in the domain (chosen based on the patch's fraction)
						int patchNumber = getRandomPatch( domainNumber );
						if( patchNumber >= 0 )
						{
							AddFood( domainNumber, patchNumber );
						}
					}

					// Grow by the integer part of the foodRate of the domain
					int foodToGrow = (int)fDomains[domainNumber].foodRate;
					for( int i = 0; i < foodToGrow; i++ )
					{
						int patchNumber = getRandomPatch( domainNumber );
						if( patchNumber >= 0 )
						{
							AddFood( domainNumber, patchNumber );
						}
						else
							break;	// no active patches in this domain, so give up
					}

					// Keep at least the minimum amount of food around
					int newFood = fDomains[domainNumber].minFoodCount - fDomains[domainNumber].foodCount;
					for( int i = 0; i < newFood; i++ )
					{
						int patchNumber = getRandomPatch(domainNumber);
						if( patchNumber >= 0 )
						{
							AddFood( domainNumber, patchNumber );
						}
						else
							break;	// no active patches in this domain, so give up
					}
				}
			}
			else
			{
				for( int patchNumber = 0; patchNumber < fDomains[domainNumber].numFoodPatches; patchNumber++ )
				{
					if( fDomains[domainNumber].fFoodPatches[patchNumber].isOn() )
					{
						if( fDomains[domainNumber].fFoodPatches[patchNumber].foodCount < fDomains[domainNumber].fFoodPatches[patchNumber].maxFoodGrownCount )
						{
							// Grow by a probability based on the decimal part of the growthRate of the patch
							float probAdd;
							if( fFoodGrowthModel == MaxRelative )
								probAdd = (fDomains[domainNumber].fFoodPatches[patchNumber].maxFoodGrownCount - fDomains[domainNumber].fFoodPatches[patchNumber].foodCount) * (fDomains[domainNumber].fFoodPatches[patchNumber].growthRate - floor(fDomains[domainNumber].fFoodPatches[patchNumber].growthRate));
							else
								probAdd = fDomains[domainNumber].fFoodPatches[patchNumber].growthRate - floor(fDomains[domainNumber].fFoodPatches[patchNumber].growthRate);
							if( randpw() < probAdd )
							{
								AddFood( domainNumber, patchNumber );
							}

							// Grow by the integer part of the growthRate
							int foodToGrow = (int) fDomains[domainNumber].fFoodPatches[patchNumber].growthRate;
							for( int i = 0; i < foodToGrow; i++ )
							{
								AddFood( domainNumber, patchNumber );
							}

							// Keep at least the minimum amount of food around
							int newFood = fDomains[domainNumber].fFoodPatches[patchNumber].minFoodCount - fDomains[domainNumber].fFoodPatches[patchNumber].foodCount;
							for( int i = 0; i < newFood; i++ )
							{
								AddFood( domainNumber, patchNumber );
							}
						}
					}
				}
			}
		}
	}

	// If any dynamic food patches destroy their food when turned off, take care of it here
	if( fFoodRemovalNeeded )
	{
		// Get a list of patches needing food removal currently
		int numPatchesNeedingRemoval = 0;
		for( int domain = 0; domain < fNumDomains; domain++ )
		{
			for( int ipatch = 0; ipatch < fDomains[domain].numFoodPatches; ipatch++ )
			{
				FoodPatch &patch = fDomains[domain].fFoodPatches[ipatch];
				// If the patch is on now, but off in the next time step, and it is supposed to remove its food when it is off...
				if( patch.removeFood && !patch.isOn() && patch.isOnChanged() )
				{
					fFoodPatchesNeedingRemoval[numPatchesNeedingRemoval++] = &patch;
				}
			}
		}

		if( numPatchesNeedingRemoval > 0 )
		{
			food* f;

			// There are patches currently needing removal, so do it
			objectxsortedlist::gXSortedObjects.reset();
			while( objectxsortedlist::gXSortedObjects.nextObj( FOODTYPE, (gobject**) &f ) )
			{
				for( int i = 0; i < numPatchesNeedingRemoval; i++ )
				{
					if( f->getPatch() == fFoodPatchesNeedingRemoval[i] )
					{
						RemoveFood( f );

						break;	// found patch and deleted food, so get out of patch loop
					}
				}
			}
		}
	}

	for( int domain = 0; domain < fNumDomains; domain++ )
	{
		for( int patch = 0; patch < fDomains[domain].numFoodPatches; patch++ )
		{
			fDomains[domain].fFoodPatches[patch].endStep();
		}
	}

    debugcheck( "after growing food and maintaining food patch stats" );
}

//---------------------------------------------------------------------------
// TSimulation::ijfitinc
//---------------------------------------------------------------------------
void TSimulation::ijfitinc(short n, short* i, short* j)
{
    (*j)++;

    if ((*j) == (*i))
        (*j)++;

    if ((*j) > n - 1)
    {
        (*j) = 0;
        (*i)++;

        if ((*i) > n - 1)
		{
            (*i) = 0;	// min(1, n - 1);
			(*j) = 1;
		}
    }
}


//---------------------------------------------------------------------------
// TSimulation::Birth
//---------------------------------------------------------------------------
void TSimulation::Birth( agent* a,
						 LifeSpan::BirthReason reason,
						 agent* a_parent1,
						 agent* a_parent2 )
{
	AgentBirthEvent birthEvent( a, reason, a_parent1, a_parent2 );

	// ---
	// --- Update Logs
	// ---
	logs->postEvent( birthEvent );

	if( a )	// a will NULL for virtual births only
	{
		fNumberAlive++;
		fNumberAliveWithMetabolism[ a->GetMetabolism()->index ]++;

		// ---
		// --- Update agent's LifeSpan
		// ---
		a->GetLifeSpan()->set_birth( fStep,
									 reason );

		// ---
		// --- Create Separation Cache Entry
		// ---
		SeparationCache::birth( birthEvent );
	}

	// ---
	// --- Update Events
	// ---
	if( fEvents && (reason != LifeSpan::BR_SIMINIT) )
	{
		switch( reason )
		{
		case LifeSpan::BR_NATURAL:
		case LifeSpan::BR_LOCKSTEP:
			fEvents->AddEvent( fStep, a_parent1->Number(), 'm' );
			fEvents->AddEvent( fStep, a_parent2->Number(), 'm' );
			break;
		case LifeSpan::BR_VIRTUAL:
			fEvents->AddEvent( fStep, a_parent1->Number(), 'm' );
			fEvents->AddEvent( fStep, a_parent2->Number(), 'm' );
			break;
		case LifeSpan::BR_CREATE:
			// no-op
			break;
		default:
			assert( false );
		}
	}
}

//---------------------------------------------------------------------------
// TSimulation::Kill
//
// Perform all actions needed to agent before list removal and deletion
//---------------------------------------------------------------------------
void TSimulation::Kill( agent* c,
						LifeSpan::DeathReason reason )
{
	AgentDeathEvent deathEvent(c, reason);

	fNumberAlive--;
	fNumberAliveWithMetabolism[c->GetMetabolism()->index]--;

	// ---
	// --- Update LifeSpan
	// ---
	c->GetLifeSpan()->set_death( fStep,
								 reason );

	// ---
	// --- Simulation Ending
	// ---
	if( reason == LifeSpan::DR_SIMEND )
	{
		logs->postEvent( deathEvent );
		SeparationCache::death( deathEvent );
		c->Die();

		return;
	}

	const short id = c->Domain();

	ttPrint( "age %ld: agent # %ld has died\n", fStep, c->Number() );

	// ---
	// --- Update Tallies
	// ---
    fNewDeaths++;
    fNumberDied++;
    fDomains[id].numdied++;
    fDomains[id].numAgents--;

	// ---
	// --- Update Stats
	// ---
	fLifeSpanStats.add( c->Age() );
	fLifeSpanRecentStats.add( c->Age() );
	fLifeFractionRecentStats.add( c->Age() / c->MaxAge() );

	// ---
	// --- Update Fitness
	// ---
	// Make any final contributions to the agent's overall, lifetime fitness
    c->lastrewards(fEnergyFitnessParameter, fAgeFitnessParameter); // if any

	// ---
	// --- Turn into food, if applicable
	// ---
	FoodPatch* fp;

	bool rFood = (fAgentsRfood == RFOOD_TRUE)
		|| ( (fAgentsRfood == RFOOD_TRUE__FIGHT_ONLY) && (reason == LifeSpan::DR_FIGHT) );

	// If agent's carcass is to become food, make it so here
    if ( rFood
    	&& ((long)objectxsortedlist::gXSortedObjects.getCount(FOODTYPE) < fMaxFoodCount)
		&& (fDomains[id].foodCount < fDomains[id].maxFoodCount)	// ??? Matt had commented this out; why?
		&& ((fp = fDomains[id].whichFoodPatch( c->x(), c->z() )) && (fp->foodCount < fp->maxFoodCount))	// ??? Matt had nothing like this here; why?
    	&& (globals::blockedEdges || (c->x() >= 0.0 && c->x() <=  globals::worldsize &&
    	                              c->z() <= 0.0 && c->z() >= -globals::worldsize)) )
    {
		const FoodType *carcassFoodType = c->GetMetabolism()->carcassFoodType;
		if( carcassFoodType )
		{
			// etodo: we probably want a way to move nutrients around...
			// like map energy[0] of agent to energy[1] of food.

			Energy foodEnergy = c->GetFoodEnergy();
			// Multiply by polarity^2 so that we have values > 0 and so that any polarity of UNDEFINED gets zeroed.
			Energy minFoodEnergyAtDeath = Energy(fMinFoodEnergyAtDeath) * carcassFoodType->energyPolarity * carcassFoodType->energyPolarity;
			Energy minFoodEnergy = Energy(food::gMinFoodEnergy) * carcassFoodType->energyPolarity * carcassFoodType->energyPolarity;

			if( foodEnergy.isDepleted(minFoodEnergyAtDeath) )
			{
				if( !minFoodEnergyAtDeath.isDepleted(minFoodEnergy) )
				{
					Energy addedEnergy;
					foodEnergy.constrain( minFoodEnergyAtDeath, food::gMaxFoodEnergy, addedEnergy );
					FoodEnergyIn( addedEnergy * -1 );
				}
			}

			if( foodEnergy.isDepleted() || (!fSkipCarcassMinFoodEnergyCheck && foodEnergy.isDepleted(minFoodEnergy)) )
			{
				FoodEnergyOut( foodEnergy );
			}
			else
			{
				food* f = new food( carcassFoodType, fStep, foodEnergy, c->x(), c->z() );
				gdlink<gobject*> *saveCurr = objectxsortedlist::gXSortedObjects.getcurr();
				objectxsortedlist::gXSortedObjects.add( f );	// dead agent becomes food
				objectxsortedlist::gXSortedObjects.setcurr( saveCurr );
				fStage.AddObject( f );			// put replacement food into the world
				if( fp )
				{
					fp->foodCount++;
					f->setPatch( fp );
				}
				else
					fprintf( stderr, "food created with no affiliated FoodPatch\n" );
				f->domain( id );
				fDomains[id].foodCount++;
			}
		}
    }
    else
    {
		FoodEnergyOut( c->GetFoodEnergy() );
    }

	// ---
	// --- Die()
	// ---
	// Must call Die() for the agent before any of the uses of Fitness() below, so we get the final, true, post-death fitness
	logs->postEvent( deathEvent );
	SeparationCache::death( deathEvent );
	c->Die();

	// ---
	// --- Remove From Stage
	// ---
    fStage.RemoveObject( c );

	// ---
	// --- x-sorted list
	// ---
	// following assumes (requires!) list to be currently pointing to c,
    // and will leave the list pointing to the previous agent
	// agent::config.xSortedAgents.remove(); // get agent out of the list
	// objectxsortedlist::gXSortedObjects.removeCurrentObject(); // get agent out of the list

	// Following assumes (requires!) the agent to have stored c->listLink correctly
	objectxsortedlist::gXSortedObjects.removeObjectWithLink( (gobject*) c );

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// !!! POST PARALLEL
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    fScheduler.postParallel( [=]() {
            analyzeBrain( c );
        });

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// !!! POST SERIAL
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    fScheduler.postSerial( [=]() {
            updateFittest( c );

            // Note: For the sake of computational efficiency, I used to never delete an agent,
            // but "reinit" and reuse them as new agents were born or created.  But Gene made
            // agents be allocated afresh on birth or creation, so we now need to delete the
            // old ones here when they die.  Remove this if I ever get a chance to go back to the
            // more efficient reinit and reuse technique.
            delete c;
        });
}

//---------------------------------------------------------------------------
// TSimulation::analyzeBrain
//
// Analyze brain of dead agent.
//---------------------------------------------------------------------------
void TSimulation::analyzeBrain( agent *c )
{
	logs->postEvent( BrainAnalysisBeginEvent(c) );

	if ( fCalcComplexity )
	{
		// This should have been configured in response to the begin event.
		const char *brainFunctionPath = c->brainAnalysisParms.functionPath.c_str();
		// Fail if it's zero length.
		assert( *brainFunctionPath );

		if( fComplexityType == "D" )	// special case the difference of complexities case
		{
			float pComplexity = CalcComplexity_brainfunction( brainFunctionPath, "P" );
			float iComplexity = CalcComplexity_brainfunction( brainFunctionPath, "I" );
			c->SetComplexity( pComplexity - iComplexity );
		}
		else if( fComplexityType != "Z" )	// avoid special hack case to evolve towards zero max velocity, for testing purposes only
		{
			// otherwise, fComplexityType has the right string in it
			c->SetComplexity( CalcComplexity_brainfunction( brainFunctionPath, fComplexityType.c_str(), fEvents ) );
		}
	}

	logs->postEvent( BrainAnalysisEndEvent(c) );
}

//---------------------------------------------------------------------------
// TSimulation::updateFittest
//
// Update lists of fittest agents to contain recently died agent, if needed.
//---------------------------------------------------------------------------
void TSimulation::updateFittest( agent *c )
{
	const short id = c->Domain();

	// Maintain the current-fittest list based on heuristic fitness
	if( (fCurrentFittestCount > 0) && (c->HeuristicFitness() >= fCurrentMaxFitness[fCurrentFittestCount-1]) )	// a current-fittest agent is dying
	{
		int haveFitAgent = 0;
		for( int i = 0; i < fCurrentFittestCount; i++ )
		{
			if( fCurrentFittestAgent[i] != c )
				fCurrentFittestAgent[ (i-haveFitAgent) ] = fCurrentFittestAgent[i];
			else
				haveFitAgent = 1;
		}

		if( haveFitAgent == 1 )		// this should usually be true, but lets make sure.
		{
			fCurrentFittestAgent[ (fCurrentFittestCount-1) ] = NULL;	// Null out the last agent in the list, just to be polite
			fCurrentFittestCount--;		// decrement the number of agents in the list now that we've removed the recently deceased agent (c) from it.
		}
	}

	// Maintain a list of the fittest agents ever, for use in the online/steady-state GA,
	// based on complete fitness, however it is currently being calculated
	float cFitness = AgentFitness( c );

	fMaxFitness = max( cFitness, fMaxFitness );

	// First on a domain-by-domain basis...
	if( fDomains[id].fittest )
		fDomains[id].fittest->update( c, cFitness );

	// Then on a whole-world basis...
	fFittest->update( c, cFitness );

	// Keep a separate list of the recent fittest, purely for data-gathering purposes,
	// also based on complete fitness, however it is being currently being calculated
	// "Recent" means since the last archival recording of recent best, as determined by fBestRecentBrainAnatomyRecordFrequency
	// (Don't bother, if we're not gathering that kind of data)
	if( fRecentFittest )
		fRecentFittest->update( c, cFitness );

	// Must also update the leastFit data structures, now that they
	// are used on-demand in the main mate/fight/eat loop in Interact()
	// As these are used during the agent's life, they must be based on the heuristic fitness function.
	// Update the domain-specific leastFit list (only one, as we know which domain it's in)
	for( int i = 0; i < fDomains[id].fNumLeastFit; i++ )
	{
		if( fDomains[id].fLeastFit[i] != c )	// not one of our least fit, so just loop again to keep searching
			continue;

		// one of our least-fit agents died, so pull in the list over it
		smPrint( "removing agent %ld from the least fit list for domain %d at position %d with fitness %g (because it died)\n", c->Number(), id, i, c->HeuristicFitness() );
		for( int j = i; j < fDomains[id].fNumLeastFit-1; j++ )
			fDomains[id].fLeastFit[j] = fDomains[id].fLeastFit[j+1];
		fDomains[id].fNumLeastFit--;
		break;	// c can only appear once in the list, so we're done
	}
}

//-------------------------------------------------------------------------------------------
// TSimulation::AddFood
//-------------------------------------------------------------------------------------------
void TSimulation::AddFood( long domainNumber, long patchNumber )
{
	food *f = fDomains[domainNumber].fFoodPatches[patchNumber].addFood( fStep );
	if( f != NULL )
	{
		fDomains[domainNumber].foodCount++;
		FoodEnergyIn( f->getEnergy() );
	}
}

//-------------------------------------------------------------------------------------------
// TSimulation::RemoveFood
//-------------------------------------------------------------------------------------------
void TSimulation::RemoveFood( food *f )
{
	FoodPatch *fp = f->getPatch();
	if( fp ) fp->foodCount--;

	int domain = f->domain();
	assert( domain >= 0 && domain < fNumDomains );
	fDomains[f->domain()].foodCount--;

	assert( f == objectxsortedlist::gXSortedObjects.getcurr()->e );
	objectxsortedlist::gXSortedObjects.removeCurrentObject();   // get it out of the list

	fStage.RemoveObject( f );  // get it out of the world

	if( f->BeingCarried() )
	{
		// if it's being carried, have to remove it from the carrier
		((agent*)(f->CarriedBy()))->DropObject( (gobject*) f );
	}

	FoodEnergyOut( f->getEnergy() );

	delete f;	// get it out of memory
}

//-------------------------------------------------------------------------------------------
// TSimulation::FoodEnergyIn
//-------------------------------------------------------------------------------------------
void TSimulation::FoodEnergyIn( const Energy &e ) {
	static bool warn = true;
	if( warn ) {
		warn = false;

		if( (globals::numEnergyTypes > 1) || (FoodType::getNumberDefinitions() > 1) ) {
			fprintf( stderr, "WARNING! FoodEnergyIn/Out too simplistic for tracking this simulation!\n" );
		}
	}
	fFoodEnergyIn += e[0];
}

//-------------------------------------------------------------------------------------------
// TSimulation::FoodEnergyOut
//-------------------------------------------------------------------------------------------
void TSimulation::FoodEnergyOut( const Energy &e ) {
	fFoodEnergyOut += e[0];
}

//-------------------------------------------------------------------------------------------
// TSimulation::AgentFitness
//-------------------------------------------------------------------------------------------
float TSimulation::AgentFitness( agent* c )
{
// 	printf( "%s (beginning): %ld %g\n", __func__, c->Number(), c->Complexity() );

	float fitness = 0.0;

	if( c->Alive() )
	{
		cerr << "Error: Simulation's AgentFitness(agent) function called while agent is still alive" nl;
		exit(1);
	}

	if( fComplexityFitnessWeight == 0.0 )	// complexity as fitness is turned off
	{
		fitness = c->HeuristicFitness() / fTotalHeuristicFitness;
	}
	else if( fComplexityType == "Z" )	// hack to evolve towards zero velocity, for testing purposes only
	{
		fitness = 0.01 / (c->MaxSpeed() + 0.01);
	}
	else	// we are using complexity as a fitness function (and may be using heuristic fitness too)
	{
		if( c->Complexity() < 0.0 )
		{
			fprintf( stderr, "********** complexity being calculated when it should already be known **********\n" );
			char filename[256];
			sprintf( filename, "run/brain/function/brainFunction_%ld.txt", c->Number() );
			if( fComplexityType == "D" )	// difference between I and P complexity being used for fitness
			{
				float pComplexity = CalcComplexity_brainfunction( filename, "P" );
				float iComplexity = CalcComplexity_brainfunction( filename, "I" );
				c->SetComplexity( pComplexity - iComplexity );
			}
			else	// fComplexityType contains the appropriate string to select the type of complexity
				c->SetComplexity( CalcComplexity_brainfunction( filename, fComplexityType.c_str(), fEvents ) );
		}
		// fitness is normalized (by the sum of the weights) after doing a weighted sum of normalized heuristic fitness and complexity
		// (Complexity runs between 0.0 and 1.0 in the early simulations.  Is there a way to guarantee this?  Do we want to?)
		fitness = (fHeuristicFitnessWeight*c->HeuristicFitness()/fTotalHeuristicFitness + fComplexityFitnessWeight*c->Complexity()) / (fHeuristicFitnessWeight+fComplexityFitnessWeight);
// 		cout << "fitness" eql fitness sp "hwt" eql fHeuristicFitnessWeight sp "hf" eql c->HeuristicFitness()/fTotalHeuristicFitness sp "cwt" eql fComplexityFitnessWeight sp "cf" eql c->Complexity() nl;
	}

// 	printf( "%s (end): %ld %g (%g)\n", __func__, c->Number(), c->Complexity(), fitness );

	return( fitness );
}

//-------------------------------------------------------------------------------------------
// TSimulation::processWorldFile
//-------------------------------------------------------------------------------------------
void TSimulation::processWorldFile( proplib::Document *docWorldFile )
{
	proplib::Document &doc = *docWorldFile;

	fLockStepWithBirthsDeathsLog = doc.get( "PassiveLockstep" );
	fMaxSteps = doc.get( "MaxSteps" );
	fEndOnPopulationCrash = doc.get( "EndOnPopulationCrash" );
	fDumpFrequency = doc.get( "CheckPointFrequency" );
	{
		string prop = doc.get( "Edges" );
		if( prop == "B" )
		{
			globals::blockedEdges = true;
			globals::wraparound = false;
			globals::stickyEdges = false;
		}
		else if( prop == "W" )
		{
			globals::blockedEdges = false;
			globals::wraparound = true;
			globals::stickyEdges = false;
		}
		else if( prop == "T" )
		{
			globals::blockedEdges = false;
			globals::wraparound = false;
			globals::stickyEdges = false;
		}
		else if( prop == "S" )
		{
			globals::blockedEdges = true;
			globals::wraparound = false;
			globals::stickyEdges = true;
		}
		else
			assert( false );
	}
	globals::numEnergyTypes = doc.get( "NumEnergyTypes" );
	fStaticTimestepGeometry = doc.get( "StaticTimestepGeometry" );
	if( fStaticTimestepGeometry )
	{
		// Brains execute in parallel, so we need brain-local RNG state
		RandomNumberGenerator::set( RandomNumberGenerator::NERVOUS_SYSTEM,
									RandomNumberGenerator::LOCAL );
	}
	fParallelInitAgents = doc.get( "ParallelInitAgents" );
	fParallelInteract = doc.get( "ParallelInteract" );
	fParallelCreateAgents = doc.get( "ParallelCreateAgents" );
	fParallelBrains = doc.get( "ParallelBrains" );
	fMinNumAgents = doc.get( "MinAgents" );
	fMaxNumAgents = doc.get( "MaxAgents" );
	fInitNumAgents = doc.get( "InitAgents" );
	fNumberToSeed = doc.get( "SeedAgents" );
	fProbabilityOfMutatingSeeds = doc.get( "SeedMutationProbability" );
	fSeedFromFile = doc.get( "SeedGenomeFromRun" );
	fPositionSeedsFromFile = doc.get( "SeedPositionFromRun" );
    fMiscAgents = doc.get( "MiscegenationDelay" );
	fInitFoodCount = doc.get( "InitFood" );
	fMinFoodCount = doc.get( "MinFood" );
	fMaxFoodCount = doc.get( "MaxFood" );
	fMaxFoodGrownCount = doc.get( "MaxFoodGrown" );
	fFoodRate = doc.get( "FoodGrowthRate" );
	{
		string foodGrowthModel = (string) doc.get( "FoodGrowthModel" );
		if( foodGrowthModel == "MaxIndependent" )
			fFoodGrowthModel = MaxIndependent;
		else
			fFoodGrowthModel = MaxRelative;
	}
	fFoodRemoveEnergy = doc.get( "FoodRemoveEnergy" );
	fFoodRemoveFirstEat = doc.get( "FoodRemoveFirstEat" );
	food::gMaxLifeSpan = doc.get( "FoodMaxLifeSpan" );
    fPositionSeed = doc.get( "PositionSeed" );
    fGenomeSeed = doc.get( "InitSeed" );
	fSimulationSeed = doc.get( "SimulationSeed" );
	{
		proplib::Property &rfood = doc.get( "AgentsAreFood" );
		if( (string)rfood == "Fight" )
			fAgentsRfood = RFOOD_TRUE__FIGHT_ONLY;
		else if( (bool)rfood )
			fAgentsRfood = RFOOD_TRUE;
		else
			fAgentsRfood = RFOOD_FALSE;
	}
	fFitness1Frequency = doc.get( "EliteFrequency" );
    fFitness2Frequency = doc.get( "PairFrequency" );
	fEpochFrequency = doc.get( "EpochFrequency" );
	fEpoch = fEpochFrequency;
	{
		int numberFittest = doc.get( "NumberFittest" );
		if( numberFittest > 0 )
			fFittest = new FittestList( numberFittest, true );
		else
			fFittest = NULL;
	}
	{
		int numberRecentFittest = doc.get( "NumberRecentFittest" );
		if( (numberRecentFittest > 0) && (fEpochFrequency > 0) )
			fRecentFittest = new FittestList( numberRecentFittest, false );
		else
			fRecentFittest = NULL;
	}
    fEatFitnessParameter = doc.get( "FitnessWeightEating" );
    fMateFitnessParameter = doc.get( "FitnessWeightMating" );
    fMoveFitnessParameter = doc.get( "FitnessWeightMoving" );
    fEnergyFitnessParameter = doc.get( "FitnessWeightEnergyAtDeath" );
    fAgeFitnessParameter = doc.get( "FitnessWeightLongevity" );
  	fTotalHeuristicFitness = fEatFitnessParameter + fMateFitnessParameter + fMoveFitnessParameter + fEnergyFitnessParameter + fAgeFitnessParameter;
	food::gMinFoodEnergy = doc.get( "MinFoodEnergy" );
    food::gMaxFoodEnergy = doc.get( "MaxFoodEnergy" );
	food::gSize2Energy = doc.get( "FoodEnergySizeScale" );
	fEat2Consume = doc.get( "FoodConsumptionRate" );
	{
		fCarryObjects = 0;
#define __SET( PROP, MASK ) if( (bool)doc.get("Carry" PROP) ) fCarryObjects |= MASK##TYPE
		__SET( "Agents", AGENT );
		__SET( "Food", FOOD );
		__SET( "Bricks", BRICK );
#undef __SET
	}
	{
		fShieldObjects = 0;
#define __SET( PROP, MASK ) if( (bool)doc.get("Shield" PROP) ) fShieldObjects |= MASK##TYPE
		__SET( "Agents", AGENT );
		__SET( "Food", FOOD );
		__SET( "Bricks", BRICK );
#undef __SET
	}
	fCarryPreventsEat = doc.get( "CarryPreventsEat" );
	fCarryPreventsFight = doc.get( "CarryPreventsFight" );
	fCarryPreventsGive = doc.get( "CarryPreventsGive" );
	fCarryPreventsMate = doc.get( "CarryPreventsMate" );

	fEatWait = doc.get( "EatWait" );
	fProbabilisticMating = doc.get( "ProbabilisticMating" );
    fMateWait = doc.get( "MateWait" );
	fEatMateSpan = doc.get( "EatMateWait" );
	fEatMateMinDistance = doc.get( "EatMateMinDistance" );
	fMaxMateVelocity = doc.get( "MaxMateVelocity" );
	fMinEatVelocity = doc.get( "MinEatVelocity" );
	fMaxEatVelocity = doc.get( "MaxEatVelocity" );
	fMaxEatYaw = doc.get( "MaxEatYaw" );

    fMinMateFraction = doc.get( "MinMateEnergyFraction" );
    fPower2Energy = doc.get( "DamageRate" );
	food::gCarryFood2Energy = doc.get( "EnergyUseCarryFood" );
	brick::gCarryBrick2Energy = doc.get( "EnergyUseCarryBrick" );
	{
		fAgentHealingRate = doc.get( "AgentHealingRate" );
		// a bool flag to check to see if healing is turned on is faster than always comparing a float.
		fHealing = fAgentHealingRate > 0.0;
	}
    fEatThreshold = doc.get( "EatThreshold" );
    fMateThreshold = doc.get( "MateThreshold" );
    fFightThreshold = doc.get( "FightThreshold" );
	fFightFraction = doc.get( "FightMultiplier" );
	{
		string fightMode = doc.get( "FightMode" );
		if( fightMode == "Normal" )
			fFightMode = FM_NORMAL;
		else if( fightMode == "Null" )
			fFightMode = FM_NULL;
		else
			assert(false);
	}
	fGiveThreshold = doc.get( "GiveThreshold" );
	fGiveFraction = doc.get( "GiveFraction" );
	fPickupThreshold = doc.get( "PickupThreshold" );
	fDropThreshold = doc.get( "DropThreshold" );
	{
		fSolidObjects = 0;
#define __SET( PROP, MASK ) if( (bool)doc.get("Solid" PROP) ) fSolidObjects |= MASK##TYPE
		__SET( "Agents", AGENT );
		__SET( "Food", FOOD );
		__SET( "Bricks", BRICK );
#undef __SET
	}
    food::gFoodHeight = doc.get( "FoodHeight" );
    food::gFoodColor = doc.get( "FoodColor" );
	brick::gBrickHeight = doc.get( "BrickHeight" );
    barrier::gBarrierHeight = doc.get( "BarrierHeight" );
    barrier::gBarrierColor = doc.get( "BarrierColor" );
	barrier::gStickyBarriers = doc.get( "StickyBarriers" );
	barrier::gRatioPositions = doc.get( "RatioBarrierPositions" );
    fGroundColor = doc.get( "GroundColor" );
    fGroundClearance = doc.get( "GroundClearance" );
    globals::worldsize = doc.get( "WorldSize" );

	// ---
	// --- Barriers
	// ---
	{
		proplib::Property &propBarriers = doc.get( "Barriers" );

		for( int ibarrier = 0; ibarrier < (int)propBarriers.elements().size(); ibarrier++ )
		{
			proplib::Property &propBarrier = propBarriers.get( ibarrier );
			// Note that barriers were already allocated in InitCppProperties()
			barrier *b = new barrier();
			barrier::gBarriers.push_back( b );

			b->getPosition().xa = propBarrier.get( "X1" );
			b->getPosition().za = propBarrier.get( "Z1" );
			b->getPosition().xb = propBarrier.get( "X2" );
			b->getPosition().zb = propBarrier.get( "Z2" );

			b->init();

			barrier::gXSortedBarriers.add( b );
		}
	}

	globals::numEnergyTypes = doc.get( "NumEnergyTypes" );

	// Process FoodTypes
	{
		proplib::Property &propFoodTypes = doc.get( "FoodTypes" );
		int numFoodTypes = propFoodTypes.size();

		for( int ifoodType = 0; ifoodType < numFoodTypes; ifoodType++ )
		{
			proplib::Property &propFoodType = propFoodTypes.get( ifoodType );

			string name = propFoodType.get( "Name" );
			if( FoodType::lookup(name) != NULL )
			{
				propFoodType.err( "Duplicate name." );
			}

			Color foodColor;
			if( propFoodType.hasProperty("FoodColor") )
				foodColor = propFoodType.get( "FoodColor" );
			else
				foodColor = doc.get( "FoodColor" );
			EnergyPolarity energyPolarity = propFoodType.get( "EnergyPolarity" );
			Energy depletionThreshold = Energy::createDepletionThreshold( fFoodRemoveEnergy, energyPolarity );

			FoodType::define( name, foodColor, energyPolarity, depletionThreshold );
		}

	}

	// Process AgentMetabolisms
	{
		proplib::Property &propAgentMetabolisms = doc.get( "AgentMetabolisms" );
		int numMetabolisms = propAgentMetabolisms.size();

		// Init array tracking number of agents with a given metabolism
		assert( numMetabolisms <= MAXMETABOLISMS );


		for( int iMetabolism = 0; iMetabolism < numMetabolisms; iMetabolism++ )
		{
			proplib::Property &propMetabolism = propAgentMetabolisms.get( iMetabolism );

			string name = propMetabolism.get( "Name" );
			if( name == "Null" )
			{
				char tmp[128];
				sprintf( tmp, "Metabolism%d", iMetabolism );
				name = tmp;
			}

			EnergyPolarity energyPolarity = propMetabolism.get( "EnergyPolarity" );

			const FoodType *carcassFoodType;
			string carcassFoodTypeMode = propMetabolism.get( "CarcassFoodTypeMode" );

			if( carcassFoodTypeMode == "None" )
			{
				carcassFoodType = NULL;
			}
			else if( carcassFoodTypeMode == "FoodTypeName" )
			{
				string foodTypeName = propMetabolism.get( "CarcassFoodTypeName" );
				carcassFoodType = FoodType::lookup( foodTypeName );
				if( carcassFoodType == NULL )
				{
					propMetabolism.err( "CarcassFoodTypeName references unknown FoodType '" + foodTypeName + "'" );
				}
			}
			else if( carcassFoodTypeMode == "FindEnergyPolarity" )
			{
				carcassFoodType = FoodType::find( energyPolarity );
				if( carcassFoodType == NULL )
				{
					propMetabolism.err( "Cannot find FoodType with matching energy polarity." );
				}
			}
			else
			{
				assert( false );
			}

			EnergyMultiplier eatMultiplier = propMetabolism.get( "EatMultiplier" );
			Energy energyDelta = propMetabolism.get( "EnergyDelta" );
			float minEatAge = propMetabolism.get( "MinEatAge" );

			Metabolism::define( name, energyPolarity, eatMultiplier, energyDelta, minEatAge, carcassFoodType );
		}

		for( int i = 0; i < Metabolism::getNumberOfDefinitions(); i++ )
		{
			fMinNumAgentsWithMetabolism[i] = (int)doc.get("MinAgents") / Metabolism::getNumberOfDefinitions();
		}
	}

	// AgentMetabolismsSelectionMode
	{
		string mode = doc.get( "AgentMetabolismSelectionMode" );
		if( mode == "Gene" )
			Metabolism::selectionMode = Metabolism::Gene;
		else if( mode == "Random" )
			Metabolism::selectionMode = Metabolism::Random;
		else
			assert( false );
	}

	// Process Domains
	{
		proplib::Property &propDomains = doc.get( "Domains" );
		fNumDomains = propDomains.size();

		long totmaxnumagents = 0;
		long totminnumagents = 0;
		int	numFoodPatchesNeedingRemoval = 0;

		for( int id = 0; id < fNumDomains; id++ )
		{
			proplib::Property &dom = propDomains.get( id );

			{
				fDomains[id].centerX = dom.get( "CenterX" );
				fDomains[id].centerZ = dom.get( "CenterZ" );
				fDomains[id].sizeX = dom.get( "SizeX" );
				fDomains[id].sizeZ = dom.get( "SizeZ" );

				fDomains[id].absoluteSizeX = globals::worldsize * fDomains[id].sizeX;
				fDomains[id].absoluteSizeZ = globals::worldsize * fDomains[id].sizeZ;

				fDomains[id].startX = fDomains[id].centerX * globals::worldsize - (fDomains[id].absoluteSizeX / 2.0);
				fDomains[id].startZ = - fDomains[id].centerZ * globals::worldsize - (fDomains[id].absoluteSizeZ / 2.0);

				fDomains[id].endX = fDomains[id].centerX * globals::worldsize + (fDomains[id].absoluteSizeX / 2.0);
				fDomains[id].endZ = - fDomains[id].centerZ * globals::worldsize + (fDomains[id].absoluteSizeZ / 2.0);

				// clean up for floating point precision a little
				if( fDomains[id].startX < 0.0006 )
					fDomains[id].startX = 0.0;
				if( fDomains[id].startZ > -0.0006 )
					fDomains[id].startZ = 0.0;
				if( fDomains[id].endX > globals::worldsize * 0.9994 )
					fDomains[id].endX = globals::worldsize;
				if( fDomains[id].endZ < -globals::worldsize * 0.9994 )
					fDomains[id].endZ = -globals::worldsize;
			}

			{
				float minAgentsFraction = dom.get( "MinAgentsFraction" );
				if( minAgentsFraction < 0.0 )
					minAgentsFraction = fDomains[id].sizeX * fDomains[id].sizeZ;
				fDomains[id].minNumAgents = nint( minAgentsFraction * fMinNumAgents );

				float maxAgentsFraction = dom.get( "MaxAgentsFraction" );
				if( maxAgentsFraction < 0.0 )
					maxAgentsFraction = fDomains[id].sizeX * fDomains[id].sizeZ;
				fDomains[id].maxNumAgents = nint( maxAgentsFraction * fMaxNumAgents );

				float initAgentsFraction = dom.get( "InitAgentsFraction" );
				if( initAgentsFraction < 0.0 )
					initAgentsFraction = fDomains[id].sizeX * fDomains[id].sizeZ;
				fDomains[id].initNumAgents = nint( initAgentsFraction * fInitNumAgents );

				float initSeedsFraction = dom.get( "InitSeedsFraction" );
				if( initSeedsFraction < 0.0 )
					initSeedsFraction = fDomains[id].sizeX * fDomains[id].sizeZ;
				fDomains[id].numberToSeed = nint( initSeedsFraction * fNumberToSeed );

				fDomains[id].probabilityOfMutatingSeeds = dom.get( "ProbabilityOfMutatingSeeds" );
				if( fDomains[id].probabilityOfMutatingSeeds < 0.0 )
					fDomains[id].probabilityOfMutatingSeeds = fProbabilityOfMutatingSeeds;

				float initFoodFraction = dom.get( "InitFoodFraction" );
				if( initFoodFraction >= 0.0 )
					fDomains[id].initFoodCount = nint( initFoodFraction * fInitFoodCount );
				else
					fDomains[id].initFoodCount = nint( fDomains[id].sizeX * fDomains[id].sizeZ * fInitFoodCount );

				float minFoodFraction = dom.get( "MinFoodFraction" );
				if( minFoodFraction >= 0.0 )
					fDomains[id].minFoodCount = nint( minFoodFraction * fMinFoodCount );
				else
					fDomains[id].minFoodCount = nint( fDomains[id].sizeX * fDomains[id].sizeZ * fMinFoodCount );

				float maxFoodFraction = dom.get( "MaxFoodFraction" );
				if( maxFoodFraction >= 0.0 )
					fDomains[id].maxFoodCount = nint( maxFoodFraction * fMaxFoodCount );
				else
					fDomains[id].maxFoodCount = nint( fDomains[id].sizeX * fDomains[id].sizeZ * fMaxFoodCount );
				float maxFoodGrownFraction = dom.get( "MaxFoodGrownFraction" );
				if( maxFoodGrownFraction >= 0.0 )
					fDomains[id].maxFoodGrownCount = nint( maxFoodGrownFraction * fMaxFoodGrownCount );
				else
					fDomains[id].maxFoodGrownCount = nint( fDomains[id].sizeX * fDomains[id].sizeZ * fMaxFoodGrownCount );
				fDomains[id].foodRate = dom.get( "FoodRate" );
				if( fDomains[id].foodRate < 0.0 )
					fDomains[id].foodRate = fFoodRate;

			}

			// Process InitAgentsPatch
			{
				proplib::Property &propPatch = dom.get( "InitAgentsPatch" );

				float centerX, centerZ;
				float sizeX, sizeZ;
				int shape, distribution;

				centerX = propPatch.get( "CenterX" );
				centerZ = propPatch.get( "CenterZ" );
				sizeX = propPatch.get( "SizeX" );
				sizeZ = propPatch.get( "SizeZ" );

				{
					string val = propPatch.get( "Shape" );
					if( val == "R" )
						shape = 0;
					else if( val == "E" )
						shape = 1;
					else
						assert( false );
				}
				{
					string val = propPatch.get( "Distribution" );
					if( val == "U" )
						distribution = 0;
					else if( val == "L" )
						distribution = 1;
					else if( val == "G" )
						distribution = 2;
					else
						assert( false );
				}

				fDomains[id].initAgentsPatch = new Patch();
				fDomains[id].initAgentsPatch->initBase(centerX, centerZ, sizeX, sizeZ, shape, distribution, 0.0, &fStage, &(fDomains[id]), id);
			}

			// Process FoodPatches
			{
				proplib::Property &propPatches = dom.get( "FoodPatches" );
				fDomains[id].numFoodPatches = propPatches.size();

				// Create an array of FoodPatches
				fDomains[id].fFoodPatches = new FoodPatch[fDomains[id].numFoodPatches];
				float patchFractionSpecified = 0.0;

				for (int i = 0; i < fDomains[id].numFoodPatches; i++)
				{
					proplib::Property &propPatch = propPatches.get( i );

					const FoodType *foodType;
					float foodFraction, foodRate;
					float centerX, centerZ;  // should be from 0.0 -> 1.0
					float sizeX, sizeZ;  // should be from 0.0 -> 1.0
					float nhsize;
					float initFoodFraction, minFoodFraction, maxFoodFraction, maxFoodGrownFraction;
					int initFood, minFood, maxFood, maxFoodGrown;
					int shape, distribution;
					bool removeFood;

					foodType = FoodType::lookup( propPatch.get("FoodTypeName") );
					if( foodType == NULL )
					{
						propPatch.get( "FoodTypeName" ).err( "Unknown FoodType name" );
					}

					centerX = propPatch.get( "CenterX" );
					centerZ = propPatch.get( "CenterZ" );
					sizeX = propPatch.get( "SizeX" );
					sizeZ = propPatch.get( "SizeZ" );

					foodFraction = propPatch.get( "FoodFraction" );
					if( foodFraction < 0.0 )
						foodFraction = sizeX * sizeZ;

					initFoodFraction = propPatch.get( "InitFoodFraction" );
					if( initFoodFraction < 0.0 )
						initFoodFraction = foodFraction;
					initFood = nint( initFoodFraction * fDomains[id].initFoodCount );

					minFoodFraction = propPatch.get( "MinFoodFraction" );
					if( minFoodFraction < 0.0 )
						minFoodFraction = foodFraction;
					minFood = nint( minFoodFraction * fDomains[id].minFoodCount );

					maxFoodFraction = propPatch.get( "MaxFoodFraction" );
					if( maxFoodFraction < 0.0 )
						maxFoodFraction = foodFraction;
					maxFood = nint( maxFoodFraction * fDomains[id].maxFoodCount );

					maxFoodGrownFraction = propPatch.get( "MaxFoodGrownFraction" );
					if( maxFoodGrownFraction < 0.0 )
						maxFoodGrownFraction = foodFraction;
					maxFoodGrown = nint( maxFoodGrownFraction * fDomains[id].maxFoodGrownCount );

					foodRate = propPatch.get( "FoodRate" );
					if (foodRate < 0.0)
						foodRate = fDomains[id].foodRate;

					{
						string val = propPatch.get( "Shape" );
						if( val == "R" )
							shape = 0;
						else if( val == "E" )
							shape = 1;
						else
							assert( false );
					}
					{
						string val = propPatch.get( "Distribution" );
						if( val == "U" )
							distribution = 0;
						else if( val == "L" )
							distribution = 1;
						else if( val == "G" )
							distribution = 2;
						else
							assert( false );
					}
					nhsize = propPatch.get( "NeighborhoodSize" );
					removeFood = propPatch.get( "RemoveFood" );
					if( removeFood )
						numFoodPatchesNeedingRemoval++;

					bool on = propPatch.get( "On" );

					fDomains[id].fFoodPatches[i].init(foodType, centerX, centerZ, sizeX, sizeZ, foodRate, initFood, minFood, maxFood, maxFoodGrown, foodFraction, shape, distribution, nhsize, on, removeFood, &fStage, &(fDomains[id]), id);

					patchFractionSpecified += foodFraction;

				}

				// If all the fractions are 0.0, then set fractions based on patch area
				if (patchFractionSpecified == 0.0)
				{
					float totalArea = 0.0;
					// First calculate the total area from all the patches
					for (int i=0; i<fDomains[id].numFoodPatches; i++)
					{
						totalArea += fDomains[id].fFoodPatches[i].getArea();
					}
					// Then calculate the fraction for each patch
					for (int i=0; i<fDomains[id].numFoodPatches; i++)
					{
						float newFraction = fDomains[id].fFoodPatches[i].getArea() / totalArea;
						fDomains[id].fFoodPatches[i].setInitCounts((int)(newFraction * fDomains[id].initFoodCount), (int)(newFraction * fDomains[id].minFoodCount), (int)(newFraction * fDomains[id].maxFoodCount), (int)(newFraction * fDomains[id].maxFoodGrownCount), newFraction);
						patchFractionSpecified += newFraction;
					}
				}

				// Make sure fractions add up to 1.0 (allowing a little slop for floating point precision)
				if( (patchFractionSpecified < 0.99999) || (patchFractionSpecified > 1.00001) )
				{
					printf( "Patch Fractions sum to %f, when they must sum to 1.0!\n", patchFractionSpecified );
					exit( 1 );
				}
			}

			{
				proplib::Property &propPatches = dom.get( "BrickPatches" );
				fDomains[id].numBrickPatches = propPatches.size();

				// Create an array of BrickPatches
				fDomains[id].fBrickPatches = new BrickPatch[fDomains[id].numBrickPatches];
				for (int i = 0; i < fDomains[id].numBrickPatches; i++)
				{
					proplib::Property &propPatch = propPatches.get( i );
					float centerX, centerZ;  // should be from 0.0 -> 1.0
					float sizeX, sizeZ;  // should be from 0.0 -> 1.0
					float nhsize;
					int shape, distribution, brickCount;
					Color color;


					centerX = propPatch.get( "CenterX" );
					centerZ = propPatch.get( "CenterZ" );
					sizeX = propPatch.get( "SizeX" );
					sizeZ = propPatch.get( "SizeZ" );
					brickCount = propPatch.get( "BrickCount" );
					{
						string val = propPatch.get( "Shape" );
						if( val == "R" )
							shape = 0;
						else if( val == "E" )
							shape = 1;
						else
							assert( false );
					}
					{
						string val = propPatch.get( "Distribution" );
						if( val == "U" )
							distribution = 0;
						else if( val == "L" )
							distribution = 1;
						else if( val == "G" )
							distribution = 2;
						else
							assert( false );
					}
					nhsize = propPatch.get( "NeighborhoodSize" );

					if( propPatch.hasProperty("BrickColor") )
						color = propPatch.get( "BrickColor" );
					else
						color = doc.get( "BrickColor" );

					bool on = propPatch.get( "On" );

					fDomains[id].fBrickPatches[i].init(color, centerX, centerZ, sizeX, sizeZ, brickCount, shape, distribution, nhsize, &fStage, &(fDomains[id]), id, on);

				}
			}

			totmaxnumagents += fDomains[id].maxNumAgents;
			totminnumagents += fDomains[id].minNumAgents;
		}

		if (totmaxnumagents > fMaxNumAgents)
		{
			char tempstring[256];
			sprintf(tempstring,"%s %ld %s %ld %s",
					"The maximum number of organisms in the world (",
					fMaxNumAgents,
					") is < the maximum summed over domains (",
					totmaxnumagents,
					"), so there may still be some indirect global influences.");
			errorflash(0, tempstring);
		}
		if (totminnumagents < fMinNumAgents)
		{
			char tempstring[256];
			sprintf(tempstring,"%s %ld %s %ld %s",
					"The minimum number of organisms in the world (",
					fMinNumAgents,
					") is > the minimum summed over domains (",
					totminnumagents,
					"), so there may still be some indirect global influences.");
			errorflash(0,tempstring);
		}

		if( numFoodPatchesNeedingRemoval > 0 )
		{
			// Allocate a maximally sized array to keep a list of food patches needing their food removed
			fFoodPatchesNeedingRemoval = new FoodPatch*[numFoodPatchesNeedingRemoval];
			fFoodRemovalNeeded = true;
		}
		else
			fFoodRemovalNeeded = false;

		int numberFittest = doc.get( "NumberFittest" );

		for (int id = 0; id < fNumDomains; id++)
		{
			fDomains[id].numAgents = 0;
			fDomains[id].numcreated = 0;
			fDomains[id].numborn = 0;
			fDomains[id].numbornsincecreated = 0;
			fDomains[id].numdied = 0;
			fDomains[id].lastcreate = 0;
			fDomains[id].maxgapcreate = 0;
			fDomains[id].energyScaleFactor = 1.0;
			fDomains[id].ifit = 0;
			fDomains[id].jfit = 1;
			if( numberFittest > 0 )
				fDomains[id].fittest = new FittestList( numberFittest, true );
			else
				fDomains[id].fittest = NULL;
		}
	} // Domains

	fUseProbabilisticFoodPatches = doc.get( "ProbabilisticFoodPatches" );


    fMinFoodEnergyAtDeath = doc.get( "MinFoodEnergyAtDeath" );
    fSkipCarcassMinFoodEnergyCheck = doc.get( "SkipCarcassMinFoodEnergyCheck" );
	fRandomBirthLocation = doc.get( "RandomBirthLocation" );
	fRandomBirthLocationRadius = doc.get( "RandomBirthLocationRadius" );
	{
		string prop = doc.get( "SmiteMode" );
		if( prop == "O" )
			fSmiteMode = 'O';
		else if( prop == "R" )
			fSmiteMode = 'R';
		else if( prop == "L" )
			fSmiteMode = 'L';
		else
			assert( false );
	}
    fSmiteFrac = doc.get( "SmiteFrac" );
    fSmiteAgeFrac = doc.get( "SmiteAgeFrac" );

	fNumDepletionSteps = doc.get( "NumDepletionSteps" );
	if( fNumDepletionSteps )
		fMaxPopulationPenaltyFraction = 1.0 / (float) fNumDepletionSteps;

	fApplyLowPopulationAdvantage = doc.get( "ApplyLowPopulationAdvantage" );

	fEnergyBasedPopulationControl = doc.get( "EnergyBasedPopulationControl" );
	fPopControlGlobal = doc.get( "PopControlGlobal" );
	fPopControlDomains = doc.get( "PopControlDomains" );
	fPopControlMinFixedRange = doc.get( "PopControlMinFixedRange" );
	fPopControlMaxFixedRange = doc.get( "PopControlMaxFixedRange" );
	fPopControlMinScaleFactor = doc.get( "PopControlMinScaleFactor" );
	fPopControlMaxScaleFactor = doc.get( "PopControlMaxScaleFactor" );

	fAllowMinDeaths = doc.get( "AllowMinDeaths" );
	fDieAtMaxAge = doc.get( "DieAtMaxAge" );

	fComplexityType = (string)doc.get( "ComplexityType" );
	fComplexityFitnessWeight = doc.get( "ComplexityFitnessWeight" );
	if( fComplexityFitnessWeight )
		fCalcComplexity = true;
	fHeuristicFitnessWeight = doc.get( "HeuristicFitnessWeight" );

	fTournamentSize = doc.get( "TournamentSize" );

	globals::recordFileType = (bool)doc.get( "CompressFiles" )
		? AbstractFile::TYPE_GZIP_FILE
		: AbstractFile::TYPE_FILE;

	fFogFunction = ((string)doc.get( "FogFunction" ))[0];
	assert( glFogFunction() == fFogFunction );
	// This value only does something if Fog Function is exponential
	// Acceptable values are between 0 and 1 (inclusive)
	fExpFogDensity = doc.get( "ExpFogDensity" );
	// This value only does something if Fog Function is linear
	// It defines the maximum distance a agent can see.
	fLinearFogEnd = doc.get( "LinearFogEnd" );
}

//-------------------------------------------------------------------------------------------
// TSimulation::initLockstepMode
//-------------------------------------------------------------------------------------------
void TSimulation::initLockstepMode()
{
	agent::config.minLifeSpan = fMaxSteps;
	agent::config.maxLifeSpan = fMaxSteps;

	agent::config.eat2Energy = 0.0;
	agent::config.mate2Energy = 0.0;
	agent::config.fight2Energy = 0.0;
	agent::config.maxSizePenalty = 0.0;
	agent::config.speed2Energy = 0.0;
	agent::config.yaw2Energy = 0.0;
	agent::config.light2Energy = 0.0;
	agent::config.focus2Energy = 0.0;
	agent::config.pickup2Energy = 0.0;
	agent::config.drop2Energy = 0.0;
	agent::config.carryAgent2Energy = 0.0;
	agent::config.carryAgentSize2Energy = 0.0;
	food::gCarryFood2Energy = 0.0;
	brick::gCarryBrick2Energy = 0.0;
	agent::config.fixedEnergyDrain = 0.0;

	fNumDepletionSteps = 0;
	fMaxPopulationPenaltyFraction = 0.0;

	fApplyLowPopulationAdvantage = false;
	fEnergyBasedPopulationControl = false;

	cout << "Due to running in LockStepWithBirthsDeathsLog mode, the following parameter values have been forcibly reset as indicated:" nl;
	cout << "  MinLifeSpan" ses agent::config.minLifeSpan nl;
	cout << "  MaxLifeSpan" ses agent::config.maxLifeSpan nl;
	cout << "  Eat2Energy" ses agent::config.eat2Energy nl;
	cout << "  Mate2Energy" ses agent::config.mate2Energy nl;
	cout << "  Fight2Energy" ses agent::config.fight2Energy nl;
	cout << "  MaxSizePenalty" ses agent::config.maxSizePenalty nl;
	cout << "  Speed2Energy" ses agent::config.speed2Energy nl;
	cout << "  Yaw2Energy" ses agent::config.yaw2Energy nl;
	cout << "  Light2Energy" ses agent::config.light2Energy nl;
	cout << "  Focus2Energy" ses agent::config.focus2Energy nl;
	cout << "  Pickup2Energy" ses agent::config.pickup2Energy nl;
	cout << "  Drop2Energy" ses agent::config.drop2Energy nl;
	cout << "  CarryAgent2Energy" ses agent::config.carryAgent2Energy nl;
	cout << "  CarryAgentSize2Energy" ses agent::config.carryAgentSize2Energy nl;
	cout << "  CarryFood2Energy" ses food::gCarryFood2Energy nl;
	cout << "  CarryBrick2Energy" ses brick::gCarryBrick2Energy nl;
	cout << "  FixedEnergyDrain" ses agent::config.fixedEnergyDrain nl;
	cout << "  NumDepletionSteps" ses fNumDepletionSteps nl;
	cout << "  .MaxPopulationPenaltyFraction" ses fMaxPopulationPenaltyFraction nl;
	cout << "  ApplyLowPopulationAdvantage" ses fApplyLowPopulationAdvantage nl;
}

//-------------------------------------------------------------------------------------------
// TSimulation::initFitnessMode
//-------------------------------------------------------------------------------------------
void TSimulation::initFitnessMode()
{
	fNumberToSeed = lrint( fMaxNumAgents * (float) fNumberToSeed / fInitNumAgents );	// same proportion as originally specified (must calculate before changing fInitNumAgents)
	if( fNumberToSeed > fMaxNumAgents )	// just to be safe
		fNumberToSeed = fMaxNumAgents;
	fInitNumAgents = fMaxNumAgents;	// population starts at maximum
	fMinNumAgents = fMaxNumAgents;		// population stays at mximum
	// 		if( fProbabilityOfMutatingSeeds == 0.0 )
	// 			fProbabilityOfMutatingSeeds = 1.0;	// so there is variation in the initial population
	//		fMateThreshold = 1.5;				// so they can't reproduce on their own

	for( int i = 0; i < fNumDomains; i++ )	// over all domains
	{
		fDomains[i].numberToSeed = lrint( fDomains[i].maxNumAgents * (float) fDomains[i].numberToSeed / fDomains[i].initNumAgents );	// same proportion as originally specified (must calculate before changing fInitNumAgents)
		if( fDomains[i].numberToSeed > fDomains[i].maxNumAgents )	// just to be safe
			fDomains[i].numberToSeed = fDomains[i].maxNumAgents;
		fDomains[i].initNumAgents = fDomains[i].maxNumAgents;	// population starts at maximum
		fDomains[i].minNumAgents  = fDomains[i].maxNumAgents;	// population stays at maximum
		// 			fDomains[i].probabilityOfMutatingSeeds = fProbabilityOfMutatingSeeds;				// so there is variation in the initial population
	}

	fNumDepletionSteps = 0;				// turn off the high-population penalty
	fMaxPopulationPenaltyFraction = 0.0;	// ditto
	fApplyLowPopulationAdvantage = false;			// turn off the low-population advantage
	fEnergyBasedPopulationControl = false;			// turn off energy-based population control

	cout << "Due to running as a steady-state GA with a fitness function, the following parameter values have been forcibly reset as indicated:" nl;
	cout << "  InitNumAgents" ses fInitNumAgents nl;
	cout << "  MinNumAgents" ses fMinNumAgents nl;
	cout << "  NumberToSeed" ses fNumberToSeed nl;
	// 		cout << "  ProbabilityOfMutatingSeeds" ses fProbabilityOfMutatingSeeds nl;
	//		cout << "  MateThreshold" ses fMateThreshold nl;
	for( int i = 0; i < fNumDomains; i++ )
	{
		cout << "  Domain " << i << ":" nl;
		cout << "    initNumAgents" ses fDomains[i].initNumAgents nl;
		cout << "    minNumAgents" ses fDomains[i].minNumAgents nl;
		cout << "    numberToSeed" ses fDomains[i].numberToSeed nl;
		// 			cout << "    probabilityOfMutatingSeeds" ses fDomains[i].probabilityOfMutatingSeeds nl;
	}
	cout << "  NumDepletionSteps" ses fNumDepletionSteps nl;
	cout << "  .MaxPopulationPenaltyFraction" ses fMaxPopulationPenaltyFraction nl;
	cout << "  ApplyLowPopulationAdvantage" ses fApplyLowPopulationAdvantage nl;
	cout << "  EnergyBasedPopulationControl" ses fEnergyBasedPopulationControl nl;
}

//---------------------------------------------------------------------------
// TSimulation::Dump
//---------------------------------------------------------------------------
void TSimulation::Dump()
{
    filebuf fb;
    if (fb.open("pw.dump", ios_base::out) == 0)
    {
        error(1, "Unable to dump state to \"pw.dump\" file");
        return;
    }

    ostream out(&fb);
    char version[16];
    sprintf(version, "%s", "pw1");

    out << version nl;
    out << fStep nl;
    out << fNumberCreated nl;
    out << fNumberCreatedRandom nl;
    out << fNumberCreated1Fit nl;
    out << fNumberCreated2Fit nl;
    out << fNumberDied nl;
    out << fNumberDiedAge nl;
    out << fNumberDiedEnergy nl;
    out << fNumberDiedFight nl;
    out << fNumberDiedEdge nl;
	out << fNumberDiedSmite nl;
    out << fNumberBorn nl;
	out << fNumberBornVirtual nl;
    out << fNumberFights nl;
    out << fMiscDenials nl;
    out << fLastCreated nl;
    out << fMaxGapCreate nl;
    out << fNumBornSinceCreated nl;
    out << fMaxFitness nl;
    out << fAverageFitness nl;
    out << fAverageFoodEnergyIn nl;
    out << fAverageFoodEnergyOut nl;
    out << fTotalFoodEnergyIn nl;
    out << fTotalFoodEnergyOut nl;

    agent::agentdump(out);

    out << objectxsortedlist::gXSortedObjects.getCount(FOODTYPE) nl;
	food* f = NULL;
	objectxsortedlist::gXSortedObjects.reset();
	while (objectxsortedlist::gXSortedObjects.nextObj(FOODTYPE, (gobject**)&f))
		f->dump(out);

    out << fFitI nl;
    out << fFitJ nl;
	if( fFittest )
		fFittest->dump( out );
	if( fRecentFittest)
		fRecentFittest->dump( out );

    out << fNumDomains nl;
    for (int id = 0; id < fNumDomains; id++)
    {
        out << fDomains[id].numAgents nl;
        out << fDomains[id].numcreated nl;
        out << fDomains[id].numborn nl;
        out << fDomains[id].numbornsincecreated nl;
        out << fDomains[id].numdied nl;
        out << fDomains[id].lastcreate nl;
        out << fDomains[id].maxgapcreate nl;
        out << fDomains[id].ifit nl;
        out << fDomains[id].jfit nl;

        if (fDomains[id].fittest)
			fDomains[id].fittest->dump( out );
    }

    out.flush();
    fb.close();
}


//---------------------------------------------------------------------------
// TSimulation::WhichDomain
//---------------------------------------------------------------------------
short TSimulation::WhichDomain(float x, float z, short d)
{
	for (short i = 0; i < fNumDomains; i++)
	{
		if (((x >= fDomains[i].startX) && (x <= fDomains[i].endX)) &&
			((z >= fDomains[i].startZ) && (z <= fDomains[i].endZ)))
			return i;
	}

	// If we reach here, we failed to find a domain, so kvetch and quit

	char errorString[256];

	printf( "Domain not found in %ld domains, located at:\n", fNumDomains );
	for( int i = 0; i < fNumDomains; i++ )
		printf( "  %d: ranging over x = (%f -> %f) and z = (%f -> %f)\n",
				i, fDomains[i].startX, fDomains[i].endX, fDomains[i].startZ, fDomains[i].endZ );

	sprintf(errorString,"%s (%g, %g) %s %d, %ld",
			"WhichDomain failed to find any domain for point at (x, z) = ",
			x, z, " & d, nd = ", d, fNumDomains);
	error(2, errorString);

	return( -1 );	// not really returning, as error(2,...) will abort
}


//---------------------------------------------------------------------------
// TSimulation::SwitchDomain
//
//	No verification is done. Assume caller has verified domains.
//---------------------------------------------------------------------------
void TSimulation::SwitchDomain(short newDomain, short oldDomain, int objectType)
{
	if( newDomain == oldDomain )
		return;

	switch( objectType )
	{
		case AGENTTYPE:
			fDomains[newDomain].numAgents++;
			fDomains[oldDomain].numAgents--;
			break;

		case FOODTYPE:
			fDomains[newDomain].foodCount++;
			fDomains[oldDomain].foodCount--;
			break;

		case BRICKTYPE:
			// Domains do not currently keep track of brick counts
			break;

		default:
			error( 2, "unknown object type %d", objectType, "" );
			break;
	}
}


//---------------------------------------------------------------------------
// TSimulation::getStatusText
//---------------------------------------------------------------------------
void TSimulation::getStatusText( StatusText& statusText,
								 int statusFrequency )
{
	char t[256];
	char t2[256];
	short id;

	// TODO: If we're neither updating the window, nor writing to the stat file,
	// then we shouldn't sprintf all these strings, or put them in the statusText
	// (but for now, the window always draws anyway, so it's not a big deal)

	sprintf( t, "step = %ld", fStep );
	statusText.push_back( strdup( t ) );

	sprintf( t, "agents = %4d", objectxsortedlist::gXSortedObjects.getCount(AGENTTYPE) );
	if (fNumDomains > 1)
	{
		sprintf(t2, " (%ld",fDomains[0].numAgents );
		strcat(t, t2 );
		for (id = 1; id < fNumDomains; id++)
		{
			sprintf(t2, ", %ld", fDomains[id].numAgents );
			strcat( t, t2 );
		}

		strcat(t,")" );
	}
	statusText.push_back( strdup( t ) );
	if( Metabolism::getNumberOfDefinitions() > 1 )
	{
		for( int i = 0; i < Metabolism::getNumberOfDefinitions(); i++ )
		{
			sprintf( t, " -%s = %4ld", Metabolism::get(i)->name.c_str(), fNumberAliveWithMetabolism[i] );
			statusText.push_back( strdup( t ) );
		}
	}

	sprintf( t, "food = %4d", objectxsortedlist::gXSortedObjects.getCount(FOODTYPE) );
	if (fNumDomains > 1)
	{
		sprintf(t2, " (%d",fDomains[0].foodCount );
		strcat(t, t2 );
		for (id = 1; id < fNumDomains; id++)
		{
			sprintf(t2, ", %d", fDomains[id].foodCount );
			strcat( t, t2 );
		}

		strcat(t,")" );
	}
	statusText.push_back( strdup( t ) );

	sprintf( t, "foodEnergy = %.1f", getFoodEnergy() );
	statusText.push_back( strdup( t ) );

	sprintf( t, "created  = %4ld", fNumberCreated );
	if (fNumDomains > 1)
	{
		sprintf( t2, " (%ld",fDomains[0].numcreated );
		strcat( t, t2 );

		for (id = 1; id < fNumDomains; id++)
		{
			sprintf( t2, ",%ld",fDomains[id].numcreated );
			strcat( t, t2 );
		}
		strcat(t,")" );
	}
	statusText.push_back( strdup( t ) );

	sprintf( t, " -random = %4ld", fNumberCreatedRandom );
	statusText.push_back( strdup( t ) );

	sprintf( t, " -two    = %4ld", fNumberCreated2Fit );
	statusText.push_back( strdup( t ) );

	sprintf( t, " -one    = %4ld", fNumberCreated1Fit );
	statusText.push_back( strdup( t ) );

	sprintf( t, "born     = %4ld", fNumberBorn );
	if (fNumDomains > 1)
	{
		sprintf( t2, " (%ld",fDomains[0].numborn );
		strcat( t, t2 );
		for (id = 1; id < fNumDomains; id++)
		{
			sprintf( t2, ",%ld",fDomains[id].numborn );
			strcat( t, t2 );
		}
		strcat(t,")" );
	}
	statusText.push_back( strdup( t ) );

	if( (fHeuristicFitnessWeight != 0.0) || (fComplexityFitnessWeight != 0.0) || fLockStepWithBirthsDeathsLog )
	{
		sprintf( t, "born_v  = %4ld", fNumberBornVirtual );
		statusText.push_back( strdup( t ) );
	}

	sprintf( t, "died     = %4ld", fNumberDied );
	if (fNumDomains > 1)
	{
		sprintf( t2, " (%ld",fDomains[0].numdied );
		strcat( t, t2 );
		for (id = 1; id < fNumDomains; id++)
		{
			sprintf( t2, ",%ld",fDomains[id].numdied );
			strcat( t, t2 );
		}
		strcat(t,")" );
	}
	statusText.push_back( strdup( t ) );


	sprintf( t, " -age    = %4ld", fNumberDiedAge );
	statusText.push_back( strdup( t ) );

	sprintf( t, " -energy = %4ld", fNumberDiedEnergy );
	statusText.push_back( strdup( t ) );

	sprintf( t, " -fight  = %4ld", fNumberDiedFight );
	statusText.push_back( strdup( t ) );

	sprintf( t, " -eat  = %4ld", fNumberDiedEat );
	statusText.push_back( strdup( t ) );

	sprintf( t, " -edge   = %4ld", fNumberDiedEdge );
	statusText.push_back( strdup( t ) );

	sprintf( t, " -smite  = %4ld", fNumberDiedSmite );
	statusText.push_back( strdup( t ) );

	sprintf( t, " -patch  = %4ld", fNumberDiedPatch );
	statusText.push_back( strdup( t ) );

    sprintf( t, "birthDenials = %ld", fBirthDenials );
	statusText.push_back( strdup( t ) );

    sprintf( t, "miscDenials = %ld", fMiscDenials );
	statusText.push_back( strdup( t ) );

    sprintf( t, "ageCreate = %ld", fLastCreated );
    if (fNumDomains > 1)
    {
        sprintf( t2, " (%ld",fDomains[0].lastcreate );
        strcat( t, t2 );
        for (id = 1; id < fNumDomains; id++)
        {
            sprintf( t2, ",%ld",fDomains[id].lastcreate );
            strcat( t, t2 );
        }
        strcat(t,")" );
    }
	statusText.push_back( strdup( t ) );

	sprintf( t, "maxGapCreate = %ld", fMaxGapCreate );
	if (fNumDomains > 1)
	{
		sprintf( t2, " (%ld",fDomains[0].maxgapcreate );
	   	strcat( t, t2 );
	   	for (id = 1; id < fNumDomains; id++)
	   	{
			sprintf( t2, ",%ld", fDomains[id].maxgapcreate );
	       	strcat(t, t2 );
	   	}
	   	strcat(t,")" );
	}
	statusText.push_back( strdup( t ) );

	if( (fHeuristicFitnessWeight != 0.0) || (fComplexityFitnessWeight != 0.0) )
		sprintf( t, "born_v/(c+bv) = %.2f", float(fNumberBornVirtual) / float(fNumberCreated + fNumberBornVirtual) );
	else
		sprintf( t, "born/total = %.2f", float(fNumberBorn) / float(fNumberCreated + fNumberBorn) );
	statusText.push_back( strdup( t ) );

	sprintf( t, "Fitness m=%.2f, c=%.2f, a=%.2f", fMaxFitness, fCurrentMaxFitness[0] / fTotalHeuristicFitness, fAverageFitness );
	statusText.push_back( strdup( t ) );

//	sprintf( t, "NormFit m=%.2f, c=%.2f, a=%.2f", fMaxFitness / fTotalHeuristicFitness, fCurrentMaxFitness[0] / fTotalHeuristicFitness, fAverageFitness / fTotalHeuristicFitness );
//	statusText.push_back( strdup( t ) );

	sprintf( t, "Fittest =" );
	int fittestCount = min( 5, fFittest->size() );
	for( int i = 0; i < fittestCount; i++ )
	{
		sprintf( t2, " %lu", fFittest->get(i)->agentID );
		strcat( t, t2 );
	}
	statusText.push_back( strdup( t ) );

	if( fittestCount > 0 )
	{
		sprintf( t, " " );
		for( int i = 0; i < fittestCount; i++ )
		{
			sprintf( t2, "  %.2f", fFittest->get(i)->fitness );
			strcat( t, t2 );
		}
		statusText.push_back( strdup( t ) );
	}

	sprintf( t, "CurFit =" );
	for( int i = 0; i < fCurrentFittestCount; i++ )
	{
		sprintf( t2, " %lu", fCurrentFittestAgent[i]->Number() );
		strcat( t, t2 );
	}
	statusText.push_back( strdup( t ) );

	if( fCurrentFittestCount > 0 )
	{
		sprintf( t, " " );
		for( int i = 0; i < fCurrentFittestCount; i++ )
		{
			sprintf( t2, "  %.2f", fCurrentFittestAgent[i]->HeuristicFitness() / fTotalHeuristicFitness );
			strcat( t, t2 );
		}
		statusText.push_back( strdup( t ) );
	}

	sprintf( t, "avgFoodEnergy = %.2f", (fAverageFoodEnergyIn - fAverageFoodEnergyOut) / (fAverageFoodEnergyIn + fAverageFoodEnergyOut) );
	statusText.push_back( strdup( t ) );

	sprintf( t, "totFoodEnergy = %.2f", (fTotalFoodEnergyIn - fTotalFoodEnergyOut) / (fTotalFoodEnergyIn + fTotalFoodEnergyOut) );
	statusText.push_back( strdup( t ) );

	sprintf( t, "totEnergyEaten = %.1f", fTotalEnergyEaten[0] );
	for( int i = 1; i < globals::numEnergyTypes; i++ )
	{
		sprintf( t2, ", %.1f", fTotalEnergyEaten[i] );
		strcat( t, t2 );
	}
	statusText.push_back( strdup( t ) );

	static Energy lastTotalEnergyEaten;
	static Energy deltaEnergy;
    if( !(fStep % statusFrequency) )
    {
		deltaEnergy = fTotalEnergyEaten - lastTotalEnergyEaten;
		lastTotalEnergyEaten = fTotalEnergyEaten;
	}
	sprintf( t, "EatRate = %.1f", deltaEnergy[0] / statusFrequency );
	for( int i = 1; i < globals::numEnergyTypes; i++ )
	{
		sprintf( t2, ", %.1f", deltaEnergy[i] / statusFrequency );
		strcat( t, t2 );
	}
	statusText.push_back( strdup( t ) );

	static long lastNumberBorn = 0;
	static long deltaBorn;
	long numberBorn;
	if( (fComplexityFitnessWeight != 0.0) || (fHeuristicFitnessWeight != 0.0) || fLockStepWithBirthsDeathsLog )
		numberBorn = fNumberBornVirtual;
	else
		numberBorn = fNumberBorn;
    if( !(fStep % statusFrequency) )
    {
		deltaBorn = numberBorn - lastNumberBorn;
		lastNumberBorn = numberBorn;
	}
	sprintf( t, "MateRate = %.2f", (double) deltaBorn / statusFrequency );
	statusText.push_back( strdup( t ) );

	sprintf( t, "LifeSpan = %lu \xb1 %lu [%lu, %lu]", nint( fLifeSpanStats.mean() ), nint( fLifeSpanStats.stddev() ), (unsigned long) fLifeSpanStats.min(), (unsigned long) fLifeSpanStats.max() );
	statusText.push_back( strdup( t ) );

	sprintf( t, "RecLifeSpan = %lu \xb1 %lu [%lu, %lu]", nint( fLifeSpanRecentStats.mean() ), nint( fLifeSpanRecentStats.stddev() ), (unsigned long) fLifeSpanRecentStats.min(), (unsigned long) fLifeSpanRecentStats.max() );
	statusText.push_back( strdup( t ) );

	// ---
	// --- addStat()
	// ---
	function<void (const char *, Stat &)> addStat =
		[&t, &statusText] ( const char *name, Stat &stat )
		{
			sprintf( t, "%s = %.1f \xb1 %.1f [%lu, %lu]",
					 name, stat.mean(), stat.stddev(), (unsigned long) stat.min(), (unsigned long) stat.max() );
			statusText.push_back( strdup( t ) );
		};

	addStat( "CurNeurons", fCurrentBrainStats.neuronCount );

	switch( Brain::config.architecture )
	{
	case Brain::Configuration::Groups:
		addStat( "CurNeurGroups", fCurrentBrainStats.groups.groupCount );
		break;
	case Brain::Configuration::Sheets:
		addStat( "CurInternalSheets", fCurrentBrainStats.sheets.internalSheetCount );
		addStat( "CurInternalNeurons", fCurrentBrainStats.sheets.internalNeuronCount );
		for( SheetSynapseType &type : SheetSynapseTypes )
		{
			char name[ 64 ];
			sprintf( name, "CurSynapse%sTo%s", sheets::Sheet::getName(type.from), sheets::Sheet::getName(type.to) );

			addStat( name, fCurrentBrainStats.sheets.synapseCount[type.from][type.to] );
		}
		break;
	default:
		assert( false );
	}

	addStat( "CurSynapses", fCurrentBrainStats.synapseCount );

	sprintf( t, "Rate %2.1f (%2.1f) %2.1f (%2.1f) %2.1f (%2.1f)",
			 fFramesPerSecondInstantaneous, fSecondsPerFrameInstantaneous,
			 fFramesPerSecondRecent,        fSecondsPerFrameRecent,
			 fFramesPerSecondOverall,       fSecondsPerFrameOverall  );
	statusText.push_back( strdup( t ) );

	if( fCalcFoodPatchAgentCounts )
	{
		int numAgentsInAnyFoodPatchInAnyDomain = 0;
		int numAgentsInOuterRangesInAnyDomain = 0;

		for( int domainNumber = 0; domainNumber < fNumDomains; domainNumber++ )
		{
			sprintf( t, "Domain %d", domainNumber);
			statusText.push_back( strdup( t ) );

			int numAgentsInAnyFoodPatch = 0;
			int numAgentsInOuterRanges = 0;

			for( int i = 0; i < fDomains[domainNumber].numFoodPatches; i++ )
			{
				numAgentsInAnyFoodPatch += fDomains[domainNumber].fFoodPatches[i].agentInsideCount;
				numAgentsInOuterRanges += fDomains[domainNumber].fFoodPatches[i].agentNeighborhoodCount;
			}

			float makePercent = 100.0 / fDomains[domainNumber].numAgents;
			float makePercentNorm = 100.0 / numAgentsInAnyFoodPatch;

			for( int i = 0; i < fDomains[domainNumber].numFoodPatches; i++ )
			{
				sprintf( t, "  FP%d %d %3d %3d  %4.1f %4.1f  %4.1f",
						 i,
						 fDomains[domainNumber].fFoodPatches[i].foodCount,
						 fDomains[domainNumber].fFoodPatches[i].agentInsideCount,
						 fDomains[domainNumber].fFoodPatches[i].agentInsideCount + fDomains[domainNumber].fFoodPatches[i].agentNeighborhoodCount,
						 fDomains[domainNumber].fFoodPatches[i].agentInsideCount * makePercent,
						(fDomains[domainNumber].fFoodPatches[i].agentInsideCount + fDomains[domainNumber].fFoodPatches[i].agentNeighborhoodCount) * makePercent,
						 fDomains[domainNumber].fFoodPatches[i].agentInsideCount * makePercentNorm );
				statusText.push_back( strdup( t ) );
			}


			sprintf( t, "  FP* %3d %3d  %4.1f %4.1f 100.0",
					 numAgentsInAnyFoodPatch,
					 numAgentsInAnyFoodPatch + numAgentsInOuterRanges,
					 numAgentsInAnyFoodPatch * makePercent,
					(numAgentsInAnyFoodPatch + numAgentsInOuterRanges) * makePercent );
			statusText.push_back( strdup( t ) );

			numAgentsInAnyFoodPatchInAnyDomain += numAgentsInAnyFoodPatch;
			numAgentsInOuterRangesInAnyDomain += numAgentsInOuterRanges;
		}

		if( fNumDomains > 1 )
		{
			float makePercent = 100.0 / objectxsortedlist::gXSortedObjects.getCount( AGENTTYPE );

			sprintf( t, "**FP* %3d %3d  %4.1f %4.1f 100.0",
					 numAgentsInAnyFoodPatchInAnyDomain,
					 numAgentsInAnyFoodPatchInAnyDomain + numAgentsInOuterRangesInAnyDomain,
					 numAgentsInAnyFoodPatchInAnyDomain * makePercent,
					(numAgentsInAnyFoodPatchInAnyDomain + numAgentsInOuterRangesInAnyDomain) * makePercent );
			statusText.push_back( strdup( t ) );
		}
	}

	// Dynamic Properties
	{
		int nprops;
		proplib::CppProperties::PropertyMetadata *metadata;
		proplib::CppProperties::getMetadata( &metadata, &nprops );

		for( int i = 0; i < nprops; i++ )
		{
			if( metadata[i].type == proplib::CppProperties::PropertyMetadata::Dynamic )
			{
				sprintf( t, "%s = %s", metadata[i].name.c_str(), metadata[i].toString() );
				statusText.push_back( strdup(t) );
			}
		}
	}
}


int TSimulation::getRandomPatch( int domainNumber )
{
	int patch;
	float ranval;
	float maxFractions = 0.0;

	// Since not all patches may be "on", we need to calculate the maximum fraction
	// attainable by those patches that are on, and therefore allowed to grow food
	for( short i = 0; i < fDomains[domainNumber].numFoodPatches; i++ )
		if( fDomains[domainNumber].fFoodPatches[i].isOn() )
			maxFractions += fDomains[domainNumber].fFoodPatches[i].fraction;

	if( maxFractions > 0.0 )	// there is an active patch in this domain
	{
		float sumFractions = 0.0;

		// Weight the random value by the maximum attainable fraction, so we always get
		// a valid patch selection (if possible--they could all be off)
		ranval = randpw() * maxFractions;

		for( short i = 0; i < fDomains[domainNumber].numFoodPatches; i++ )
		{
			if( fDomains[domainNumber].fFoodPatches[i].isOn() )
			{
				sumFractions += fDomains[domainNumber].fFoodPatches[i].fraction;
				if( ranval <= sumFractions )
					return( i );    // this is the patch
			}
		}

		// Shouldn't get here
		patch = int( floor( ranval * fDomains[domainNumber].numFoodPatches ) );
		if( patch >= fDomains[domainNumber].numFoodPatches )
			patch  = fDomains[domainNumber].numFoodPatches - 1;
		fprintf( stderr, "%s: ranval of %g failed to end up in any food patch; assigning patch #%d\n", __FUNCTION__, ranval, patch );
	}
	else
		patch = -1;	// no patches are active in this domain

	return( patch );
}

void TSimulation::SetNextLockstepEvent()
{
	if( !fLockStepWithBirthsDeathsLog )
	{
		// if SetNextLockstepEvent() is called w/o fLockStepWithBirthsDeathsLog turned on, error and exit.
		cerr << "ERROR: You called SetNextLockstepEvent() and 'fLockStepWithBirthsDeathsLog' isn't set to true.  Though not fatal, it's certain that you didn't intend to do this.  Exiting." << endl;
		exit(1);
	}

	const char *delimiters = " ";		// a single space is the field delimiter
	char LockstepLine[512];				// making this big in case we add longer lines in the future.

	fLockstepNumDeathsAtTimestep = 0;
	fLockstepNumBirthsAtTimestep = 0;

	if( fgets( LockstepLine, sizeof(LockstepLine), fLockstepFile ) )			// get the next event, if the LOCKSTEP-BirthsDeaths.log still has entries in it.
	{
		fLockstepTimestep = atoi( strtok( LockstepLine, delimiters ) );		// token => timestep
		assert( fLockstepTimestep > 0 );										// if we get a >= zero timestep something is definitely wrong.

		int currentpos;
		int nexttimestep;
		char LockstepEvent;

		do
		{
			nexttimestep = 0;
			LockstepEvent = (strtok( NULL, delimiters ))[0];    // token => event

			//TODO: Add support for the 'GENERATION' event.  Note a GENERATION event cannot be identical to a BIRTH event.  They must be made from the Fittest list.
			if( LockstepEvent == 'B' )
				fLockstepNumBirthsAtTimestep++;
			else if( LockstepEvent == 'D' )
				fLockstepNumDeathsAtTimestep++;
			else if( LockstepEvent == 'C' )
			{
				fLockstepNumBirthsAtTimestep++;
				cerr << "t" << fLockstepTimestep << ": Warning: a CREATION event occured, but we're simply going to treat it as a random BIRTH." << endl;
			}
			else
			{
				cerr << "ERROR/SetNextLockstepEvent(): Currently only support events 'DEATH', 'BIRTH', and 'CREATION' events in the Lockstep file.  Exiting.";
				cerr << "Latest Event: '" << LockstepEvent << "'" << endl;
				exit(1);
			}

			currentpos = ftell( fLockstepFile );

			//=======================

			if( (fgets(LockstepLine, sizeof(LockstepLine), fLockstepFile)) != NULL )		// if LOCKSTEP-BirthsDeaths.log still has entries in it, nexttimestep is the timestep of the next line.
				nexttimestep = atoi( strtok( LockstepLine, delimiters ) );    // token => timestep

		} while( fLockstepTimestep == nexttimestep );

		// reset to the beginning of the next timestep
		fseek( fLockstepFile, currentpos, 0 );
		lsPrint( "SetNextLockstepEvent()/ Timestep: %d\tDeaths: %d\tBirths: %d\n", fLockstepTimestep, fLockstepNumDeathsAtTimestep, fLockstepNumBirthsAtTimestep );
	}
}

//GL Fog
char TSimulation::glFogFunction()
{
//	if( fFogFunction == 'O' || fFogFunction == 'L' || fFogFunction == 'E' )
		return fFogFunction;
//	else
//		return 'O';
}

float TSimulation::glExpFogDensity() { return fExpFogDensity; }
int TSimulation::glLinearFogEnd()    { return fLinearFogEnd;  }
