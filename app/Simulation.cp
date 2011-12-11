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
#include <iostream>
#include <omp.h>
#include <sstream>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <assert.h>

// qt
#include <qapplication.h>
#include <qdesktopwidget.h>

// Local
#include "AbstractFile.h"
#include "barrier.h"
#include "Brain.h"
#include "BrainMonitorWindow.h"
#include "ChartWindow.h"
#include "ContactEntry.h"
#include "condprop.h"
#include "AgentPOVWindow.h"
#include "debug.h"
#include "food.h"
#include "globals.h"
#include "food.h"
#include "Genome.h"
#include "GenomeUtil.h"
#include "proplib.h"
#include "Metabolism.h"
#include "Queue.h"
#include "RandomNumberGenerator.h"
#include "Resources.h"
#include "SceneView.h"
#include "TextStatusWindow.h"
#include "PwMovieUtils.h"
#include "complexity.h"

#include "objectxsortedlist.h"
#include "Patch.h"
#include "FoodPatch.h"
#include "BrickPatch.h"
#include "brick.h"


using namespace condprop;
using namespace genome;
using namespace std;

//===========================================================================
// TSimulation
//===========================================================================

static long numglobalcreated = 0;    // needs to be static so we only get warned about influence of global creations once ever

long TSimulation::fMaxNumAgents;
long TSimulation::fStep;
short TSimulation::fOverHeadRank = 1;
agent* TSimulation::fMonitorAgent = NULL;
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
	#define lsPrint( x... ) { if( (fStep >= MinDebugStep) && (fStep <= MaxDebugStep) ) printf( x ); }
#else
	#define lsPrint( x... )
#endif

#define IS_PREVENTED_BY_CARRY( ACTION, AGENT ) \
	( (fCarryPrevents##ACTION != 0) && ((AGENT)->NumCarries() > 0) && (randpw() < fCarryPrevents##ACTION) )

#define SYSTEM(cmd) {int rc = system(cmd); if(rc != 0) {fprintf(stderr, "Failed executing command '%s'\n", cmd); exit(1);}}

//---------------------------------------------------------------------------
// TSimulation::TSimulation
//---------------------------------------------------------------------------
TSimulation::TSimulation( TSceneView* sceneView, TSceneWindow* sceneWindow, const char *worldfilePath, const bool statusToStdout )
	:
		fLockStepWithBirthsDeathsLog(false),
		fLockstepFile(NULL),
		fAgentTracking(false),
//		fMonitorAgentRank(0),
		fMonitorAgentRankOld(0),
		fRotateWorld(false),
//		fOverHeadRank(0),
//		fMonitorAgent(NULL),
		fBrainFunctionRecentRecordFrequency(1000),
		fBestSoFarBrainAnatomyRecordFrequency(0),
		fBestSoFarBrainFunctionRecordFrequency(0),
		fBestRecentBrainAnatomyRecordFrequency(0),
		fBestRecentBrainFunctionRecordFrequency(0),
		fRecordBirthsDeaths(false),

		fLifeSpanLog(NULL),
		fRecordPosition(false),
		fRecordContacts(false),
		fContactsLog(NULL),
		fRecordCollisions(false),
		fCollisionsLog(NULL),
		fRecordCarry(false),
		fCarryLog(NULL),
		fRecordEnergy(false),
		fEnergyLog(NULL),

		fBrainAnatomyRecordAll(false),
		fBrainFunctionRecordAll(false),
		fBrainAnatomyRecordSeeds(false),
		fBrainFunctionRecordSeeds(false),
		fApplyLowPopulationAdvantage(false),

		fRecordComplexity(false),

		fRecordGenomes(false),
		fRecordSeparations(false),
		fRecordAdamiComplexity(false),
		fAdamiComplexityRecordFrequency(0),
		
		fEvents(NULL),

		fSceneView(sceneView),
		fSceneWindow(sceneWindow),
		fOverheadWindow(NULL),
		fBirthrateWindow(NULL),
		fFitnessWindow(NULL),
		fFoodEnergyWindow(NULL),
		fPopulationWindow(NULL),
		fBrainMonitorWindow(NULL),
		fGeneSeparationWindow(NULL),
		fAgentPOVWindow(NULL),
		fTextStatusWindow(NULL),
		fMaxSteps(0),
		fPaused(false),
		fDelay(0),
		fDumpFrequency(500),
		fStatusFrequency(100),
		fLoadState(false),
		inited(false),
		
		fSolidObjects(0x4),	// only bricks are solid by default, for historical reasons

		fHealing(0),
		fGroundClearance(0.0),
		fOverHeadRankOld(0),
		fOverheadAgent(NULL),

		fChartBorn(true),
		fChartFitness(true),
		fChartFoodEnergy(true),
		fChartPopulation(true),
//		fShowBrain(false),
		fShowTextStatus(true),
		fRecordGeneStats(false),
		fRecordFoodPatchStats(false),
		fCalcFoodPatchAgentCounts(false),

		fNewDeaths(0),
		fNumberFit(0),
		fFittest(NULL),

		fNumberRecentFit(0),
		fRecentFittest(NULL),
		fFogFunction('O'),
		fBrainMonitorStride(25),
		fGeneSum(NULL),
		fGeneSum2(NULL),
		fGeneStatsAgents(NULL),
		fGeneStatsFile(NULL),
		fFoodPatchStatsFile(NULL),
		fNumAgentsNotInOrNearAnyFoodPatch(0),
		fConditionalProps( new condprop::PropertyList() )
{
	Init( worldfilePath, statusToStdout );
}


//---------------------------------------------------------------------------
// TSimulation::~TSimulation
//---------------------------------------------------------------------------
TSimulation::~TSimulation()
{
	Stop();


	// If we were keeping the simulation in sync with a LOCKSTEP-BirthsDeaths.log, close the file now that the simulation is over.
	if( fLockstepFile )
		fclose( fLockstepFile );
	
	if( fGeneSum )
		free( fGeneSum );

	if( fGeneSum2 )
		free( fGeneSum2 );

    if( fGeneStatsAgents )
		free( fGeneStatsAgents );
		
	if( fGeneStatsFile )
		fclose( fGeneStatsFile );

	// Close windows and save prefs
	if (fBirthrateWindow != NULL)
		delete fBirthrateWindow;

	if (fFitnessWindow != NULL)
		delete fFitnessWindow;

	if (fFoodEnergyWindow != NULL)
		delete fFoodEnergyWindow;

	if (fPopulationWindow != NULL)
		delete fPopulationWindow;
	
	if (fBrainMonitorWindow != NULL)
		delete fBrainMonitorWindow;	

	if (fGeneSeparationWindow != NULL)
		delete fGeneSeparationWindow;

	if (fAgentPOVWindow != NULL)
		delete fAgentPOVWindow;

	if (fTextStatusWindow != NULL)
		delete fTextStatusWindow;
	
	if (fOverheadWindow != NULL)
		delete fOverheadWindow;
}


//---------------------------------------------------------------------------
// TSimulation::Start
//---------------------------------------------------------------------------
void TSimulation::Start()
{

}


//---------------------------------------------------------------------------
// TSimulation::Stop
//---------------------------------------------------------------------------
void TSimulation::Stop()
{
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
        {
            for (int i = 0; i < fNumberFit; i++)
            {
				if (fDomains[id].fittest[i])
				{
					if (fDomains[id].fittest[i]->genes != NULL)
						delete fDomains[id].fittest[i]->genes;
					delete fDomains[id].fittest[i];
				}
			}
            delete fDomains[id].fittest;
        }
		
		if( fDomains[id].fLeastFit )
			delete[] fDomains[id].fLeastFit;
    }


    if (fFittest != NULL)
    {
        for (int i = 0; i < fNumberFit; i++)
        {
			if (fFittest[i])
			{
				if (fFittest[i]->genes != NULL)
					delete fFittest[i]->genes;
				delete fFittest[i];
			}
		}		
        delete[] fFittest;
    }
	
    if( fRecentFittest != NULL )
    {
        for( int i = 0; i < fNumberRecentFit; i++ )
        {
			if( fRecentFittest[i] )
			{
//				if( fRecentFittest[i]->genes != NULL )	// we don't save the genes in the bestRecent list
//					delete fFittest[i]->genes;
				delete fRecentFittest[i];
			}
		}		
        delete[] fRecentFittest;
    }
	
    Brain::braindestruct();

    agent::agentdestruct();
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
	long			oldNumBorn;
	long			oldNumCreated;

	if( !inited )
	{
		printf( "%s: called before TSimulation::Init()\n", __FUNCTION__ );
		return;
	}
	
	if( (frame == 0) && (fSimulationSeed != 0) )
	{
		srand48(fSimulationSeed);
	}
	
	frame++;
//	printf( "%s: frame = %lu\n", __FUNCTION__, frame );
	
	if( fMaxSteps && ((fStep+1) > fMaxSteps) )
	{
		End( "MaxSteps" );
	}
	else if( fEndOnPopulationCrash && 
			 (objectxsortedlist::gXSortedObjects.getCount(AGENTTYPE) <= fMinNumAgents) )
	{
		cerr << "Population crash at step " << fStep << endl;
		End( "PopulationCrash" );
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

	// Update the barriers, now that they can be dynamic
	barrier* b;
	barrier::gXSortedBarriers.reset();
	while( barrier::gXSortedBarriers.next( b ) )
		b->update();
	barrier::gXSortedBarriers.xsort();
	
	// Update the conditional properties
	fConditionalProps->update( fStep );

	// Update all agents, using their neurally controlled behaviors
	{
		// Make the agent POV window the current GL context
		fAgentPOVWindow->makeCurrent();
		
		// Clear the window's color and depth buffers
		fAgentPOVWindow->qglClearColor( Qt::black );
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
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
			agent::gPopulationPenaltyFraction = 0.0;	// we're not currently applying the high population penalty

			// If we want to help it, apply the low population advantage
			if( fApplyLowPopulationAdvantage )
			{
				agent::gLowPopulationAdvantageFactor = 1.0 - (float) (initNumAgents - numAgents) / (initNumAgents - minNumAgents);
				if( agent::gLowPopulationAdvantageFactor < 0.0 )
					agent::gLowPopulationAdvantageFactor = 0.0;
				if( agent::gLowPopulationAdvantageFactor > 1.0 )
					agent::gLowPopulationAdvantageFactor = 1.0;
			}
		}
		else if( excess > 0 )	// population is greater than initial level everywhere
		{
			agent::gLowPopulationAdvantageFactor = 1.0;	// we're not currently applying the low population advantage

			// apply the high population penalty (if the max-penalty is non-zero)
			agent::gPopulationPenaltyFraction = agent::gMaxPopulationPenaltyFraction * (numAgents - initNumAgents) / (maxNumAgents - initNumAgents);
			if( agent::gPopulationPenaltyFraction < 0.0 )
				agent::gPopulationPenaltyFraction = 0.0;
			if( agent::gPopulationPenaltyFraction > agent::gMaxPopulationPenaltyFraction )
				agent::gPopulationPenaltyFraction = agent::gMaxPopulationPenaltyFraction;
		}
		
//		printf( "step=%4ld, pop=%3d, initPop=%3ld, minPop=%2ld, maxPop=%3ld, maxPopPenaltyFraction=%g, popPenaltyFraction=%g, lowPopAdvantageFactor=%g\n",
//				fStep, numAgents, initNumAgents, minNumAgents, maxNumAgents,
//				agent::gMaxPopulationPenaltyFraction, agent::gPopulationPenaltyFraction, agent::gLowPopulationAdvantageFactor );
		
		if( fStaticTimestepGeometry )
		{
			UpdateAgents_StaticTimestepGeometry();
		}
		else // if( fStaticTimestepGeometry )
		{
			UpdateAgents();
		}

		// Swap buffers for the agent POV window when they're all done
		fAgentPOVWindow->swapBuffers();
	}

	if( (fHeuristicFitnessWeight != 0.0) || (fComplexityFitnessWeight != 0.0) )
		oldNumBorn = fNumberBornVirtual;
	else
		oldNumBorn = fNumberBorn;
	oldNumCreated = fNumberCreated;
	
//  if( fDoCPUWork )

	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
	// ^^^ MASTER TASK ExecInteract
	// ^^^
	// ^^^ Handles collisions, matings, fights, deaths, births, etc
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
	class ExecInteract : public ITask
	{
	public:
		virtual void task_exec( TSimulation *sim )
		{
			sim->Interact();
		}
	} execInteract;

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// !!! EXEC MASTER
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	fScheduler.execMasterTask( this,
							   execInteract,
							   !fParallelInteract );
		
	assert( fNumberAlive == objectxsortedlist::gXSortedObjects.getCount(AGENTTYPE) );

	debugcheck( "after Interact() in step %ld", fStep );

//	fAverageFitness /= agent::gXSortedAgents.count();
	if( fNumAverageFitness > 0 )
		fAverageFitness /= fNumAverageFitness * fTotalHeuristicFitness;

#if DebugMaxFitness
	printf( "At age %ld (c,n,fit,c->fit) =", fStep );
	for( i = 0; i < fCurrentFittestCount; i++ )
		printf( " (%08lx,%ld,%5.2f,%5.2f)", (ulong) fCurrentFittestAgent[i], fCurrentFittestAgent[i]->Number(), fCurrentMaxFitness[i], fCurrentFittestAgent[i]->HeuristicFitness() );
	printf( "\n" );
#endif

    if (fMonitorGeneSeparation && (fNewDeaths > 0))
        CalculateGeneSeparationAll();

	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
	// ^^^ MASTER TASK CreateAgentsTask
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
	class CreateAgentsTask : public ITask
	{
	public:
		virtual void task_exec( TSimulation *sim )
		{
			sim->CreateAgents();
		}
	} execCreateAgents;

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// !!! EXEC MASTER
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	fScheduler.execMasterTask( this,
							   execCreateAgents,
							   !fParallelCreateAgents );

    if ((fNewLifes || fNewDeaths) && fMonitorGeneSeparation)
    {
        if (fRecordGeneSeparation)
            RecordGeneSeparation();

		if (fChartGeneSeparation && fGeneSeparationWindow != NULL)
			fGeneSeparationWindow->AddPoint(fGeneSepVals, fNumGeneSepVals);
    }

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
	
	// Update the various graphical windows
	if (fGraphics)
	{
		fSceneView->Draw();
		
		// Text Status window
		fTextStatusWindow->Draw();
		
		// Overhead window
		if (fOverHeadRank)
		{
			if (!fAgentTracking
				|| (fOverHeadRank != fOverHeadRankOld)
				|| !fOverheadAgent
				|| !(fOverheadAgent->Alive()))
			{
				fOverheadAgent = fCurrentFittestAgent[fOverHeadRank - 1];
				fOverHeadRankOld = fOverHeadRank;
			}
			
			if (fOverheadAgent != NULL)
			{
				fOverheadCamera.setx(fOverheadAgent->x());
                                fOverheadCamera.setz(fOverheadAgent->z());
			}
		}
		//Update the title of the overhead window with the rank, agent number and whether or not we are tracking (T) (CMB 3/26/06)
		if( fOverheadWindow && fOverheadWindow->isVisible() && fOverheadAgent)
		{
			char overheadTitle[64];
			if (fAgentTracking)
			{
			    sprintf( overheadTitle, "Overhead View (T%ld:%ld)", (long int) fOverHeadRank, fOverheadAgent->Number() );
			}else{			
			    sprintf( overheadTitle, "Overhead View (%ld:%ld)", (long int) fOverHeadRank, fOverheadAgent->Number() );
			}
			fOverheadWindow->setWindowTitle( QString(overheadTitle) );			
		}
		fOverheadWindow->Draw();
		
		// Born / (Born + Created) window
		if (fChartBorn
			&& fBirthrateWindow != NULL
		/*  && fBirthrateWindow->isVisible() */
		   )
		{
			bool newBirths;
			float birthRatio;
			if( (fHeuristicFitnessWeight != 0.0) || (fComplexityFitnessWeight != 0.0) )
			{
				newBirths = oldNumBorn != fNumberBornVirtual;
				birthRatio = float( fNumberBornVirtual ) / float( fNumberBornVirtual + fNumberCreated );
			}
			else
			{
				newBirths = oldNumBorn != fNumberBorn;
				birthRatio = float( fNumberBorn ) / float( fNumberBorn + fNumberCreated );
			}
			if( newBirths || (oldNumCreated != fNumberCreated) )
				fBirthrateWindow->AddPoint( birthRatio );
		}

		// Fitness window
		if( fChartFitness && fFitnessWindow != NULL /* && fFitnessWindow->isVisible() */ )
		{
			fFitnessWindow->AddPoint(0, fMaxFitness );
			fFitnessWindow->AddPoint(1, fCurrentMaxFitness[0] );
			fFitnessWindow->AddPoint(2, fAverageFitness );
		}
		
		// Energy flux window
		if( fChartFoodEnergy && fFoodEnergyWindow != NULL /* && fFoodEnergyWindow->isVisible() */ )
		{
			fFoodEnergyWindow->AddPoint(0, (fFoodEnergyIn - fFoodEnergyOut) / (fFoodEnergyIn + fFoodEnergyOut));
			fFoodEnergyWindow->AddPoint(1, (fTotalFoodEnergyIn - fTotalFoodEnergyOut) / (fTotalFoodEnergyIn + fTotalFoodEnergyOut));
			fFoodEnergyWindow->AddPoint(2, (fAverageFoodEnergyIn - fAverageFoodEnergyOut) / (fAverageFoodEnergyIn + fAverageFoodEnergyOut));
		}
		
		// Population window
		if( fChartPopulation && fPopulationWindow != NULL /* && fPopulationWindow->isVisible() */ )
		{				
			{
				fPopulationWindow->AddPoint(0, float(objectxsortedlist::gXSortedObjects.getCount(AGENTTYPE)));
			}				
			
			if (fNumDomains > 1)
			{
				for (int id = 0; id < fNumDomains; id++)
					fPopulationWindow->AddPoint((id + 1), float(fDomains[id].numAgents));
			}
		}

//		dbprintf( "age=%ld, rank=%d, rankOld=%d, tracking=%s, fittest=%08lx, monitored=%08lx, alive=%s\n",
//				  fStep, fMonitorAgentRank, fMonitorAgentRankOld, BoolString( fAgentTracking ), (ulong) fCurrentFittestAgent[fMonitorAgentRank-1], (ulong) fMonitorAgent, BoolString( !(!fMonitorAgent || !fMonitorAgent->Alive()) ) );

		// Brain window
		if ((fMonitorAgentRank != fMonitorAgentRankOld)
			 || (fMonitorAgentRank && !fAgentTracking && (fCurrentFittestAgent[fMonitorAgentRank - 1] != fMonitorAgent))
			 || (fMonitorAgentRank && fAgentTracking && (!fMonitorAgent || !fMonitorAgent->Alive())))
		{			
			if (fMonitorAgent != NULL)
			{
				if (fBrainMonitorWindow != NULL)
					fBrainMonitorWindow->StopMonitoring();
			}					
							
			if (fMonitorAgentRank && fBrainMonitorWindow != NULL && fBrainMonitorWindow->isVisible() )
			{
				Q_CHECK_PTR(fBrainMonitorWindow);
				fMonitorAgent = fCurrentFittestAgent[fMonitorAgentRank - 1];
				fBrainMonitorWindow->StartMonitoring(fMonitorAgent);					
			}
			else
			{
				fMonitorAgent = NULL;
			}
		
			fMonitorAgentRankOld = fMonitorAgentRank;			
		}
		//Added T for title if we are in tracking mode (CMB 3/26/06)
		if( fBrainMonitorWindow && fBrainMonitorWindow->isVisible() && fMonitorAgent && (fStep % fBrainMonitorStride == 0) )
		{
			char title[64];
			if (fAgentTracking)
			{
			    sprintf( title, "Brain Monitor (T%ld:%ld)", fMonitorAgentRank, fMonitorAgent->Number() );
			}else{			
			    sprintf( title, "Brain Monitor (%ld:%ld)", fMonitorAgentRank, fMonitorAgent->Number() );
			}			
			fBrainMonitorWindow->setWindowTitle( QString(title) );
			fBrainMonitorWindow->Draw();
		}
	}

// xxx
//sleep( 10 );

	debugcheck( "after brain monitor window in step %ld", fStep );
	
	// Archive the Recent brain function files, if we're doing that
	if( fBrainFunctionRecentRecordFrequency && fStep >= fCurrentRecentEpoch )
	{
		EndComplexityLog( fCurrentRecentEpoch );
		if( fStep < fMaxSteps )
		{
			char t[256];
			fCurrentRecentEpoch += fBrainFunctionRecentRecordFrequency;
			sprintf( t, "run/brain/Recent/%ld", fCurrentRecentEpoch );
			if( mkdir( t, PwDirMode ) )
				eprintf( "Error making %s directory (%d)\n", t, errno );
			InitComplexityLog( fCurrentRecentEpoch );
		}
	}

	// Archive the bestSoFar brain anatomy and function files, if we're doing that
	if( fBestSoFarBrainAnatomyRecordFrequency && ((fStep % fBestSoFarBrainAnatomyRecordFrequency) == 0) )
	{
		char s[256];
		int limit = fNumberDied < fNumberFit ? fNumberDied : fNumberFit;

		sprintf( s, "run/brain/bestSoFar/%ld", fStep );
		mkdir( s, PwDirMode );
		for( int i = 0; i < limit; i++ )
		{
			char t[256];	// target (use s for source)
			sprintf( s, "run/brain/anatomy/brainAnatomy_%ld_incept.txt", fFittest[i]->agentID );
			sprintf( t, "run/brain/bestSoFar/%ld/%d_brainAnatomy_%ld_incept.txt", fStep, i, fFittest[i]->agentID );
			if( link( s, t ) )
				eprintf( "Error (%d) linking from \"%s\" to \"%s\"\n", errno, s, t );
			sprintf( s, "run/brain/anatomy/brainAnatomy_%ld_birth.txt", fFittest[i]->agentID );
			sprintf( t, "run/brain/bestSoFar/%ld/%d_brainAnatomy_%ld_birth.txt", fStep, i, fFittest[i]->agentID );
			if( link( s, t ) )
				eprintf( "Error (%d) linking from \"%s\" to \"%s\"\n", errno, s, t );
			sprintf( s, "run/brain/anatomy/brainAnatomy_%ld_death.txt", fFittest[i]->agentID );
			sprintf( t, "run/brain/bestSoFar/%ld/%d_brainAnatomy_%ld_death.txt", fStep, i, fFittest[i]->agentID );
			if( link( s, t ) )
				eprintf( "Error (%d) linking from \"%s\" to \"%s\"\n", errno, s, t );
			sprintf( s, "run/brain/function/brainFunction_%ld.txt", fFittest[i]->agentID );
			sprintf( t, "run/brain/bestSoFar/%ld/%d_brainFunction_%ld.txt", fStep, i, fFittest[i]->agentID );
			if( link( s, t ) )
				eprintf( "Error (%d) linking from \"%s\" to \"%s\"\n", errno, s, t );
			
			// Generate the bestSoFar Complexity, if we're doing that.
			// Note that we calculate complexity for the Processing units only, by default,
			// but if we're using complexity as a fitness function then fFittest[i]->complexity
			// could be any of the available types of complexity (including certain differences).
// 			if(	fRecordComplexity )
// 			{
// 				if( fFittest[i]->complexity == 0.0 )		// if Complexity is zero it means we have to Calculate it
// 				{
// 					fFittest[i]->complexity = CalcComplexity_brainfunction( t, "P" );		// Complexity of Processing Units Only, all time steps
// 					cout << "[COMPLEXITY] Agent: " << fFittest[i]->agentID << "\t Processing Complexity: " << fFittest[i]->complexity << endl;
// 				}
// 			}

		}
	}
	
	// Archive the bestRecent brain anatomy and function files, if we're doing that
	if( fBestRecentBrainAnatomyRecordFrequency && ((fStep % fBestRecentBrainAnatomyRecordFrequency) == 0) )
	{
		char s[256];
		int limit = fNumberDied < fNumberRecentFit ? fNumberDied : fNumberRecentFit;

		sprintf( s, "run/brain/bestRecent/%ld", fStep );
		mkdir( s, PwDirMode );
		for( int i = 0; i < limit; i++ )
		{
			if( fRecentFittest[i]->agentID > 0 )
			{
				char t[256];	// target (use s for source)
				sprintf( s, "run/brain/anatomy/brainAnatomy_%ld_incept.txt", fRecentFittest[i]->agentID );
				sprintf( t, "run/brain/bestRecent/%ld/%d_brainAnatomy_%ld_incept.txt", fStep, i, fRecentFittest[i]->agentID );
				if( link( s, t ) )
					eprintf( "Error (%d) linking from \"%s\" to \"%s\"\n", errno, s, t );
				sprintf( s, "run/brain/anatomy/brainAnatomy_%ld_birth.txt", fRecentFittest[i]->agentID );
				sprintf( t, "run/brain/bestRecent/%ld/%d_brainAnatomy_%ld_birth.txt", fStep, i, fRecentFittest[i]->agentID );
				if( link( s, t ) )
					eprintf( "Error (%d) linking from \"%s\" to \"%s\"\n", errno, s, t );
				sprintf( s, "run/brain/anatomy/brainAnatomy_%ld_death.txt", fRecentFittest[i]->agentID );
				sprintf( t, "run/brain/bestRecent/%ld/%d_brainAnatomy_%ld_death.txt", fStep, i, fRecentFittest[i]->agentID );
				if( link( s, t ) )
					eprintf( "Error (%d) linking from \"%s\" to \"%s\"\n", errno, s, t );
				sprintf( s, "run/brain/function/brainFunction_%ld.txt", fRecentFittest[i]->agentID );
				sprintf( t, "run/brain/bestRecent/%ld/%d_brainFunction_%ld.txt", fStep, i, fRecentFittest[i]->agentID );
				if( link( s, t ) )
					eprintf( "Error (%d) linking from \"%s\" to \"%s\"\n", errno, s, t );

// 				if(	fRecordComplexity )
// 				{
// 					if( fRecentFittest[i]->complexity == 0.0 )		// if Complexity is zero it means we have to Calculate it
// 					{
// 						fRecentFittest[i]->complexity = CalcComplexity_brainfunction( t, "P" );		// Complexity of Processing Units Only, all time steps
// 						cout << "[COMPLEXITY] Agent: " << fRecentFittest[i]->agentID << "\t Processing Complexity: " << fRecentFittest[i]->complexity << endl;
// 					}
// 				}

			}
		
		}
		
		//Calculate Mean and StdDev of the fRecentFittest Complexities.
		if( fRecordComplexity )
		{
			int limit2 = limit <= fNumberRecentFit ? limit : fNumberRecentFit;
		
			double mean=0;
			double stddev=0;	//Complexity: Time to Average and StdDev the BestRecent List
			int count=0;		//Keeps a count of the number of entries in fRecentFittest.
			
			for( int i=0; i<limit2; i++ )
			{
				if( fRecentFittest[i]->agentID > 0 )
				{
//					cout << "[" <<  fStep << "] " << fRecentFittest[i]->agentID << ": " << fRecentFittest[i]->complexity << endl;
					mean += fRecentFittest[i]->complexity;		// Get Sum of all Complexities
					count++;
				}
			}
		
			mean = mean / count;			// Divide by count to get the average
		

			if( ! (mean >= 0) )			// If mean is 'nan', make it zero instead of 'nan'  -- Only true before any agents have died.
			{
				mean = 0;
				stddev = 0;
			}
			else						// Calculate the stddev (You'll do this except when before an agent has died)
			{
				for( int i=0; i<limit2; i++ )
				{
					if( fRecentFittest[i]->agentID > 0 )
					{
						stddev += pow(fRecentFittest[i]->complexity - mean, 2);		// Get Sum of all Complexities
					}
				}
			}
		
			stddev = sqrt(stddev / (count-1) );		// note that this stddev is divided by N-1 (MATLAB default)
			double StandardError = stddev / sqrt(count);
//DEBUG			cout << "Mean = " << mean << "  //  StdDev = " << stddev << endl;
			
			FILE * cFile;
			
			if( (cFile =fopen("run/brain/bestRecent/complexity.txt", "a")) == NULL )
			{
				cerr << "could not open run/brain/bestRecent/complexity.txt for writing" << endl;
				exit(1);
			}
			
			// print to complexity.txt
			fprintf( cFile, "%ld %f %f %f %d\n", fStep, mean, stddev, StandardError, count );
			fclose( cFile );
			
		}

		// Now delete all bestRecent agent files, unless they are also on the bestSoFar list
		// Also empty the bestRecent list here, so we start over each epoch
		int limit2 = fNumberDied < fNumberFit ? fNumberDied : fNumberFit;
		for( int i = 0; i < limit; i++ )
		{
			if( !fBrainAnatomyRecordAll || !fBrainFunctionRecordAll )
			{
				// First determine whether or not each bestRecent agent is in the bestSoFar list or not
				bool inBestSoFarList = false;
				for( int j = 0; j < limit2; j++ )
				{
					if( fRecentFittest[i]->agentID == fFittest[j]->agentID )
					{
						inBestSoFarList = true;
						break;
					}
				}
				
				// If each bestRecent agent is NOT in the bestSoFar list, then unlink all its files from their original location
				if( !inBestSoFarList && (fRecentFittest[i]->agentID > 0) )
				{
					if( (fBestRecentBrainAnatomyRecordFrequency || fBestSoFarBrainAnatomyRecordFrequency) && !fBrainAnatomyRecordAll )
					{
						sprintf( s, "run/brain/anatomy/brainAnatomy_%ld_incept.txt", fRecentFittest[i]->agentID );
						if( unlink( s ) )
							eprintf( "Error (%d) unlinking \"%s\"\n", errno, s );
						sprintf( s, "run/brain/anatomy/brainAnatomy_%ld_birth.txt", fRecentFittest[i]->agentID );
						if( unlink( s ) )
							eprintf( "Error (%d) unlinking \"%s\"\n", errno, s );
						sprintf( s, "run/brain/anatomy/brainAnatomy_%ld_death.txt", fRecentFittest[i]->agentID );
						if( unlink( s ) )
							eprintf( "Error (%d) unlinking \"%s\"\n", errno, s );
					}
					if( (fBestRecentBrainFunctionRecordFrequency || fBestSoFarBrainFunctionRecordFrequency) && !fBrainFunctionRecordAll && !fBrainFunctionRecentRecordFrequency )
					{
						sprintf( s, "run/brain/function/brainFunction_%ld.txt", fRecentFittest[i]->agentID );
						if( unlink( s ) )
							eprintf( "Error (%d) unlinking \"%s\"\n", errno, s );
					}
				}
			}
			
			// Empty the bestRecent list by zeroing out agent IDs and fitnesses
			fRecentFittest[i]->agentID = 0;
			fRecentFittest[i]->fitness = 0.0;
			fRecentFittest[i]->complexity = 0.0;
		}

	}
	
	if( fRecordAdamiComplexity && ((fStep % fAdamiComplexityRecordFrequency) == 0) )		// lets compute our AdamiComplexity -- 1-bit window
	{
			unsigned int genevalue;
			FILE * FileOneBit;
			FILE * FileTwoBit;
			FILE * FileFourBit;
			FILE * FileSummary;

			float SumInformationOneBit = 0;
			float SumInformationTwoBit = 0;
			float SumInformationFourBit = 0;
			
			float entropyOneBit[8];
			float informationOneBit[8];
			
			float entropyTwoBit[4];
			float informationTwoBit[4];
			float entropyFourBit[2];
			float informationFourBit[2];
						
			agent* c = NULL;

			if( (FileOneBit = fopen("run/genome/AdamiComplexity-1bit.txt", "a")) == NULL )
			{
				cerr << "could not open run/genome/AdamiComplexity-1bit.txt for writing [2].  Exiting." << endl;
				exit(1);
			}
			if( (FileTwoBit = fopen("run/genome/AdamiComplexity-2bit.txt", "a")) == NULL )
			{
				cerr << "could not open run/genome/AdamiComplexity-2bit.txt for writing [2].  Exiting." << endl;
				exit(1);
			}		
			if( (FileFourBit = fopen("run/genome/AdamiComplexity-4bit.txt", "a")) == NULL )
			{
				cerr << "could not open run/genome/AdamiComplexity-4bit.txt for writing [2].  Exiting." << endl;
				exit(1);
			}
			if( (FileSummary = fopen("run/genome/AdamiComplexity-summary.txt", "a")) == NULL )
			{
				cerr << "could not open run/genome/AdamiComplexity-summary.txt for writing [2].  Exiting." << endl;
				exit(1);
			}


			fprintf( FileOneBit,  "%ld:", fStep );		// write the timestep on the beginning of the line
			fprintf( FileTwoBit,  "%ld:", fStep );		// write the timestep on the beginning of the line
			fprintf( FileFourBit, "%ld:", fStep );		// write the timestep on the beginning of the line
			fprintf( FileSummary, "%ld ", fStep );		// write the timestep on the beginning of the line
			
			int numagents = objectxsortedlist::gXSortedObjects.getCount(AGENTTYPE);

			bool bits[numagents][8];
		
			for( int gene = 0, n = GenomeUtil::schema->getMutableSize(); gene < n; gene++ )			// for each gene ...
			{
				int count = 0;

				objectxsortedlist::gXSortedObjects.reset();				
		
				while( objectxsortedlist::gXSortedObjects.nextObj( AGENTTYPE, (gobject**) &c ) )	// for each agent ...
				{
					genevalue = c->Genes()->get_raw_uint(gene);

					
					if( genevalue >= 128 ) { bits[count][0]=true; genevalue -= 128; } else { bits[count][0] = false; }
					if( genevalue >= 64  ) { bits[count][1]=true; genevalue -= 64; }  else { bits[count][1] = false; }
					if( genevalue >= 32  ) { bits[count][2]=true; genevalue -= 32; }  else { bits[count][2] = false; }
					if( genevalue >= 16  ) { bits[count][3]=true; genevalue -= 16; }  else { bits[count][3] = false; }
					if( genevalue >= 8   ) { bits[count][4]=true; genevalue -= 8; }   else { bits[count][4] = false; }
					if( genevalue >= 4   ) { bits[count][5]=true; genevalue -= 4; }   else { bits[count][5] = false; }
					if( genevalue >= 2   ) { bits[count][6]=true; genevalue -= 2; }   else { bits[count][6] = false; }
					if( genevalue == 1   ) { bits[count][7]=true; }                   else { bits[count][7] = false; }

					count++;
				}
				
				
				/* PRINT THE BYTE UNDER CONSIDERATION */
				
				/* DOING ONE BIT WINDOW */
				for( int i=0; i<8; i++ )		// for each window 1-bits wide...
				{
					int number_of_ones=0;
					
					for( int agent=0; agent<numagents; agent++ )
						if( bits[agent][i] == true ) { number_of_ones++; }		// if agent has a 1 in the column, increment number_of_ones
										
					float prob_1 = (float) number_of_ones / (float) numagents;
					float prob_0 = 1.0 - prob_1;
					float logprob_0, logprob_1;
					
					if( prob_1 == 0.0 ) logprob_1 = 0.0;
					else logprob_1 = log2(prob_1);

					if( prob_0 == 0.0 ) logprob_0 = 0.0;
					else logprob_0 = log2(prob_0);

					
					entropyOneBit[i] = -1 * ( (prob_0 * logprob_0) + (prob_1 * logprob_1) );			// calculating the entropy for this bit...
					informationOneBit[i] = 1.0 - entropyOneBit[i];										// 1.0 = maxentropy
					SumInformationOneBit += informationOneBit[i];
//DEBUG				cout << "Gene" << gene << "_bit[" << i << "]: probs =\t{" << prob_0 << "," << prob_1 << "}\tEntropy =\t" << entropyOneBit[i] << " bits\t\tInformation =\t" << informationOneBit[i] << endl;
				}
			
				fprintf( FileOneBit, " %.4f %.4f %.4f %.4f %.4f %.4f %.4f %.4f", informationOneBit[0], informationOneBit[1], informationOneBit[2], informationOneBit[3], informationOneBit[4], 
					informationOneBit[5], informationOneBit[6], informationOneBit[7] );

				/* DOING TWO BIT WINDOW */

				for( int i=0; i<4; i++ )		// for each window 2-bits wide...
				{
					int number_of[4];
					for( int j=0; j<4; j++) { number_of[j] = 0; }		// zero out the array

					for( int agent=0; agent<numagents; agent++ )
					{
						if( bits[agent][i*2] )			// Bits: 1 ?
						{
							if( bits[agent][(i*2)+1] ) { number_of[3]++; }	// Bits: 1 1
							else						 { number_of[2]++; }		// Bits: 1 0							
						}
						else							// Bits: 0 ?
						{
							if( bits[agent][(i*2)+1] ) { number_of[1]++; }		// Bits: 0 1
							else						 { number_of[0]++; }		// Bits: 0 0
						}
					}



					float prob[4];
					float logprob[4];
					float sum=0;
					
					for( int j=0; j<4; j++ )
					{
						prob[j] = (float) number_of[j] / (float) numagents;
						if( prob[j] == 0.0 ) { logprob[j] = 0.0; }
						else { logprob[j] = log2(prob[j]); }						// in units of mers

						sum += prob[j] * logprob[j];						// H(X) = -1 * SUM{ p(x) * log p(x) }
					}

					entropyTwoBit[i] = (sum * -1);							// multiplying the sum by -1 to get the Entropy
					informationTwoBit[i] = 2.0 - entropyTwoBit[i];			// since we're in units of mers, maxentroopy is always 1.0
					SumInformationTwoBit += informationTwoBit[i];

//DEBUG				cout << "Gene" << gene << "_window2bit[" << i << "]: probs =\t{" << prob[0] << "," << prob[1] << "," << prob[2] << "," << prob[3] << "}\tEntropy =\t" << entropyTwoBit[i] << " mers\t\tInformation =\t" << informationTwoBit[i] << endl;
				}
				
				fprintf( FileTwoBit, " %.4f %.4f %.4f %.4f", informationTwoBit[0], informationTwoBit[1], informationTwoBit[2], informationTwoBit[3] );


				/* DOING FOUR BIT WINDOW */

				for( int i=0; i<2; i++ )		// for each window four-bits wide...
				{
					int number_of[16];
					for( int j=0; j<16; j++) { number_of[j] = 0; }		// zero out the array

					
					for( int agent=0; agent<numagents; agent++ )
					{
						if( bits[agent][i*4] )					// 1 ? ? ?.  Possibilities are 8-15
						{
							if( bits[agent][(i*4)+1] )			// 1 1 ? ?.  Possibilities are 12-15
							{
								if( bits[agent][(i*4)+2] )		// 1 1 1 ?.  Possibilities are 14-15
								{
									if( bits[agent][(i*4)+3] )	{ number_of[15]++; }	// 1 1 1 1
									else							{ number_of[14]++; }	// 1 1 1 0
								}
								else								// 1 1 0 ?.  Possibilities are 12-13
								{
									if( bits[agent][(i*4)+3] )	{ number_of[13]++; }	// 1 1 0 1
									else							{ number_of[12]++; }	// 1 1 0 0
								}
							}
							else									// 1 0 ? ?.  Possibilities are 8-11
							{
								if( bits[agent][(i*4)+2] )		// 1 0 1 ?.  Possibilities are 14-15
								{
									if( bits[agent][(i*4)+3] )	{ number_of[11]++; }	// 1 0 1 1
									else							{ number_of[10]++; }	// 1 0 1 0
								}
								else								// 1 0 0 ?.  Possibilities are 12-13
								{
									if( bits[agent][(i*4)+3] )	{ number_of[9]++; }		// 1 0 0 1
									else							{ number_of[8]++; }		// 1 0 0 0
								}
							}
						}
						else										// 0 ? ? ?.  Possibilities are 0-7
						{
							if( bits[agent][(i*4)+1] )			// 0 1 ? ?.  Possibilities are 4-8
							{
								if( bits[agent][(i*4)+2] )		// 0 1 1 ?.  Possibilities are 6-7
								{
									if( bits[agent][(i*4)+3] )	{ number_of[7]++; } // 0 1 1 1
									else							{ number_of[6]++; }	// 0 1 0 0
								}
								else								// 0 1 0 ?.  Possibilities are 4-5
								{
									if( bits[agent][(i*4)+3] )	{ number_of[5]++; } // 0 1 0 1
									else							{ number_of[4]++; }	// 0 1 0 0
								}
							}
							else									// 0 0 ? ?.  Possibilities are 0-3
							{
								if( bits[agent][(i*4)+2] )		// 0 0 1 ?.  Possibilities are 2-3
								{
									if( bits[agent][(i*4)+3] )	{ number_of[3]++; } // 0 0 1 1
									else							{ number_of[2]++; }	// 0 0 1 0
								}
								else								// 0 0 0 ?.  Possibilities are 0-1
								{
									if( bits[agent][(i*4)+3] )	{ number_of[1]++; } // 0 0 0 1
									else							{ number_of[0]++; }	// 0 0 0 0
								}							
							}
						}
					}
					
					float prob[16];
					float logprob[16];
					float sum=0;
					
					for( int j=0; j<16; j++ )
					{
						prob[j] = (float) number_of[j] / (float) numagents;
						if( prob[j] == 0.0 ) { logprob[j] = 0.0; }
						else { logprob[j] = log2(prob[j]); }

						sum += prob[j] * logprob[j];						// H(X) = -1 * SUM{ p(x) * log p(x) }
					}
					
					entropyFourBit[i] = (sum * -1);							// multiplying the sum by -1 to get the Entropy
					informationFourBit[i] = 4.0 - entropyFourBit[i];		// since we're in units of mers, maxentroopy is always 1.0
					SumInformationFourBit += informationFourBit[i];
//DEBUG				cout << "Gene" << gene << "_window4bit[" << i << "]: probs =\t{" << prob[0] << "," << prob[1] << "," << prob[2] << "," << prob[3] << "," << prob[4] << "," << 
//DEBUG					prob[5] << "," << prob[6] << "," << prob[7] << "," << prob[8] << "," << prob[9] << "," << prob[10] << "," << prob[11] << "," << 
//DEBUG					prob[12] << "," << prob[13] << "," << prob[14] << "," << prob[15] << "}\tEntropy =\t" << entropyFourBit[i] << " mers\t\tInformation =\t" << informationFourBit[i] << endl;
				}

				fprintf( FileFourBit, " %.4f %.4f", informationFourBit[0], informationFourBit[1] );

			}

			fprintf( FileOneBit,  "\n" );		// end the line
			fprintf( FileTwoBit,  "\n" );		// end the line
			fprintf( FileFourBit, "\n" );		// end the line
			fprintf( FileSummary, "%.4f %.4f %.4f\n", SumInformationOneBit, SumInformationTwoBit, SumInformationFourBit );		// write the timestep on the beginning of the line

			// Done computing AdamiComplexity.  Close our log files.
			fclose( FileOneBit );
			fclose( FileTwoBit );
			fclose( FileFourBit );
			fclose( FileSummary );
	}


	// Handle tracking gene Separation
	if( fMonitorGeneSeparation && fRecordGeneSeparation )
		RecordGeneSeparation();

	//Rotate the world a bit each time step... (CMB 3/10/06)
	if (fRotateWorld)
	{
		fCameraAngle += fCameraRotationRate;
		float camrad = fCameraAngle * DEGTORAD;
		fCamera.settranslation((0.5+fCameraRadius*sin(camrad))*globals::worldsize, fCameraHeight*globals::worldsize,(-.5+fCameraRadius*cos(camrad))*
globals::worldsize);
	}
}

//---------------------------------------------------------------------------
// TSimulation::End
//---------------------------------------------------------------------------
void TSimulation::End( const string &reason )
{
	agent *a;

	objectxsortedlist::gXSortedObjects.reset();
	while (objectxsortedlist::gXSortedObjects.nextObj(AGENTTYPE, (gobject**)&a))
	{
		Kill( a, LifeSpan::DR_SIMEND );
	}	

	EndSeparationsLog();
	EndLifeSpanLog();
	EndContactsLog();
	EndCollisionsLog();
	EndCarryLog();
	EndEnergyLog();

	if( fRecordMovie )
	{
		fMovieWriter->close();
	}

	{
		barrier* b;
		barrier::gXSortedBarriers.reset();
		while( barrier::gXSortedBarriers.next( b ) )
			delete b;
	}

	fConditionalProps->dispose();

	printf( "Simulation stopped after step %ld\n", fStep );

	{
		ofstream fout( "run/endStep.txt" );
		fout << fStep << endl;
		fout.close();
	}

	{
		ofstream fout( "run/endReason.txt" );
		fout << reason << endl;
		fout.close();
	}

	// TODO: Clean up the dispose path.
	// Deleting the SceneWindow deletes *this*.
	delete fSceneWindow;
	
	exit( 0 );
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


//---------------------------------------------------------------------------
// TSimulation::Init
//
// The order that initializer functions are called is important as values
// may depend on a variable initialized earlier.
//---------------------------------------------------------------------------
void TSimulation::Init( const char *argWorldfilePath, const bool statusToStdout )
{
 	// Set up graphical constructs
	Resources::loadPolygons( &fGround, "ground" );

    srand(1);

	fGraphics = true;
    InitWorld();

	proplib::Document *docWorldFile;
	proplib::Document *docSchema;
	Resources::parseWorldFile( &docWorldFile, &docSchema, argWorldfilePath );
	cout << "worldfile = " << docWorldFile->getName() << endl;

	ProcessWorldFile( docWorldFile );
	
	if( statusToStdout )	// command-line arg overrides worldfile only if true
		fStatusToStdout = true;

	// Init array tracking number of agents with a given metabolism
	assert( Metabolism::getNumberOfDefinitions() < MAXMETABOLISMS );
	for( int i = 0; i < Metabolism::getNumberOfDefinitions(); i++ )
	{
		fNumberAliveWithMetabolism[i] = 0;
	}
	
	if( fStaticTimestepGeometry )
	{
		// Brains execute in parallel, so we need brain-local RNG state
		RandomNumberGenerator::set( RandomNumberGenerator::NERVOUS_SYSTEM,
									RandomNumberGenerator::LOCAL );
	}

	InitNeuralValues();	 // Must be called before genome and brain init
	
    Brain::braininit();
    agent::agentinit();
	
	GenomeUtil::createSchema();
	
	 // Following is part of one way to speed up the graphics
	 // Note:  this code must agree with the agent sizing in agent::grow()
	 // and the food sizing in food::initlen().

	float maxagentlenx = agent::gMaxAgentSize / sqrt(genome::gMinmaxspeed);
	float maxagentlenz = agent::gMaxAgentSize * sqrt(genome::gMaxmaxspeed);
	float maxagentradius = 0.5 * sqrt(maxagentlenx*maxagentlenx + maxagentlenz*maxagentlenz);
	float maxfoodlen = 0.75 * food::gMaxFoodEnergy / food::gSize2Energy;
	float maxfoodradius = 0.5 * sqrt(maxfoodlen * maxfoodlen * 2.0);
	food::gMaxFoodRadius = maxfoodradius;
	agent::gMaxRadius = maxagentradius > maxfoodradius ?
						  maxagentradius : maxfoodradius;

    InitMonitoringWindows();

    if( fNumberFit > 0 )
    {
        fFittest = new FitStruct*[fNumberFit];
		Q_CHECK_PTR( fFittest );

        for (int i = 0; i < fNumberFit; i++)
        {
			fFittest[i] = new FitStruct;
			Q_CHECK_PTR( fFittest[i] );
            fFittest[i]->genes = GenomeUtil::createGenome();
			Q_CHECK_PTR( fFittest[i]->genes );
            fFittest[i]->fitness = 0.0;
			fFittest[i]->agentID = 0;
			fFittest[i]->complexity = 0.0;
        }
		
        fRecentFittest = new FitStruct*[fNumberRecentFit];
		Q_CHECK_PTR( fRecentFittest );

        for( int i = 0; i < fNumberRecentFit; i++ )
        {
			fRecentFittest[i] = new FitStruct;
			Q_CHECK_PTR( fRecentFittest[i] );
            fRecentFittest[i]->genes = NULL;	// GenomeUtil::createGenome()->legacy;	// we don't save the genes in the bestRecent list
            fRecentFittest[i]->fitness = 0.0;
			fRecentFittest[i]->agentID = 0;
			fRecentFittest[i]->complexity = 0.0;
        }
		
        for( int id = 0; id < fNumDomains; id++ )
        {
            fDomains[id].fittest = new FitStruct*[fNumberFit];
			Q_CHECK_PTR( fDomains[id].fittest );

            for (int i = 0; i < fNumberFit; i++)
            {
				fDomains[id].fittest[i] = new FitStruct;
				Q_CHECK_PTR( fDomains[id].fittest[i] );
                fDomains[id].fittest[i]->genes = GenomeUtil::createGenome();
				Q_CHECK_PTR( fDomains[id].fittest[i]->genes );
                fDomains[id].fittest[i]->fitness = 0.0;
				fDomains[id].fittest[i]->agentID = 0;
				fDomains[id].fittest[i]->complexity = 0.0;
            }
        }
    }

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

	// Set up the run directory and its subsidiaries
	char s[256];
	char t[256];

#define MKDIR(PATH)												\
	if( mkdir( PATH, PwDirMode ) )								\
		eprintf( "Error making %s directory (%d)\n", PATH, errno );

	// First save the old directory, if it exists
	sprintf( s, "run" );
	sprintf( t, "run_%ld", time(NULL) );
	(void) rename( s, t );

	MKDIR( "run" );
	MKDIR( "run/stats" );

	MKDIR( "run/genome" );
	MKDIR( "run/genome/meta" );
	if( fRecordGenomes )
	{
		MKDIR( "run/genome/agents" );
	}

	if( fRecordPosition || fPositionSeedsFromFile || fRecordBarrierPosition )
	{
		MKDIR( "run/motion" );
		MKDIR( "run/motion/position" );
		if( fRecordPosition )
			MKDIR( "run/motion/position/agents" );
		if( fRecordBarrierPosition )
			MKDIR( "run/motion/position/barriers" );
	}

	if( fRecordContacts || fRecordCollisions || fRecordCarry || fRecordEnergy )
	{
		MKDIR( "run/events" );
	}

	if( fConditionalProps->isLogging() )
	{
		MKDIR( "run/condprop" );
	}
	fConditionalProps->init();

	if( fBestSoFarBrainAnatomyRecordFrequency || fBestSoFarBrainFunctionRecordFrequency ||
		fBestRecentBrainAnatomyRecordFrequency || fBestRecentBrainFunctionRecordFrequency ||
		fBrainAnatomyRecordAll || fBrainFunctionRecordAll || fBrainFunctionRecentRecordFrequency ||
		fBrainAnatomyRecordSeeds || fBrainFunctionRecordSeeds || fAdamiComplexityRecordFrequency || fRecordPosition)
	{
		int agent_factor = 0;

		if( RecordBrainFunction( 1 ) ) agent_factor++;
		if( fRecordPosition ) agent_factor++;

		int nfiles = 100 + (fMaxNumAgents * agent_factor);

		// If we're going to be saving info on all these files, must increase the number allowed open
		if( SetMaximumFiles( nfiles ) )
		  {
		    eprintf( "Error setting maximum files to %d (%d) -- consult ulimit\n", nfiles, errno );
		    //exit(1);
		  }

		if( mkdir( "run/brain", PwDirMode ) )
			eprintf( "Error making run/brain directory (%d)\n", errno );
			
			
	#define RecordRandomBrainAnatomies 0
	#if RecordRandomBrainAnatomies
		if( mkdir( "run/brain/random", PwDirMode ) )
			eprintf( "Error making run/brain/random directory (%d)\n", errno );
	#endif
		if( fBestSoFarBrainAnatomyRecordFrequency || fBestRecentBrainAnatomyRecordFrequency || fBrainAnatomyRecordAll || fBrainAnatomyRecordSeeds )
			if( mkdir( "run/brain/anatomy", PwDirMode ) )
				eprintf( "Error making run/brain/anatomy directory (%d)\n", errno );
		if( fBrainFunctionRecentRecordFrequency || fBestSoFarBrainFunctionRecordFrequency || fBestRecentBrainFunctionRecordFrequency || fBrainFunctionRecordAll || fBrainFunctionRecordSeeds )
			if( mkdir( "run/brain/function", PwDirMode ) )
				eprintf( "Error making run/brain/function directory (%d)\n", errno );
		if( fBestSoFarBrainAnatomyRecordFrequency || fBestSoFarBrainFunctionRecordFrequency )
			if( mkdir( "run/brain/bestSoFar", PwDirMode ) )
				eprintf( "Error making run/brain/bestSoFar directory (%d)\n", errno );
		if( fBestRecentBrainAnatomyRecordFrequency || fBestRecentBrainFunctionRecordFrequency )
			if( mkdir( "run/brain/bestRecent", PwDirMode ) )
				eprintf( "Error making run/brain/bestRecent directory (%d)\n", errno );
		if( fBrainAnatomyRecordSeeds || fBrainFunctionRecordSeeds )
		{
			if( mkdir( "run/brain/seeds", PwDirMode ) )
				eprintf( "Error making run/brain/seeds directory (%d)\n", errno );
			if( fBrainAnatomyRecordSeeds )
				if( mkdir( "run/brain/seeds/anatomy", PwDirMode ) )
					eprintf( "Error making run/brain/seeds/anatomy directory (%d)\n", errno );
			if( fBrainFunctionRecordSeeds )
				if( mkdir( "run/brain/seeds/function", PwDirMode ) )
					eprintf( "Error making run/brain/seeds/function directory (%d)\n", errno );
		}
		if( fBrainFunctionRecentRecordFrequency )
		{
			if( mkdir( "run/brain/Recent", PwDirMode ) )
				eprintf( "Error making run/brain/Recent directory (%d)\n", errno );
			if( mkdir( "run/brain/Recent/0", PwDirMode ) )
				eprintf( "Error making run/brain/Recent/0 directory (%d)\n", errno );
			fCurrentRecentEpoch = fBrainFunctionRecentRecordFrequency;
			sprintf( t, "run/brain/Recent/%ld", fCurrentRecentEpoch );
			if( mkdir( t, PwDirMode ) )
				eprintf( "Error making %s directory (%d)\n", t, errno );
		}
	}


	if( fRecordAdamiComplexity )
	{
		FILE * File;
		
		if( (File = fopen("run/genome/AdamiComplexity-1bit.txt", "a")) == NULL )
		{
			cerr << "could not open run/genome/AdamiComplexity-1bit.txt for writing [1]. Exiting." << endl;
			exit(1);
		}
		fprintf( File, "%% BitsInGenome: %d WindowSize: 1\n", GenomeUtil::schema->getMutableSize() * 8 );		// write the number of bits into the top of the file.
		fclose( File );

		if( (File = fopen("run/genome/AdamiComplexity-2bit.txt", "a")) == NULL )
		{
			cerr << "could not open run/genome/AdamiComplexity-2bit.txt for writing [1]. Exiting." << endl;
			exit(1);
		}
		fprintf( File, "%% BitsInGenome: %d WindowSize: 2\n", GenomeUtil::schema->getMutableSize() * 8 );		// write the number of bits into the top of the file.
		fclose( File );


		if( (File = fopen("run/genome/AdamiComplexity-4bit.txt", "a")) == NULL )
		{
			cerr << "could not open run/genome/AdamiComplexity-4bit.txt for writing [1]. Exiting." << endl;
			exit(1);
		}
		fprintf( File, "%% BitsInGenome: %d WindowSize: 4\n", GenomeUtil::schema->getMutableSize() * 8 );		// write the number of bits into the top of the file.
		fclose( File );

		if( (File = fopen("run/genome/AdamiComplexity-summary.txt", "a")) == NULL )
		{
			cerr << "could not open run/genome/AdamiComplexity-summary.txt for writing [1]. Exiting." << endl;
			exit(1);
		}
		fprintf( File, "%% Timestep 1bit 2bit 4bit\n" );
		fclose( File );
	}
	
	if( fRecordBirthsDeaths )
	{
		FILE * File;
		if( (File = fopen("run/BirthsDeaths.log", "a")) == NULL )
		{
			cerr << "could not open run/BirthsDeaths.log for writing [1]. Exiting." << endl;
			exit(1);
		}
		fprintf( File, "%% Timestep Event Agent# Parent1 Parent2\n" );
		fclose( File );
	}

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

	// If we're going to record the gene means and std devs, we need to allocate a couple of stat arrays
	if( fRecordGeneStats )
	{
		fGeneSum  = (unsigned long*) malloc( sizeof( *fGeneSum  ) * GenomeUtil::schema->getMutableSize() );
		Q_CHECK_PTR( fGeneSum );
		fGeneSum2 = (unsigned long*) malloc( sizeof( *fGeneSum2 ) * GenomeUtil::schema->getMutableSize() );
		Q_CHECK_PTR( fGeneSum2 );
		fGeneStatsAgents = (agent**) malloc( sizeof( *fGeneStatsAgents ) * GetMaxAgents() );
		
		fGeneStatsFile = fopen( "run/genome/genestats.txt", "w" );
		Q_CHECK_PTR( fGeneStatsFile );
		
		fprintf( fGeneStatsFile, "%d\n", GenomeUtil::schema->getMutableSize() );
	}

#define PrintGeneIndexesFlag 1
#if PrintGeneIndexesFlag
	{
		FILE* f = fopen( "run/genome/meta/geneindex.txt", "w" );
		Q_CHECK_PTR( f );
		
		GenomeUtil::schema->printIndexes( f );
		
		fclose( f );
	}
	{
		FILE* f = fopen( "run/genome/meta/genelayout.txt", "w" );
		Q_CHECK_PTR( f );
		
		GenomeUtil::schema->printIndexes( f, GenomeUtil::layout );
		
		fclose( f );

		SYSTEM( "cat run/genome/meta/genelayout.txt | sort -n > run/genome/meta/genelayout-sorted.txt" );
	}
	{
		FILE* f = fopen( "run/genome/meta/genetitle.txt", "w" );
		Q_CHECK_PTR( f );
		
		GenomeUtil::schema->printTitles( f );
		
		fclose( f );
	}
	{
		FILE* f = fopen( "run/genome/meta/generange.txt", "w" );
		Q_CHECK_PTR( f );
		
		GenomeUtil::schema->printRanges( f );
		
		fclose( f );		
	}
#endif

	//If we're recording the number of agents in or near various foodpatches, then open the stat file
	if( fRecordFoodPatchStats )
	{
			fFoodPatchStatsFile = fopen( "run/foodpatchstats.txt", "w" );
			Q_CHECK_PTR( fFoodPatchStatsFile );
	}
	
	{
		if( docWorldFile->getPath() != "" )
		{
			SYSTEM( ("cp " + docWorldFile->getPath() + " run/original.wf").c_str() );
		}
		SYSTEM( ("cp " + docSchema->getPath() + " run/original.wfs").c_str() );

		{
			ofstream out( "run/normalized.wf" );
			docWorldFile->write( out );
		}

		{
			ofstream out( "run/reduced.wf" );
			proplib::Schema::reduce( docSchema, docWorldFile, false );
			docWorldFile->write( out );
		}
		{
			ofstream out( "run/normalized.wfs" );
			docSchema->write( out );
		}

		delete docWorldFile;
		delete docSchema;
	}

    // Pass ownership of the cast to the stage [TODO] figure out ownership issues
    fStage.SetCast(&fWorldCast);

	fFoodEnergyIn = 0.0;
	fFoodEnergyOut = 0.0;
	fEnergyEaten.zero();

	srand48(fGenomeSeed);

	if (!fLoadState)
	{
		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
		// ^^^ MASTER TASK ExecInitAgents
		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
		class ExecInitAgents : public ITask
		{
		public:
			virtual void task_exec( TSimulation *sim )
			{
				sim->InitAgents();
			}
		} execInitAgents;

		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		// !!! EXEC MASTER
		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		fScheduler.execMasterTask( this,
								   execInitAgents,
								   !fParallelInitAgents );
			
		// Add food to the food patches until they each have their initFoodCount number of food pieces
		for( int domainNumber = 0; domainNumber < fNumDomains; domainNumber++ )
		{
			fDomains[domainNumber].numFoodPatchesGrown = 0;
			
			for( int foodPatchNumber = 0; foodPatchNumber < fDomains[domainNumber].numFoodPatches; foodPatchNumber++ )
			{
				if( fDomains[domainNumber].fFoodPatches[foodPatchNumber].on( 0 ) )
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

		for( int domainNumber = 0; domainNumber < fNumDomains; domainNumber++ )
		{
			for( int brickPatchNumber = 0; brickPatchNumber < fDomains[domainNumber].numBrickPatches; brickPatchNumber++ )
			{
				fDomains[domainNumber].fBrickPatches[brickPatchNumber].updateOn( 0 );
			}
		}
	}

	fEatStatistics.Init();

    fTotalFoodEnergyIn = fFoodEnergyIn;
    fTotalFoodEnergyOut = fFoodEnergyOut;
    fTotalEnergyEaten = fEnergyEaten;
    fAverageFoodEnergyIn = 0.0;
    fAverageFoodEnergyOut = 0.0;

    fGround.sety(-fGroundClearance);
    fGround.setscale(globals::worldsize);
    fGround.setcolor(fGroundColor);
    fWorldSet.Add(&fGround);
    fStage.SetSet(&fWorldSet);

	// Add barriers
	barrier* b = NULL;
	while( barrier::gXSortedBarriers.next(b) )
		fWorldSet.Add(b);

	// Set up scene and camera
	fScene.SetStage(&fStage);
	fScene.SetCamera(&fCamera);
	fCamera.SetPerspective(fCameraFOV, (float)fSceneView->width() / (float)fSceneView->height(), 0.01, 1.5 * globals::worldsize);	
	
	//The main camera will rotate around the world, so we need to set up the angle and translation  (CMB 03/10/06)
	fCameraAngle = fCameraAngleStart;
	float camrad = fCameraAngle * DEGTORAD;
	fCamera.settranslation((0.5 + fCameraRadius * sin(camrad)) * globals::worldsize,
						fCameraHeight * globals::worldsize, (-0.5 + fCameraRadius * cos(camrad)) * globals::worldsize);

//	fCamera.SetRotation(0.0, -fCameraFOV / 3.0, 0.0);	
	fCamera.setcolor(fCameraColor);
	fCamera.UseLookAt();
	fCamera.SetFixationPoint(0.5 * globals::worldsize, 0.0, -0.5 * globals::worldsize);	
	fCamera.SetRotation(0.0, 90, 0.0);  	
//	fStage.AddObject(&fCamera);
	
	//Set up the overhead camera view (CMB 3/13/06)
	// Set up overhead scene and overhead camera
	fOverheadScene.SetStage(&fStage);
	fOverheadScene.SetCamera(&fOverheadCamera);

	//Set up the overhead camera (CMB 3/13/06)
	fOverheadCamera.setcolor(fCameraColor);
//	fOverheadCamera.SetFog(false, glFogFunction(), glExpFogDensity(), glLinearFogEnd() );
	fOverheadCamera.SetPerspective(fCameraFOV, (float)fSceneView->width() / (float)fSceneView->height(),0.01, 1.5 * globals::worldsize);
	fOverheadCamera.settranslation(0.5*globals::worldsize, 0.2*globals::worldsize,-0.5*globals::worldsize);
	fOverheadCamera.SetRotation(0.0, -fCameraFOV, 0.0);
	//fOverheadCamera.UseLookAt();

	//Add the overhead camera into the scene (CMB 3/13/06)
//	fStage.AddObject(&fOverheadCamera);

	// Add scene to scene view and to overhead view
	Q_CHECK_PTR(fSceneView);
	fSceneView->SetScene(&fScene);
	fOverheadWindow->SetScene( &fOverheadScene );  //Set up overhead view (CMB 3/13/06)

#define DebugGenetics false
#if DebugGenetics
	// This little snippet of code confirms that genetic copying, crossover, and mutation are behaving somewhat reasonably
	objectxsortedlist::gXSortedObjects.reset();
	agent* c1 = NULL;
	agent* c2 = NULL;
	Genome* g1 = NULL;
	Genome* g2 = NULL;
	Genome g1c(GenomeUtil::schema), g1x2(GenomeUtil::schema), g1x2m(GenomeUtil::schema);
	objectxsortedlist::gXSortedObjects.nextObj(AGENTTYPE, (gobject**)&c1 );
	objectxsortedlist::gXSortedObjects.nextObj(AGENTTYPE, (gobject**)&c2 );
	g1 = c1->Genes();
	g2 = c2->Genes();
	cout << "************** G1 **************" nl;
	g1->print();
	cout << "************** G2 **************" nl;
	g2->print();
	g1c.copyFrom( g1 );
	g1x2.crossover( g1, g2, false );
	g1x2m.crossover( g1, g2, true );
	cout << "************** G1c **************" nl;
	g1c.print();
	cout << "************** G1x2 **************" nl;
	g1x2.print();
	cout << "************** G1x2m **************" nl;
	g1x2m.print();
	exit(0);
#endif
	
	// Set up gene Separation monitoring
	if (fMonitorGeneSeparation)
    {
		fGeneSepVals = new float[fMaxNumAgents * (fMaxNumAgents - 1) / 2];
        fNumGeneSepVals = 0;
        CalculateGeneSeparationAll();

        if (fRecordGeneSeparation)
        {
            fGeneSeparationFile = fopen("run/genesep", "w");
	    	RecordGeneSeparation();
        }

		if (fChartGeneSeparation && fGeneSeparationWindow != NULL)
			fGeneSeparationWindow->AddPoint(fGeneSepVals, fNumGeneSepVals);
    }
	
	// Set up to record a movie, if requested
	if( fRecordMovie )
	{
		char	movieFileName[256] = "run/movie.pmv";	// put date and time into the name TODO
				
		FILE *movieFile = fopen( movieFileName, "wb" );
		if( !movieFile )
		{
			eprintf( "Error opening movie file: %s\n", movieFileName );
			exit( 1 );
		}
		
		fMovieWriter = new PwMovieWriter( movieFile );
		fSceneView->SetMovieWriter( fMovieWriter );
	}

	InitSeparationsLog();
	InitLifeSpanLog();
	InitContactsLog();
	InitCollisionsLog();
	InitCarryLog();
	InitEnergyLog();
	InitComplexityLog( fBrainFunctionRecentRecordFrequency );
// 	InitAvrComplexityLog();
	
	if( fComplexityFitnessWeight != 0.0 || fRecordComplexity )
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
	
	inited = true;
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

	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
	// ^^^ PARALLEL TASK GrowAgent
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
	class GrowAgent : public ITask
	{
	public:
		agent *a;
		GrowAgent( agent *a )
		{
			this->a = a;
		}

		virtual void task_exec( TSimulation *sim )
		{
			a->grow( sim->fMateWait,
					 sim->fRecordGenomes,
					 sim->RecordBrainAnatomy( a->Number() ),
					 sim->RecordBrainFunction( a->Number() ),
					 sim->fRecordPosition );


#if RecordRandomBrainAnatomies
			a->GetBrain()->dumpAnatomical( "run/brain/random", "birth", a->Number(), 0.0 );
#endif
		}
	};

	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
	// ^^^ SERIAL TASK UpdateStats
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
	class UpdateStats : public ITask
	{
	public:
		agent *a;
		UpdateStats( agent *a )
		{
			this->a = a;
		}

		virtual void task_exec( TSimulation *sim )
		{
			sim->fNeuronGroupCountStats.add( a->GetBrain()->NumNeuronGroups() );

			sim->FoodEnergyIn( a->GetFoodEnergy() );

			if( sim->fRecordPosition )
				a->RecordPosition();
		}
	};

	// first handle the individual domains
	for (id = 0; id < fNumDomains; id++)
	{
		numSeededDomain = 0;	// reset for each domain

		int limit = min((fMaxNumAgents - (long)objectxsortedlist::gXSortedObjects.getCount(AGENTTYPE)), fDomains[id].initNumAgents);
		for (int i = 0; i < limit; i++)
		{
			bool isSeed = false;

			c = agent::getfreeagent(this, &fStage);
			Q_ASSERT(c != NULL);
				
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

			// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
			// !!! POST PARALLEL
			// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
			fScheduler.postParallel( new GrowAgent(c) );
				
			fStage.AddObject(c);
				
			float x = randpw() * (fDomains[id].absoluteSizeX - 0.02) + fDomains[id].startX + 0.01;
			float z = randpw() * (fDomains[id].absoluteSizeZ - 0.02) + fDomains[id].startZ + 0.01;
			//float z = -0.01 - randpw() * (globals::worldsize - 0.02);
			float y = 0.5 * agent::gAgentHeight;
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
			fScheduler.postSerial( new UpdateStats(c) );

			Birth( c, LifeSpan::BR_SIMINIT );
		}
			
		numSeededTotal += numSeededDomain;
	}
	
	// Handle global initial creations, if necessary
	Q_ASSERT( fInitNumAgents <= fMaxNumAgents );
		
	while( (int)objectxsortedlist::gXSortedObjects.getCount(AGENTTYPE) < fInitNumAgents )
	{
		bool isSeed = true;

		c = agent::getfreeagent( this, &fStage );
		Q_CHECK_PTR(c);

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

		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		// !!! POST PARALLEL
		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		fScheduler.postParallel( new GrowAgent(c) );
			
		fStage.AddObject(c);
			
		float x =  0.01 + randpw() * (globals::worldsize - 0.02);
		float z = -0.01 - randpw() * (globals::worldsize - 0.02);
		float y = 0.5 * agent::gAgentHeight;
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
		fScheduler.postSerial( new UpdateStats(c) );

		Birth(c, LifeSpan::BR_SIMINIT );
	}
}

//---------------------------------------------------------------------------
// TSimulation::InitNeuralValues
//
// Calculate neural values based either defalt neural values or persistant
// values that were loaded prior to this call.
//---------------------------------------------------------------------------
void TSimulation::InitNeuralValues()
{
	int numinputneurgroups = 5;
	if( genome::gEnableMateWaitFeedback )
		numinputneurgroups++;
	if( genome::gEnableSpeedFeedback )
		numinputneurgroups++;
	if( genome::gEnableCarry )
		numinputneurgroups += 2;
	brain::gNeuralValues.numinputneurgroups = numinputneurgroups;

	int numoutneurgroups = 7;
	if( genome::gEnableGive )
		numoutneurgroups++;
	if( genome::gEnableCarry )
		numoutneurgroups += 2;
	brain::gNeuralValues.numoutneurgroups = numoutneurgroups;

    brain::gNeuralValues.maxnoninputneurgroups = brain::gNeuralValues.maxinternalneurgroups + brain::gNeuralValues.numoutneurgroups;
    brain::gNeuralValues.maxneurgroups = brain::gNeuralValues.maxnoninputneurgroups + brain::gNeuralValues.numinputneurgroups;
    brain::gNeuralValues.maxneurpergroup = brain::gNeuralValues.maxeneurpergroup + brain::gNeuralValues.maxineurpergroup;
    brain::gNeuralValues.maxinternalneurons = brain::gNeuralValues.maxneurpergroup * brain::gNeuralValues.maxinternalneurgroups;
    brain::gNeuralValues.maxinputneurons = genome::gMaxvispixels * 3 + 2;
    brain::gNeuralValues.maxnoninputneurons = brain::gNeuralValues.maxinternalneurons + brain::gNeuralValues.numoutneurgroups;
    brain::gNeuralValues.maxneurons = brain::gNeuralValues.maxinternalneurons + brain::gNeuralValues.maxinputneurons + brain::gNeuralValues.numoutneurgroups;

    // the 2's are due to the input & output neurons
    //     doubling as e & i presynaptically
    // the 3's are due to the output neurons also acting as e-neurons
    //     postsynaptically, accepting internal connections
    // the -'s are due to the output & internal neurons not self-stimulating
    brain::gNeuralValues.maxsynapses = brain::gNeuralValues.maxinternalneurons * brain::gNeuralValues.maxinternalneurons 	// internal
                + 2 * brain::gNeuralValues.numoutneurgroups * brain::gNeuralValues.numoutneurgroups					// output
                + 3 * brain::gNeuralValues.maxinternalneurons * brain::gNeuralValues.numoutneurgroups       			// internal/output
                + 2 * brain::gNeuralValues.maxinternalneurons * brain::gNeuralValues.maxinputneurons  				// internal/input
                + 2 * brain::gNeuralValues.maxinputneurons * brain::gNeuralValues.numoutneurgroups       				// input/output
                - 2 * brain::gNeuralValues.numoutneurgroups													// output/output
                - brain::gNeuralValues.maxinternalneurons;                   									// internal/internal
}


//---------------------------------------------------------------------------
// TSimulation::InitWorld
//
// Initialize simulation values. Pay attention to the ordering of the
// value initialization, as values may depend on a previously set value.
//---------------------------------------------------------------------------
void TSimulation::InitWorld()
{
	globals::worldsize = 100.0;
	globals::wraparound = false;
	globals::blockedEdges = true;
	globals::stickyEdges = false;
	
    fMinNumAgents = 15;
    fInitNumAgents = 15;
	fNumberToSeed = 0;
	fProbabilityOfMutatingSeeds = 0.0;
    fMinFoodCount = 15;
    fMaxFoodCount = 50;
    fMaxFoodGrownCount = 25;
    fInitFoodCount = 25;
    fFoodRate = 0.1;
    fMiscAgents = 150;
	fPositionSeed = 42;
    fGenomeSeed = 42;
	fSimulationSeed = 0;
    fAgentsRfood = RFOOD_TRUE;
    fFitness1Frequency = 100;
    fFitness2Frequency = 2;
	fEat2Consume = 20.0;
    fNumberFit = 30;
	fNumberRecentFit = 10;
    fEatFitnessParameter = 10.0;
    fMateFitnessParameter = 10.0;
    fMoveFitnessParameter = 1.0;
    fEnergyFitnessParameter = 1.0;
    fAgeFitnessParameter = 1.0;
 	fTotalHeuristicFitness = fEatFitnessParameter
					+ fMateFitnessParameter
					+ fMoveFitnessParameter
					+ fEnergyFitnessParameter
					+ fAgeFitnessParameter;
    fMinMateFraction = 0.05;
	fMateWait = 25;
	fEatMateSpan = 0;
    fPower2Energy = 2.5;
	fEatThreshold = 0.2;
    fMateThreshold = 0.5;
    fFightThreshold = 0.2;
	fFightFraction = 1.0;
	fGiveThreshold = 0.2;
	fPickupThreshold = 0.5;
	fDropThreshold = 0.5;
  	fGroundColor.r = 0.1;
    fGroundColor.g = 0.15;
    fGroundColor.b = 0.05;
    fCameraColor.r = 1.0;
    fCameraColor.g = 1.0;
    fCameraColor.b = 1.0;
    fCameraRadius = 0.6;
    fCameraHeight = 0.35;
    fCameraRotationRate = 0.09;
    fRotateWorld = (fCameraRotationRate != 0.0);	//Boolean for enabling or disabling world roation (CMB 3/19/06)
    fCameraAngleStart = 0.0;
    fCameraFOV = 90.0;
	fMonitorAgentRank = 1;
	fMonitorAgentRankOld = 0;
	fMonitorAgent = NULL;
	fMonitorGeneSeparation = false;
    fRecordGeneSeparation = false;

    fNumberCreated = 0;
    fNumberCreatedRandom = 0;
    fNumberCreated1Fit = 0;
    fNumberCreated2Fit = 0;
	fNumberAlive = 0;
    fNumberBorn = 0;
	fNumberBornVirtual = 0;
    fNumberDied = 0;
    fNumberDiedAge = 0;
    fNumberDiedEnergy = 0;
    fNumberDiedFight = 0;
    fNumberDiedEat = 0;
    fNumberDiedEdge = 0;
	fNumberDiedSmite = 0;
	fNumberDiedPatch = 0;
    fNumberFights = 0;
    fMaxFitness = 0.;
    fAverageFitness = 0.;
	fBirthDenials = 0;
    fMiscDenials = 0;
    fLastCreated = 0;
    fMaxGapCreate = 0;
    fMinGeneSeparation = 1.e+10;
    fMaxGeneSeparation = 0.0;
    fAverageGeneSeparation = 5.e+9;
    fNumBornSinceCreated = 0;
    fChartGeneSeparation = false; // GeneSeparation (if true, genesepmon must be true)
    fDeathProbability = 0.001;
	fSmiteMode = 'L';
    fSmiteFrac = 0.10;
	fSmiteAgeFrac = 0.25;
    fShowVision = true;
	fStaticTimestepGeometry = false;
	fRecordMovie = false;
	fMovieWriter = NULL;
	fRecordPerformanceStats = true;

    fFitI = 0;
    fFitJ = 1;
		
	fStep = 0;
	globals::worldsize = 100.0;

    brain::gMinWin = 22;
    brain::gDecayRate = 0.9;
    brain::gLogisticSlope = 0.5;
    brain::gMaxWeight = 1.0;
    brain::gInitMaxWeight = 0.5;
	brain::gNumPrebirthCycles = 25;
 	brain::gNeuralValues.mininternalneurgroups = 1;
    brain::gNeuralValues.maxinternalneurgroups = 5;
    brain::gNeuralValues.mineneurpergroup = 1;
    brain::gNeuralValues.maxeneurpergroup = 8;
    brain::gNeuralValues.minineurpergroup = 1;
    brain::gNeuralValues.maxineurpergroup = 8;
	brain::gNeuralValues.numoutneurgroups = 7;  // set dynamically in InitNeuralValues()
    brain::gNeuralValues.numinputneurgroups = 5;  // set dynamically in InitNeuralValues()
    brain::gNeuralValues.maxbias = 1.0;
    brain::gNeuralValues.minbiaslrate = 0.0;
    brain::gNeuralValues.maxbiaslrate = 0.2;
    brain::gNeuralValues.minconnectiondensity = 0.0;
    brain::gNeuralValues.maxconnectiondensity = 1.0;
    brain::gNeuralValues.mintopologicaldistortion = 0.0;
	brain::gNeuralValues.maxtopologicaldistortion = 1.0;
    brain::gNeuralValues.maxsynapse2energy = 0.01;
    brain::gNeuralValues.maxneuron2energy = 0.1;
	
    genome::gGrayCoding = true;
    genome::gMinvispixels = 1;
    genome::gMaxvispixels = 8;
    genome::gMinMutationRate = 0.01;
    genome::gMaxMutationRate = 0.1;
    genome::gMinNumCpts = 1;
    genome::gMaxNumCpts = 5;
    genome::gMinLifeSpan = 1000;
    genome::gMaxLifeSpan = 2000;
	genome::gMinStrength = 0.5;
	genome::gMaxStrength = 2.0;
    genome::gMinmateenergy = 0.2;
    genome::gMaxmateenergy = 0.8;
    genome::gMinmaxspeed = 0.5;
    genome::gMaxmaxspeed = 1.5;
    genome::gMinlrate = 0.0;
    genome::gMaxlrate = 0.1;
    genome::gMiscBias = 1.0;
    genome::gMiscInvisSlope = 2.0;
    genome::gMinBitProb = 0.1;
    genome::gMaxBitProb = 0.9;
	genome::gEnableMateWaitFeedback = false;
	genome::gEnableSpeedFeedback = false;
	genome::gEnableGive = false;
	genome::gEnableCarry = false;

	fGraphics = true;
    agent::gVision = true;
    agent::gMaxVelocity = 1.0;
    fMaxNumAgents = 50;
    agent::gInitMateWait = 25;
    agent::gMinAgentSize = 1.0;
    agent::gMinAgentSize = 4.0;
    agent::gMinMaxEnergy = 500.0;
    agent::gMaxMaxEnergy = 1000.0;
    agent::gSpeed2DPosition = 1.0;
    agent::gYaw2DYaw = 1.0;
    agent::gMinFocus = 20.0;
    agent::gMaxFocus = 140.0;
    agent::gAgentFOV = 10.0;
    agent::gMaxSizeAdvantage = 2.5;
    agent::gEnergyUseMultiplier = 1.0;
    agent::gEat2Energy = 0.01;
    agent::gMate2Energy = 0.1;
    agent::gFight2Energy = 1.0;
    agent::gMaxSizePenalty = 10.0;
    agent::gSpeed2Energy = 0.1;
    agent::gYaw2Energy = 0.1;
    agent::gLight2Energy = 0.01;
    agent::gFocus2Energy = 0.001;
	agent::gPickup2Energy = 0.5;
	agent::gDrop2Energy = 0.001;
	agent::gCarryAgent2Energy = 0.05;
	agent::gCarryAgentSize2Energy = 0.05;
    agent::gFixedEnergyDrain = 0.1;
	agent::gMaxCarries = 3;
    agent::gAgentHeight = 0.2;

	food::gMinFoodEnergy = 20.0;
	fMinFoodEnergyAtDeath = food::gMinFoodEnergy;
    food::gMaxFoodEnergy = 300.0;
    food::gSize2Energy = 100.0;
	food::gCarryFood2Energy = 0.01;
    food::gFoodHeight = 0.6;
    food::gFoodColor.r = 0.2;
    food::gFoodColor.g = 0.6;
    food::gFoodColor.b = 0.2;

    barrier::gBarrierHeight = 5.0;
    barrier::gBarrierColor.r = 0.35;
    barrier::gBarrierColor.g = 0.25;
    barrier::gBarrierColor.b = 0.15;

	brick::gBrickHeight = 0.5;
	brick::gCarryBrick2Energy = 0.01;

}


//---------------------------------------------------------------------------
// TSimulation::InitMonitoringWindows
//
// Create and display monitoring windows
//---------------------------------------------------------------------------

void TSimulation::InitMonitoringWindows()
{
	// Agent birthrate
	fBirthrateWindow = new TChartWindow( "born / (born + created)", "Birthrate" );
//	fBirthrateWindow->setWindowTitle( QString( "born / (born + created)" ) );
	sprintf( fBirthrateWindow->title, "born / (born + created)" );
	fBirthrateWindow->SetRange(0.0, 1.0);
	fBirthrateWindow->setFixedSize(TChartWindow::kMaxWidth, TChartWindow::kMaxHeight);

	// Agent fitness		
	fFitnessWindow = new TChartWindow( "maxfit, curmaxfit, avgfit", "Fitness", 3);
//	fFitnessWindow->setWindowTitle( QString( "maxfit, curmaxfit, avgfit" ) );
	sprintf( fFitnessWindow->title, "maxfit, curmaxfit, avgfit" );
	fFitnessWindow->SetRange(0, 0.0, 1.0);
	fFitnessWindow->SetRange(1, 0.0, 1.0);
	fFitnessWindow->SetRange(2, 0.0, 1.0);
	fFitnessWindow->SetColor(0, 1.0, 1.0, 1.0);
	fFitnessWindow->SetColor(1, 1.0, 0.3, 0.0);
	fFitnessWindow->SetColor(2, 0.0, 1.0, 1.0);
	fFitnessWindow->setFixedSize(TChartWindow::kMaxWidth, TChartWindow::kMaxHeight);	
	
	// Food energy
	fFoodEnergyWindow = new TChartWindow( "energy in, total, avg", "Energy", 3);
//	fFoodEnergyWindow->setWindowTitle( QString( "energy in, total, avg" ) );
	sprintf( fFoodEnergyWindow->title, "energy in, total, avg" );
	fFoodEnergyWindow->SetRange(0, -1.0, 1.0);
	fFoodEnergyWindow->SetRange(1, -1.0, 1.0);
	fFoodEnergyWindow->SetRange(2, -1.0, 1.0);
	fFoodEnergyWindow->setFixedSize(TChartWindow::kMaxWidth, TChartWindow::kMaxHeight);	
	
	// Population
	const Color popColor[7] =
	{
		Color( 1.0, 0.0, 0.0, 0.0 ),
		Color( 0.0, 1.0, 0.0, 0.0 ),
		Color( 0.0, 0.0, 1.0, 0.0 ),
		Color( 0.0, 1.0, 1.0, 0.0 ),
		Color( 1.0, 0.0, 1.0, 0.0 ),
		Color( 1.0, 1.0, 0.0, 0.0 ),
		Color( 1.0, 1.0, 1.0, 1.0 )
	};
	
	const short numpop = (fNumDomains < 2) ? 1 : (fNumDomains + 1);
    fPopulationWindow = new TChartWindow( "Population", "Population", numpop);
//	fPopulationWindow->setWindowTitle( "population size" );
	sprintf( fPopulationWindow->title, "Population" );
	for (int i = 0; i < numpop; i++)
	{
		int colorIndex = i % 7;
		fPopulationWindow->SetRange(short(i), 0, fMaxNumAgents);
		fPopulationWindow->SetColor(i, popColor[colorIndex]);
	}
	fPopulationWindow->setFixedSize(TChartWindow::kMaxWidth, TChartWindow::kMaxHeight);

	// Brain monitor
	fBrainMonitorWindow = new TBrainMonitorWindow();
//	fBrainMonitorWindow->setWindowTitle( QString( "Brain Monitor" ) );
	
	// Agent POV
	fAgentPOVWindow = new TAgentPOVWindow( fMaxNumAgents, this );
	
	// Status window
	fTextStatusWindow = new TTextStatusWindow( this );
	
	//Overhead window
	fOverheadWindow = new TOverheadView(this);			

#if 0
	// Gene separation
	fGeneSeparationWindow = new TBinChartWindow( "gene separation", "GeneSeparation" );
//	fGeneSeparationWindow->setWindowTitle( "gene separation" );
	fGeneSeparationWindow->SetRange(0.0, 1.0);
	fGeneSeparationWindow->SetExponent(0.5);
#endif
		
	// Tile and show them along the left edge of the screen
//	const int titleHeight = fBirthrateWindow->frameGeometry().height() - TChartWindow::kMaxHeight;
	const int titleHeight = fBirthrateWindow->frameGeometry().height() - fBirthrateWindow->geometry().height();

	int topY = titleHeight + kMenuBarHeight;

	if (fBirthrateWindow != NULL)
	{
		fBirthrateWindow->RestoreFromPrefs(0, topY);
		topY = fBirthrateWindow->frameGeometry().bottom() + titleHeight + 1;
	}

	if (fFitnessWindow != NULL)
	{	
		fFitnessWindow->RestoreFromPrefs(0, topY);
		topY = fFitnessWindow->frameGeometry().bottom() + titleHeight + 1;
	}
	
	if (fFoodEnergyWindow != NULL)
	{		
		fFoodEnergyWindow->RestoreFromPrefs(0, topY);
		topY = fFoodEnergyWindow->frameGeometry().bottom() + titleHeight + 1;
	}
	
	if (fPopulationWindow != NULL)
	{			
		fPopulationWindow->RestoreFromPrefs(0, topY);
		topY = fPopulationWindow->frameGeometry().bottom() + titleHeight + 1;
	}
	
	if (fGeneSeparationWindow != NULL)
	{
		fGeneSeparationWindow->RestoreFromPrefs(0, topY);
		topY = fGeneSeparationWindow->frameGeometry().bottom() + titleHeight + 1;
	}
	
	const int mainWindowTitleHeight = 22;

	if (fAgentPOVWindow != NULL)
		fAgentPOVWindow->RestoreFromPrefs( fBirthrateWindow->width() + 1, kMenuBarHeight + mainWindowTitleHeight + titleHeight );
	
	if (fBrainMonitorWindow != NULL)
		fBrainMonitorWindow->RestoreFromPrefs( fBirthrateWindow->width() + 1, kMenuBarHeight + mainWindowTitleHeight + titleHeight + titleHeight );

	if (fTextStatusWindow != NULL)
	{
		QDesktopWidget* desktop = QApplication::desktop();
		const QRect& screenSize = desktop->screenGeometry( desktop->primaryScreen() );
		fTextStatusWindow->RestoreFromPrefs( screenSize.width() - TTextStatusWindow::kDefaultWidth, kMenuBarHeight /*+ mainWindowTitleHeight + titleHeight*/ );
	}
 	
	//Open overhead window CMB 3/17/06
	if (fOverheadWindow != NULL)
		fOverheadWindow->RestoreFromPrefs( fBirthrateWindow->width() + 1, kMenuBarHeight + mainWindowTitleHeight + titleHeight + titleHeight );
                //(screenleft,screenleft+.75*xscreensize, screenbottom,screenbottom+(5./6.)*yscreensize);
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

	SYSTEM( "cp seedPositions.txt run/motion/position" );

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
// TSimulation::InitLifeSpanLog
//---------------------------------------------------------------------------

void TSimulation::InitLifeSpanLog()
{
	fLifeSpanLog = new DataLibWriter( "run/lifespans.txt" );

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

	fLifeSpanLog->beginTable( "LifeSpans",
							  colnames,
							  coltypes );
}

//---------------------------------------------------------------------------
// TSimulation::UpdateLifeSpanLog
//---------------------------------------------------------------------------

void TSimulation::UpdateLifeSpanLog( agent *a )
{
	LifeSpan *ls = a->GetLifeSpan();

	fLifeSpanLog->addRow( a->Number(),
						  ls->birth.step,
						  LifeSpan::BR_NAMES[ls->birth.reason],
						  ls->death.step,
						  LifeSpan::DR_NAMES[ls->death.reason] );
}

//---------------------------------------------------------------------------
// TSimulation::EndLifeSpanLog
//---------------------------------------------------------------------------

void TSimulation::EndLifeSpanLog()
{
	fLifeSpanLog->endTable();
	delete fLifeSpanLog;
	fLifeSpanLog = NULL;
}

//---------------------------------------------------------------------------
// TSimulation::InitContactsLog
//---------------------------------------------------------------------------

void TSimulation::InitContactsLog()
{
	if( !fRecordContacts )
		return;

	fContactsLog = new DataLibWriter( "run/events/contacts.log" );

	ContactEntry::start( fContactsLog );
}

//---------------------------------------------------------------------------
// TSimulation::EndContactsLog
//---------------------------------------------------------------------------

void TSimulation::EndContactsLog()
{
	if( !fRecordContacts )
		return;

	ContactEntry::stop( fContactsLog );

	delete fContactsLog;
	fContactsLog = NULL;
}

//---------------------------------------------------------------------------
// TSimulation::InitCollisionsLog
//---------------------------------------------------------------------------

void TSimulation::InitCollisionsLog()
{
	if( !fRecordCollisions )
		return;

	fCollisionsLog = new DataLibWriter( "run/events/collisions.log" );

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

	fCollisionsLog->beginTable( "Collisions",
								colnames,
								coltypes );
}

//---------------------------------------------------------------------------
// TSimulation::UpdateCollisionsLog
//---------------------------------------------------------------------------

void TSimulation::UpdateCollisionsLog( agent *c,
									   ObjectType ot )
{
	if( !fRecordCollisions )
	{
		return;
	}

	// We store type as a string since this data will most likely
	// be processed by a script.
	static const char *names[] = {"agent",
								  "food",
								  "brick",
								  "barrier",
								  "edge"};

	fCollisionsLog->addRow( fStep,
							c->Number(),
							names[ot] );
}

//---------------------------------------------------------------------------
// TSimulation::EndCollisionsLog
//---------------------------------------------------------------------------

void TSimulation::EndCollisionsLog()
{
	if( !fRecordCollisions )
		return;

	fCollisionsLog->endTable();
	delete fCollisionsLog;
	fCollisionsLog = NULL;
}

//---------------------------------------------------------------------------
// TSimulation::InitCarryLog
//---------------------------------------------------------------------------

void TSimulation::InitCarryLog()
{
	if( !fRecordCarry )
		return;

	fCarryLog = new DataLibWriter( "run/events/carry.log" );

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

	fCarryLog->beginTable( "Carry",
						   colnames,
						   coltypes );
}


//---------------------------------------------------------------------------
// TSimulation::UpdateCarryLog
//---------------------------------------------------------------------------

void TSimulation::UpdateCarryLog( agent *c,
								  gobject *obj,
								  CarryAction action )
{
	if( !fRecordCarry )
	{
		return;
	}

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

	fCarryLog->addRow( fStep,
					   c->Number(),
					   actions[action],
					   objectType,
					   obj->getTypeNumber() );
}


//---------------------------------------------------------------------------
// TSimulation::EndCarryLog
//---------------------------------------------------------------------------

void TSimulation::EndCarryLog()
{
	if( !fRecordCarry )
		return;

	fCarryLog->endTable();
	delete fCarryLog;
	fCarryLog = NULL;
}

//---------------------------------------------------------------------------
// TSimulation::InitEnergyLog
//---------------------------------------------------------------------------

void TSimulation::InitEnergyLog()
{
	if( !fRecordEnergy )
		return;

	fEnergyLog = new DataLibWriter( "run/events/energy.log" );

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

	fEnergyLog->beginTable( "Energy",
							colnames,
							coltypes );
}


//---------------------------------------------------------------------------
// TSimulation::UpdateEnergyLog
//---------------------------------------------------------------------------

void TSimulation::UpdateEnergyLog( agent *c,
								   gobject *obj,
								   float neuralActivation,
								   const Energy &energy,
								   EnergyLogEventType elet )
{
	if( !fRecordEnergy )
	{
		return;
	}

	static const char *eventTypes[] = {"G",
									   "F",
									   "E"};
	const char *eventType = eventTypes[elet];

	Variant coldata[ 5 + globals::numEnergyTypes ];
	coldata[0] = fStep;
	coldata[1] = c->Number();
	coldata[2] = eventType;
	coldata[3] = (long)obj->getTypeNumber();
	coldata[4] = neuralActivation;
	for( int i = 0; i < globals::numEnergyTypes; i++ )
	{
		coldata[5 + i] = energy[i];
	}

	fEnergyLog->addRow( coldata );
}


//---------------------------------------------------------------------------
// TSimulation::EndEnergyLog
//---------------------------------------------------------------------------

void TSimulation::EndEnergyLog()
{
	if( !fRecordEnergy )
		return;

	fEnergyLog->endTable();
	delete fEnergyLog;
	fEnergyLog = NULL;
}

//---------------------------------------------------------------------------
// TSimulation::InitSeparationsLog
//---------------------------------------------------------------------------

void TSimulation::InitSeparationsLog()
{
	if( fRecordSeparations )
	{
		fSeparationsLog = new DataLibWriter( "run/genome/separations.txt" );
	}
	else
	{
		fSeparationsLog = NULL;
	}

	fSeparationCache.start( fSeparationsLog );
}

//---------------------------------------------------------------------------
// TSimulation::EndSeparationsLog
//---------------------------------------------------------------------------

void TSimulation::EndSeparationsLog()
{
	fSeparationCache.stop();

	if( fRecordSeparations )
	{
		delete fSeparationsLog;
		fSeparationsLog = NULL;
	}
}


//---------------------------------------------------------------------------
// TSimulation::InitComplexityLog
//---------------------------------------------------------------------------

void TSimulation::InitComplexityLog( long epoch )
{
	//printf( "%s %ld %s\n", __func__, epoch, fRecordComplexity ? "True" : "False" );

	if( ! fRecordComplexity )
		return;	

	fComplexityLog = OpenComplexityLog( epoch );
	
	if( epoch == fBrainFunctionRecentRecordFrequency )	// first time only
		fComplexitySeedLog = OpenComplexityLog( 0 );
}


//---------------------------------------------------------------------------
// TSimulation::OpenComplexityLog
//---------------------------------------------------------------------------

DataLibWriter* TSimulation::OpenComplexityLog( long epoch )
{
	char path[256];
	sprintf( path, "run/brain/Recent/%ld/complexity_%s.plt", epoch, fComplexityType.c_str() );
	
	//printf( "%s %s\n", __func__, path );

	DataLibWriter* log = new DataLibWriter( path );

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

	log->beginTable( fComplexityType.c_str(),
					 colnames,
					 coltypes );
	
	return( log );
}


//---------------------------------------------------------------------------
// TSimulation::UpdateComplexityLog
//---------------------------------------------------------------------------

void TSimulation::UpdateComplexityLog( agent *a )
{
	//printf( "%s: %ld %g\n", __func__, a->Number(), a->Complexity() );
	
	// complexity will only be 0.0 if it went uncomputed, such as when
	// the number of neurons is more than the number of time steps the
	// agent lived, or, under some running conditions, the agent's
	// lifespan is simply too short (< 200 time steps).
	if( a->Complexity() > 0.0 )
	{
		fComplexityLog->addRow( a->Number(), a->Complexity() );
	
		if( a->Number() <= fInitNumAgents )
			fComplexitySeedLog->addRow( a->Number(), a->Complexity() );
	}
}


//---------------------------------------------------------------------------
// TSimulation::EndComplexityLog
//---------------------------------------------------------------------------

void TSimulation::EndComplexityLog( long epoch )
{
	//printf( "%s %ld %p\n", __func__, epoch, fComplexityLog );

	// If we've run long enough we can be sure all the seed agents
	// have died, then close the complexity seed log (if it's still open)
	if( fComplexitySeedLog && epoch >= genome::gMaxLifeSpan )
	{
		fComplexitySeedLog->endTable();
// 		UpdateAvrComplexityLog( 0, fComplexitySeedLog );
		delete fComplexitySeedLog;
		fComplexitySeedLog = NULL;
	}

	if( fComplexityLog )
	{
		fComplexityLog->endTable();
// 		UpdateAvrComplexityLog( epoch, fComplexityLog );
		delete fComplexityLog;
		fComplexityLog = NULL;
	}
}


//---------------------------------------------------------------------------
// TSimulation::InitAvrComplexityLog
//---------------------------------------------------------------------------

// void TSimulation::InitAvrComplexityLog()
// {
// 	if( ! fRecordComplexity )
// 		return;
// 	
// 	fAvrComplexityLog = new DataLibWriter( "run/brain/Recent/AvrComplexity.plt" );
// 
// 	const char *colnames[] =
// 		{
// 			"Timestep",
// 			"min",
// 			"q1",
// 			"median",
// 			"q3",
// 			"max",
// 			"mean",
// 			"mean_stderr",
// 			"sampsize",
// 			NULL
// 		};
// 	const datalib::Type coltypes[] =
// 		{
// 			datalib::INT,
// 			datalib::FLOAT,
// 			datalib::FLOAT,
// 			datalib::FLOAT,
// 			datalib::FLOAT,
// 			datalib::FLOAT,
// 			datalib::FLOAT,
// 			datalib::FLOAT,
// 			datalib::INT
// 		};
// 
// 	fAvrComplexityLog->beginTable( fComplexityType.c_str(),
// 								   colnames,
// 								   coltypes );
// }


//---------------------------------------------------------------------------
// TSimulation::UpdateAvrComplexityLog
//---------------------------------------------------------------------------

// NOTE: Following code is just leftover from UpdateComplexityLog() and must be entirely replaced
// void TSimulation::UpdateAvrComplexityLog()
// {
// 	fComplexityLog->addRow( a->Number(),
// 							a->GetComplexity() );
// 	
// 	if( a->Number() <= fInitAgents )
// 		fComplexitySeedLog->addRow( a->Number(),
// 									a->GetComplexity() );
// }


//---------------------------------------------------------------------------
// TSimulation::EndAvrComplexityLog
//---------------------------------------------------------------------------

// NOTE: Following code is just leftover from EndComplexityLog() and must be entirely replaced
// void TSimulation::EndAvrComplexityLog( long epoch )
// {
// 	if( fComplexityLog )
// 	{
// 		fComplexityLog->endTable();
// 		delete fComplexityLog;
// 		fComplexityLog = NULL;
// 	}
// 	
// 	// If we've run long enough we can be sure all the seed agents
// 	// have died, then close the complexity seed log (if it's still open)
// 	if( fComplexitySeedLog && epoch >= genome::gMaxLifeSpan )
// 	{
// 		fComplexitySeedLog->endTable();
// 		delete fComplexitySeedLog;
// 		fComplexitySeedLog = NULL;
// 	}
// }


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
											agent::gSpeed2DPosition,
											fSolidObjects,
											NULL);
	}
}


//---------------------------------------------------------------------------
// TSimulation::UpdateAgents_StaticTimestepGeometry
//---------------------------------------------------------------------------
void TSimulation::UpdateAgents_StaticTimestepGeometry()
{
	if( fParallelBrains )
	{
		//************************************************************
		//************************************************************
		//************************************************************
		//*****
		//***** BEGIN PARALLEL REGION
		//*****
		//************************************************************
		//************************************************************
		//************************************************************
		fUpdateBrainQueue.reset();

#pragma omp parallel
		{
			//////////////////////////////////////////////////
			//// MASTER ONLY
			//////////////////////////////////////////////////
#pragma omp master
			{
				fStage.Compile();
				objectxsortedlist::gXSortedObjects.reset();

				agent *avision = NULL;

				while (objectxsortedlist::gXSortedObjects.nextObj(AGENTTYPE, (gobject**)&avision))
				{
					avision->UpdateVision();

					fUpdateBrainQueue.post( avision );
				}

				fUpdateBrainQueue.endOfPosts();

				fStage.Decompile();
			}

			//////////////////////////////////////////////////
			//// MASTER & SLAVES
			//////////////////////////////////////////////////
			{
				agent *abrain = NULL;

				while( fUpdateBrainQueue.fetch(&abrain) )
				{
					abrain->UpdateBrain();
				}
			}
		}
		//************************************************************
		//************************************************************
		//************************************************************
		//*****
		//***** END PARALLEL REGION
		//*****
		//************************************************************
		//************************************************************
		//************************************************************
	}
	else
	{
		fStage.Compile();
		objectxsortedlist::gXSortedObjects.reset();

		agent *a = NULL;

		while (objectxsortedlist::gXSortedObjects.nextObj(AGENTTYPE, (gobject**)&a))
		{
			a->UpdateVision();
			a->UpdateBrain();
		}

		fStage.Decompile();
	}

	// ---
	// --- Body (Serial)
	// ---
	{
		agent *a;

		objectxsortedlist::gXSortedObjects.reset();
		while( objectxsortedlist::gXSortedObjects.nextObj( AGENTTYPE, (gobject**)&a) )
		{
			if( !a->BeingCarried() )
				fFoodEnergyOut += a->UpdateBody( fMoveFitnessParameter,
												 agent::gSpeed2DPosition,
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

	// reset all the FoodPatch agent counts to 0
	if( fCalcFoodPatchAgentCounts )
	{
		fNumAgentsNotInOrNearAnyFoodPatch = 0;

		for( int domainNumber = 0; domainNumber < fNumDomains; domainNumber++ )
		{
			for( int foodPatchNumber = 0; foodPatchNumber < fDomains[domainNumber].numFoodPatches; foodPatchNumber++ )
			{
				fDomains[domainNumber].fFoodPatches[foodPatchNumber].resetAgentCounts();
			}
		}
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
				printf( " (%08lx,%ld,%5.2f)", (ulong) fDomains[id].fLeastFit[i], fDomains[id].fLeastFit[i]->Number(), fDomains[id].fLeastFit[i]->HeuristicFitness() );
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
		printf( "%s: at age %ld about to process %ld agents, %ld pieces of food, starting with agent %08lx (%4ld), ending with agent %08lx (%4ld)\n", __FUNCTION__, fStep, objectxsortedlist::gXSortedObjects.getCount(AGENTTYPE), objectxsortedlist::gXSortedObjects.getCount(FOODTYPE), (ulong) c, c->Number(), (ulong) lastAgent, lastAgent->Number() );
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
        cDied = FALSE;

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

				ContactEntry contactEntry( fStep, c, d );

				if( fRecordSeparations )
				{
					// Force a separation calculation so it gets logged.
					fSeparationCache.separation( c, d );
				}

				// -----------------------
				// ---- Mate (Normal) ----
				// -----------------------
                Mate( c, d, &contactEntry );

				// -----------------------
				// -------- Fight --------
				// -----------------------
				bool dDied = false;
                if (fPower2Energy > 0.0)
                {
					Fight( c, d, &contactEntry, &cDied, &dDied );
                }

				// -----------------------
				// -------- Give ---------
				// -----------------------
				if( genome::gEnableGive )
				{
					if( !cDied && !dDied )
					{
						Give( c, d, &contactEntry, &cDied, true );
						if(!cDied)
						{				
							Give( d, c, &contactEntry, &dDied, false );
						}
					}
				}
				
				if( fRecordContacts )
					contactEntry.log( fContactsLog );

				if( cDied )
					break;

            }  // if close enough
        }  // while (agent::gXSortedAgents.next(d))

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
		if( genome::gEnableCarry )
			Carry( c );

		// -----------------------
		// ------- Fitness -------
		// -----------------------
		// keep tabs of current and average fitness for surviving organisms
		Fitness( c );

    } // while loop on agents (c)

	fEatStatistics.StepEnd();
}


//---------------------------------------------------------------------------
// TSimulation::DeathAndStats
//---------------------------------------------------------------------------
void TSimulation::DeathAndStats( void )
{
	agent *c;
	short id;
	
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

	fCurrentNeuronGroupCountStats.reset();
	fCurrentNeuronCountStats.reset();
	fCurrentSynapseCountStats.reset();
	objectxsortedlist::gXSortedObjects.reset();
    while( objectxsortedlist::gXSortedObjects.nextObj( AGENTTYPE, (gobject**) &c ) )
    {

		fCurrentNeuronGroupCountStats.add( c->GetBrain()->NumNeuronGroups() );
		fCurrentNeuronCountStats.add( c->GetBrain()->GetNumNeurons() );
		fCurrentSynapseCountStats.add( c->GetBrain()->GetNumSynapses() );

        id = c->Domain();						// Determine the domain in which the agent currently is located
	
		if( ! fLockStepWithBirthsDeathsLog )
		{
			// If we're not running in LockStep mode, allow natural deaths
			// If we're not using the LowPopulationAdvantage to prevent the population getting too low,
			// or there are enough agents that we can still afford to lose one (globally & in agent's domain)...
			if( !fApplyLowPopulationAdvantage ||
				((objectxsortedlist::gXSortedObjects.getCount( AGENTTYPE ) > fMinNumAgents)
				 && (fNumberAliveWithMetabolism[c->GetMetabolism()->index] > fMinNumAgentsWithMetabolism[c->GetMetabolism()->index])
				 && (fDomains[c->Domain()].numAgents > fDomains[c->Domain()].minNumAgents)) )
			{
				if ( c->GetEnergy().isDepleted() ||
					 c->Age() >= c->MaxAge()     ||
					 ( !globals::blockedEdges && !globals::wraparound &&
					 	(c->x() < 0.0 || c->x() >  globals::worldsize ||
						 c->z() > 0.0 || c->z() < -globals::worldsize) ) ||
					 c->GetDeathByPatch() )
				{
					LifeSpan::DeathReason reason = LifeSpan::DR_NATURAL;

					if (c->Age() >= c->MaxAge())
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

	// If we're saving gene stats, compute them here
	if( fRecordGeneStats )
	{
		// Because we'll be performing the stats calculations/recording in parallel
		// with the master task, which will kill and birth agents, we must create a
		// snapshot of the agents alive right now.
		int nagents = objectxsortedlist::gXSortedObjects.getCount(AGENTTYPE);
		objectxsortedlist::gXSortedObjects.reset();
		for( int i = 0; i < nagents; i++ )
		{
			objectxsortedlist::gXSortedObjects.nextObj( AGENTTYPE, (gobject**)fGeneStatsAgents + i );
		}

		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
		// ^^^ PARALLEL TASK RecordGeneStats
		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
		class RecordGeneStats : public ITask
		{
		public:
			int nagents;
			RecordGeneStats( int nagents )
			{
				this->nagents = nagents;
			}

			virtual void task_exec( TSimulation *sim )
			{
				unsigned long *sum = sim->fGeneSum;
				unsigned long *sum2 = sim->fGeneSum2;
				int ngenes = GenomeUtil::schema->getMutableSize(); 

				memset( sum, 0, sizeof(*sum) * ngenes );
				memset( sum2, 0, sizeof(*sum2) * ngenes );

				for( int i = 0; i < nagents; i++ )
				{
					sim->fGeneStatsAgents[i]->Genes()->updateSum( sum, sum2 );
				}

				fprintf( sim->fGeneStatsFile, "%ld", sim->fStep );
				for( int i = 0; i < ngenes; i++ )
				{
					float mean, stddev;
			
					mean = (float) sum[i] / (float) nagents;
					stddev = sqrt( (float) sum2[i] / (float) nagents  -  mean * mean );
					fprintf( sim->fGeneStatsFile, " %.1f,%.1f", mean, stddev );
				}
				fprintf( sim->fGeneStatsFile, "\n" );
			}
		};

		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		// !!! POST PARALLEL
		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		fScheduler.postParallel( new RecordGeneStats(nagents) );
	}

}

//---------------------------------------------------------------------------
// TSimulation::MateLockstep
//---------------------------------------------------------------------------
void TSimulation::MateLockstep( void )
{
	lsPrint( "t%ld: Triggering %d random births...\n", fStep, fLockstepNumBirthsAtTimestep );

	agent* c = NULL;		// mommy
	agent* d = NULL;		// daddy
	
	for( int count = 0; count < fLockstepNumBirthsAtTimestep; count++ ) 
	{
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
			// Make sure it's old enough (anything except just birthed), and it has been long enough since it mated
			if( (testAgent->Age() > 0) && ((testAgent->Age() - testAgent->LastMate()) >= fMateWait) )
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
			// If it was not just birthed, it has been long enough since it mated, and it's not the same as mommy, it'll do for daddy.
			if( (testAgent->Age() > 0) && ((testAgent->Age() - testAgent->LastMate()) >= fMateWait) && (testAgent->Number() != c->Number()) )
				d = testAgent;	// as long as there's another single legitimate agent for mating, Daddy will be non-NULL

			i++;
				
			if( (i > randomIndex) && (d != NULL) )
				break;
		}

		assert( c != NULL && d != NULL );				// If for some reason we can't select parents, then we'll become out of sync with LOCKSTEP-BirthDeaths.log
		
		lsPrint( "* I have selected Mommy(%ld) and Daddy(%ld). Population size = %d\n", c->Number(), d->Number(), numAgents );
		
		/* === We've selected our parents, now time to birth our agent. */
			
		ttPrint( "age %ld: agents # %ld & %ld are mating randomly\n", fStep, c->Number(), d->Number() );
		
		agent* e = agent::getfreeagent( this, &fStage );
		Q_CHECK_PTR(e);

		e->Genes()->crossover(c->Genes(), d->Genes(), true);
		e->grow( fMateWait,
				 fRecordGenomes,
				 RecordBrainAnatomy( e->Number() ),
				 RecordBrainFunction( e->Number() ),
				 fRecordPosition );
		Energy eenergy = c->mating( fMateFitnessParameter, fMateWait ) + d->mating( fMateFitnessParameter, fMateWait );
		Energy minenergy = fMinMateFraction * ( c->GetMaxEnergy() + d->GetMaxEnergy() ) * 0.5;	// just a modest, reasonable amount; this doesn't really matter in lockstep mode
		eenergy.constrain( minenergy, e->GetMaxEnergy() );
		e->SetEnergy(eenergy);
		e->SetFoodEnergy(eenergy);
		float x =  0.01 + randpw() * (globals::worldsize - 0.02);
		float z = -0.01 - randpw() * (globals::worldsize - 0.02);
		float y = 0.5 * agent::gAgentHeight;
		e->settranslation(x, y, z);
		float yaw =  360.0 * randpw();
		c->setyaw(yaw);
		short kd = WhichDomain(x, z, 0);
		e->Domain(kd);
		fStage.AddObject(e);
		objectxsortedlist::gXSortedObjects.add(e); // Add the new agent directly to the list of objects (no new agent list); the e->listLink that gets auto stored here should be valid immediately
					
		fNewLifes++;
		fDomains[kd].numAgents++;
		fNumberBorn++;
		fDomains[kd].numborn++;
		fNumBornSinceCreated++;
		fDomains[kd].numbornsincecreated++;
		fNeuronGroupCountStats.add( e->GetBrain()->NumNeuronGroups() );
		ttPrint( "age %ld: agent # %ld is born\n", fStep, e->Number() );
		birthPrint( "step %ld: agent # %ld born to %ld & %ld, at (%g,%g,%g), yaw=%g, energy=%g, domain %d (%d & %d), neurgroups=%d\n",
			fStep, e->Number(), c->Number(), d->Number(), e->x(), e->y(), e->z(), e->yaw(), e->Energy(), kd, id, jd, e->GetBrain()->NumNeuronGroups() );

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
						ContactEntry *contactEntry )
{
	int cMatePotential = GetMatePotential( c );
	int dMatePotential = GetMatePotential( d );
	int cMateStatus = GetMateStatus( cMatePotential, dMatePotential );
	int dMateStatus = GetMateStatus( dMatePotential, cMatePotential );

#ifndef OF1
	if( !fLockStepWithBirthsDeathsLog )
	{
#endif
		if( (cMateStatus == MATE__DESIRED) && (dMateStatus == MATE__DESIRED) )
		{
			// the agents are mate-worthy, so now deal with other conditions...
		
			// test for steady-state GA vs. natural selection
			if( (fHeuristicFitnessWeight != 0.0) || (fComplexityFitnessWeight != 0.0) )
			{
				// we're using the steady state GA (instead of natural selection)
				// count virtual offspring (offspring that would have resulted from
				// otherwise successful mating behaviors), but don't actually instantiate them
				(void) c->mating( fMateFitnessParameter, fMateWait );
				(void) d->mating( fMateFitnessParameter, fMateWait );
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
					Q_CHECK_PTR(e);

					e->Genes()->crossover(c->Genes(), d->Genes(), true);

					Energy eenergy = c->mating( fMateFitnessParameter, fMateWait ) + d->mating( fMateFitnessParameter, fMateWait );

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

					// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
					// ^^^ PARALLEL TASK GrowAgent
					// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
					class GrowAgent : public ITask
					{
					public:
						agent *e;
						Energy eenergy;
						GrowAgent( agent *e, const Energy &eenergy )
						{
							this->e = e;
							this->eenergy = eenergy;
						}

						virtual void task_exec( TSimulation *sim )
						{
							e->grow( sim->fMateWait,
									 sim->fRecordGenomes,
									 sim->RecordBrainAnatomy( e->Number() ),
									 sim->RecordBrainFunction( e->Number() ),
									 sim->fRecordPosition );

							e->SetEnergy(eenergy);
							e->SetFoodEnergy(eenergy);
						}
					};

					// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
					// !!! POST PARALLEL
					// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
					fScheduler.postParallel( new GrowAgent(e, eenergy) );

					// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
					// ^^^ SERIAL TASK AddAgent
					// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
					class AddAgent : public ITask
					{
					public:
						agent *e;
						AddAgent( agent *e )
						{
							this->e = e;
						}

						virtual void task_exec( TSimulation *sim )
						{
							sim->fStage.AddObject(e);
							gdlink<gobject*> *saveCurr = objectxsortedlist::gXSortedObjects.getcurr();
							objectxsortedlist::gXSortedObjects.add(e); // Add the new agent directly to the list of objects (no new agent list); the e->listLink that gets auto stored here should be valid immediately
							objectxsortedlist::gXSortedObjects.setcurr( saveCurr );

							sim->fNeuronGroupCountStats.add( e->GetBrain()->NumNeuronGroups() );
						}
					};

					// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
					// !!! POST SERIAL
					// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
					fScheduler.postSerial( new AddAgent(e) );
				}
			}	// steady-state GA vs. natural selection
		}	// if agents are trying to mate
#ifndef OF1
	} // if not lockstep
#endif

	contactEntry->mate( c, cMateStatus );
	contactEntry->mate( d, dMateStatus );

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
						 ContactEntry *contactEntry,
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

	contactEntry->fight( c, cstatus );
	contactEntry->fight( d, dstatus );

	if ( (cpower > 0.0) || (dpower > 0.0) )
	{
		ttPrint( "age %ld: agents # %ld & %ld are fighting\n", fStep, c->Number(), d->Number() );

		// somebody wants to fight
		fNumberFights++;

		if( cpower > 0.0 )
		{
			Energy ddamage = d->damage( cpower * fPower2Energy, fFightMode == FM_NULL );
			if( !ddamage.isZero() )
				UpdateEnergyLog( c, d, c->Fight(), ddamage, ELET__FIGHT );
		}

		if( dpower > 0.0 )
		{
			Energy cdamage = c->damage( dpower * fPower2Energy, fFightMode == FM_NULL );
			if( !cdamage.isZero() )
				UpdateEnergyLog( d, c, d->Fight(), cdamage, ELET__FIGHT );
		}

		if( !fLockStepWithBirthsDeathsLog )
		{
			// If we're not running in LockStep mode, allow natural deaths
			if (d->GetEnergy().isDepleted())
			{
				//cout << "before deaths2 "; agent::gXSortedAgents.list();	//dbg
				Kill( d, LifeSpan::DR_FIGHT );
				fNumberDiedFight++;
				//cout << "after deaths2 "; agent::gXSortedAgents.list();	//dbg

				*dDied = true;
			}
			if (c->GetEnergy().isDepleted())
			{
				objectxsortedlist::gXSortedObjects.toMark( AGENTTYPE ); // point back to c
				Kill( c, LifeSpan::DR_FIGHT );
				fNumberDiedFight++;

				// note: this leaves list pointing to item before c, and markedAgent set to previous agent
				//objectxsortedlist::gXSortedObjects.setMarkPrevious( AGENTTYPE );	// if previous object was a agent, this would step one too far back, I think - lsy
				//cout << "after deaths3 "; agent::gXSortedAgents.list();	//dbg
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
						ContactEntry *contactEntry,
						bool *xDied,
						bool toMarkOnDeath )
{
	Energy energy;
	int xstatus = GetGiveStatus( x, energy );

	contactEntry->give( x, xstatus );

#if DEBUGCHECK
	unsigned long xnum = x->Number();
#endif

	if( !energy.isDepleted() )
	{
		y->receive( x, energy );

		UpdateEnergyLog( x, y, x->Give(), energy, ELET__GIVE );

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
				UpdateEnergyLog( c, f, c->Eat(), energyEaten, ELET__EAT );
				if( fEvents )
					fEvents->AddEvent( fStep, c->Number(), 'e' );
								 
				FoodEnergyOut( foodEnergyLost );
				fEnergyEaten += energyEaten;
				
				eatPrint( "at step %ld, agent %ld at (%g,%g) with rad=%g wasted %g units of food at (%g,%g) with rad=%g\n", fStep, c->Number(), c->x(), c->z(), c->radius(), foodEaten, f->x(), f->z(), f->radius() );

				if(f->isDepleted() )  // all gone
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
					UpdateEnergyLog( c, f, c->Eat(), energyEaten, ELET__EAT );
					if( fEvents )
						fEvents->AddEvent( fStep, c->Number(), 'e' );

					FoodEnergyOut( foodEnergyLost );
					fEnergyEaten += energyEaten;
					
					eatPrint( "at step %ld, agent %ld at (%g,%g) with rad=%g wasted %g units of food at (%g,%g) with rad=%g\n", fStep, c->Number(), c->x(), c->z(), c->radius(), foodEaten, f->x(), f->z(), f->radius() );

					if( f->isDepleted() )  // all gone
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
		if( c->GetEnergy().isDepleted() )
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
		if( c->NumCarries() < agent::gMaxCarries )
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
			if( ( o->x() + 2.0 * max( max( food::gMaxFoodRadius, agent::gMaxRadius ), brick::gBrickRadius ) ) < ( c->x() - c->radius() ) )
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

				if( c->NumCarries() >= agent::gMaxCarries )	// carrying as much as we can,
					break;								// so get out of the backward while loop
			}
		}
	}

	// can we pick up anything else?
	if( c->NumCarries() < agent::gMaxCarries )
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
					
					if( c->NumCarries() >= agent::gMaxCarries )	// carrying as much as we can,
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
			printf( "appended agent %08lx (%4ld) to fittest list at position %d with fitness %g, count = %d\n", (ulong) c, c->Number(), fCurrentFittestCount-1, c->HeuristicFitness(), fCurrentFittestCount );
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
			printf( "inserted agent %08lx (%4ld) into fittest list at position %ld with fitness %g, count = %d\n", (ulong) c, c->Number(), i, c->HeuristicFitness(), fCurrentFittestCount );
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
                Q_CHECK_PTR(newAgent);

                if ( fNumberFit && (fDomains[id].numdied >= fNumberFit) )
                {
                    // the list exists and is full
                    if (fFitness1Frequency
                    	&& ((fDomains[id].numcreated / fFitness1Frequency) * fFitness1Frequency == fDomains[id].numcreated) )
                    {
                        // revive 1 fittest
                        newAgent->Genes()->copyFrom(fDomains[id].fittest[0]->genes);
                        fNumberCreated1Fit++;
						gaPrint( "%5ld: domain %d creation from one fittest (%4lu) %4ld\n", fStep, id, fDomains[id].fittest[0]->agentID, fNumberCreated1Fit );
                    }
                    else if (fFitness2Frequency
                    		 && ((fDomains[id].numcreated / fFitness2Frequency) * fFitness2Frequency == fDomains[id].numcreated) )
                    {
                        // mate 2 from array of fittest
						if( fTournamentSize > 0 )
						{
							// using tournament selection
							int parent1, parent2;
							PickParentsUsingTournament(fNumberFit, &parent1, &parent2);
							newAgent->Genes()->crossover(fDomains[id].fittest[parent1]->genes,
														   fDomains[id].fittest[parent2]->genes,
														   true);
							fNumberCreated2Fit++;
							gaPrint( "%5ld: domain %d creation from two (%d, %d) fittest (%4lu, %4lu) %4ld\n", fStep, id, parent1, parent2, fDomains[id].fittest[parent1]->agentID, fDomains[id].fittest[parent2]->agentID, fNumberCreated2Fit );
						}
						else
						{
							// by iterating through the array of fittest
							newAgent->Genes()->crossover(fDomains[id].fittest[fDomains[id].ifit]->genes,
														   fDomains[id].fittest[fDomains[id].jfit]->genes,
														   true);
							fNumberCreated2Fit++;
							gaPrint( "%5ld: domain %d creation from two (%d, %d) fittest (%4lu, %4lu) %4ld\n", fStep, id, fDomains[id].ifit, fDomains[id].jfit, fDomains[id].fittest[fDomains[id].ifit]->agentID, fDomains[id].fittest[fDomains[id].jfit]->agentID, fNumberCreated2Fit );
							ijfitinc(&(fDomains[id].ifit), &(fDomains[id].jfit));
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

				// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
				// ^^^ PARALLEL TASK GrowAgent
				// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
				class GrowAgent : public ITask
				{
				public:
					agent *a;
					GrowAgent( agent *a )
					{
						this->a = a;
					}

					virtual void task_exec( TSimulation *sim )
					{
						a->grow( sim->fMateWait,
								 sim->fRecordGenomes,
								 sim->RecordBrainAnatomy( a->Number() ),
								 sim->RecordBrainFunction( a->Number() ),
								 sim->fRecordPosition );
					}
				};

				// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
				// ^^^ SERIAL TASK UpdateStats
				// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
				class UpdateStats : public ITask
				{
				public:
					agent *a;
					UpdateStats( agent *a )
					{
						this->a = a;
					}

					virtual void task_exec( TSimulation *sim )
					{
						sim->fNeuronGroupCountStats.add( a->GetBrain()->NumNeuronGroups() );

						sim->FoodEnergyIn( a->GetFoodEnergy() );
					}
				};

				// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
				// !!! POST PARALLEL
				// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
				fScheduler.postParallel( new GrowAgent(newAgent) );

				float x = randpw() * (fDomains[id].absoluteSizeX - 0.02) + fDomains[id].startX + 0.01;
				float z = randpw() * (fDomains[id].absoluteSizeZ - 0.02) + fDomains[id].startZ + 0.01;
				float y = 0.5 * agent::gAgentHeight;
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
				fScheduler.postSerial( new UpdateStats(newAgent) );
				
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
            Q_CHECK_PTR(newAgent);

            if( fNumberFit && (fNumberDied >= fNumberFit) )
            {
                // the list exists and is full
                if( fFitness1Frequency
                	&& ((numglobalcreated / fFitness1Frequency) * fFitness1Frequency == numglobalcreated) )
                {
                    // revive 1 fittest
                    newAgent->Genes()->copyFrom( fFittest[0]->genes );
                    fNumberCreated1Fit++;
					gaPrint( "%5ld: global creation from one fittest (%4lu) %4ld\n", fStep, fFittest[0]->agentID, fNumberCreated1Fit );
                }
                else if( fFitness2Frequency && ((numglobalcreated / fFitness2Frequency) * fFitness2Frequency == numglobalcreated) )
                {
                    // mate 2 from array of fittest
					if( fTournamentSize > 0 )
					{
						// using tournament selection
						int parent1, parent2;
						TSimulation::PickParentsUsingTournament(fNumberFit, &parent1, &parent2);
						newAgent->Genes()->crossover( fFittest[parent1]->genes, fFittest[parent2]->genes, true );
						fNumberCreated2Fit++;
						gaPrint( "%5ld: global creation from two (%d, %d) fittest (%4lu, %4lu) %4ld\n", fStep, parent1, parent2, fFittest[parent1]->agentID, fFittest[parent2]->agentID, fNumberCreated2Fit );
					}
					else
					{
						// by iterating through the array of fittest
						newAgent->Genes()->crossover( fFittest[fFitI]->genes, fFittest[fFitJ]->genes, true );
						fNumberCreated2Fit++;
						gaPrint( "%5ld: global creation from two (%d, %d) fittest (%4lu, %4lu) %4ld\n", fStep, fFitI, fFitJ, fFittest[fFitI]->agentID, fFittest[fFitJ]->agentID, fNumberCreated2Fit );
						ijfitinc( &fFitI, &fFitJ );
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

            newAgent->grow( fMateWait,
							fRecordGenomes,
							RecordBrainAnatomy( newAgent->Number() ),
							RecordBrainFunction( newAgent->Number() ),
							fRecordPosition );
			FoodEnergyIn( newAgent->GetFoodEnergy() );

            newAgent->settranslation(randpw() * globals::worldsize, 0.5 * agent::gAgentHeight, randpw() * -globals::worldsize);
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
			fNeuronGroupCountStats.add( newAgent->GetBrain()->NumNeuronGroups() );

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
			fDomains[domain].fBrickPatches[patch].updateOn( fStep );
		}
	}
}

//---------------------------------------------------------------------------
// TSimulation::MaintainFood
//---------------------------------------------------------------------------
void TSimulation::MaintainFood()
{
	// Update on/off state of food patches
	for( int domain = 0; domain < fNumDomains; domain++ )
	{
		for( int patch = 0; patch < fDomains[domain].numFoodPatches; patch++ )
		{
			fDomains[domain].fFoodPatches[patch].updateOn( fStep );
		}
	}

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
					if( !fDomains[domainNumber].fFoodPatches[patchNumber].initFoodGrown() && fDomains[domainNumber].fFoodPatches[patchNumber].on( fStep ) )
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
					float probAdd = (fDomains[domainNumber].maxFoodGrownCount - fDomains[domainNumber].foodCount) * fDomains[domainNumber].foodRate;
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
					if( fDomains[domainNumber].fFoodPatches[patchNumber].on( fStep ) )
					{
						if( fDomains[domainNumber].fFoodPatches[patchNumber].foodCount < fDomains[domainNumber].fFoodPatches[patchNumber].maxFoodGrownCount )
						{

							float probAdd = (fDomains[domainNumber].fFoodPatches[patchNumber].maxFoodGrownCount - fDomains[domainNumber].fFoodPatches[patchNumber].foodCount) * fDomains[domainNumber].fFoodPatches[patchNumber].growthRate;

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
			for( int patch = 0; patch < fDomains[domain].numFoodPatches; patch++ )
			{
				// printf( "%2ld: d=%d, p=%d, on=%d, on1=%d, r=%d\n", fStep, domain, patch, fDomains[domain].fFoodPatches[patch].on( fStep ), fDomains[domain].fFoodPatches[patch].on( fStep+1 ), fDomains[domain].fFoodPatches[patch].removeFood );
				// If the patch is on now, but off in the next time step, and it is supposed to remove its food when it is off...
				if( fDomains[domain].fFoodPatches[patch].removeFood && fDomains[domain].fFoodPatches[patch].turnedOff( fStep ) )
				{
					fFoodPatchesNeedingRemoval[numPatchesNeedingRemoval++] = &(fDomains[domain].fFoodPatches[patch]);
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

	if( fFoodPatchStatsFile )
	{
		fprintf( fFoodPatchStatsFile, "%ld: foodPatchCounts =", fStep );
		for( int domainNumber = 0; domainNumber < fNumDomains; domainNumber++ )
		{
			for( int patchNumber = 0; patchNumber < fDomains[domainNumber].numFoodPatches; patchNumber++ )
			{
				fprintf( fFoodPatchStatsFile, " %d", fDomains[domainNumber].fFoodPatches[patchNumber].foodCount );
			}
		}
		fprintf( fFoodPatchStatsFile, "\n");
	}

    debugcheck( "after growing food and maintaining food patch stats" );
}

//---------------------------------------------------------------------------
// TSimulation::RecordGeneSeparation
//---------------------------------------------------------------------------
void TSimulation::RecordGeneSeparation()
{
	fprintf(fGeneSeparationFile, "%ld %g %g %g\n",
			fStep,
			fMaxGeneSeparation,
			fMinGeneSeparation,
			fAverageGeneSeparation);
}


//---------------------------------------------------------------------------
// TSimulation::CalculateGeneSeparation
//---------------------------------------------------------------------------
void TSimulation::CalculateGeneSeparation(agent* ci)
{
	// TODO add assert to validate statement below
	
	// NOTE: This version assumes ci is *not* currently in gSortedAgents.
    // It also marks the current position in the list on entry and returns
    // there before exit so it can be invoked during the insertion of the
    // newagents list into the existing gSortedAgents list.
	
    //objectxsortedlist::gXSortedObjects.mark();
    objectxsortedlist::gXSortedObjects.setMark( AGENTTYPE );
	
    agent* cj = NULL;
    float genesep;
    float genesepsum = 0.0;
    long numgsvalsold = fNumGeneSepVals;

	objectxsortedlist::gXSortedObjects.reset();
	while (objectxsortedlist::gXSortedObjects.nextObj(AGENTTYPE, (gobject**)&cj))
    {
		genesep = ci->Genes()->separation(cj->Genes());
        fMaxGeneSeparation = fmax(genesep, fMaxGeneSeparation);
        fMinGeneSeparation = fmin(genesep, fMinGeneSeparation);
        genesepsum += genesep;
        fGeneSepVals[fNumGeneSepVals++] = genesep;
    }

    long n = objectxsortedlist::gXSortedObjects.getCount(AGENTTYPE);
    if (numgsvalsold != (n * (n - 1) / 2))
    {
		char tempstring[256];
        sprintf(tempstring,"%s %s %ld %s %ld %s %ld",
				"genesepcalc: numgsvalsold not equal to n * (n - 1) / 2.\n",
				"  numgsvals, n, n * (n - 1) / 2 = ",
				numgsvalsold,", ",n,", ", n * (n - 1) / 2);
		error(2, tempstring);
    }

    if (fNumGeneSepVals != (n * (n + 1) / 2))
    {
		char tempstring[256];
        sprintf(tempstring,"%s %s %ld %s %ld %s %ld",
			 	"genesepcalc: numgsvals not equal to n * (n + 1) / 2.\n",
				"  numgsvals, n, n * (n + 1) / 2 = ",
				fNumGeneSepVals,", ",n,", ",n*(n+1)/2);
        error(2,tempstring);
    }

    fAverageGeneSeparation = (genesepsum + fAverageGeneSeparation * numgsvalsold) / fNumGeneSepVals;
	
    //objectxsortedlist::gXSortedObjects.tomark();
    objectxsortedlist::gXSortedObjects.toMark(AGENTTYPE);
}



//---------------------------------------------------------------------------
// TSimulation::CalculateGeneSeparationAll
//---------------------------------------------------------------------------
void TSimulation::CalculateGeneSeparationAll()
{
    agent* ci = NULL;
    agent* cj = NULL;

    float genesep;
    float genesepsum = 0.0;
    fMinGeneSeparation = 1.e+10;
    fMaxGeneSeparation = 0.0;
    fNumGeneSepVals = 0;

    objectxsortedlist::gXSortedObjects.reset();
    while (objectxsortedlist::gXSortedObjects.nextObj(AGENTTYPE, (gobject**)&ci))
    {
		objectxsortedlist::gXSortedObjects.setMark(AGENTTYPE);

		while (objectxsortedlist::gXSortedObjects.nextObj(AGENTTYPE, (gobject**)&cj))
		{	
            genesep = ci->Genes()->separation(cj->Genes());
            fMaxGeneSeparation = max(genesep, fMaxGeneSeparation);
            fMinGeneSeparation = min(genesep, fMinGeneSeparation);
            genesepsum += genesep;
            fGeneSepVals[fNumGeneSepVals++] = genesep;
        }
		//objectxsortedlist::gXSortedObjects.tomark();
		objectxsortedlist::gXSortedObjects.toMark(AGENTTYPE);
    }

    // n * (n - 1) / 2 is how many calculations were made
	long n = objectxsortedlist::gXSortedObjects.getCount(AGENTTYPE);
    if (fNumGeneSepVals != (n * (n - 1) / 2))
    {
		char tempstring[256];
        sprintf(tempstring, "%s %s %ld %s %ld %s %ld",
            "genesepcalcall: numgsvals not equal to n * (n - 1) / 2.\n",
            "  numgsvals, n, n * (n - 1) / 2 = ",
            fNumGeneSepVals,", ",n,", ",n * (n - 1) / 2);
        error(2, tempstring);
    }

    fAverageGeneSeparation = genesepsum / fNumGeneSepVals;
}


//---------------------------------------------------------------------------
// TSimulation::ijfitinc
//---------------------------------------------------------------------------
void TSimulation::ijfitinc(short* i, short* j)
{
    (*j)++;

    if ((*j) == (*i))
        (*j)++;

    if ((*j) > fNumberFit - 1)
    {
        (*j) = 0;
        (*i)++;

        if ((*i) > fNumberFit - 1)
		{
            (*i) = 0;	// min(1, fNumberFit - 1);
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
	if( a )	// a will NULL for virtual births only
	{
		fNumberAlive++;
		fNumberAliveWithMetabolism[ GenomeUtil::getMetabolism(a->Genes())->index ]++;
	
		// ---
		// --- Update LifeSpan
		// ---
		a->GetLifeSpan()->set_birth( fStep,
									 reason );
	
	
		// ---
		// --- Update Separation Cache
		// ---
		fSeparationCache.birth( a );
	}
	
	// ---
	// --- Update Birth/Death Log
	// ---
	if( fRecordBirthsDeaths && (reason != LifeSpan::BR_SIMINIT) )
	{
		FILE * File;
		if( (File = fopen("run/BirthsDeaths.log", "a")) == NULL )
		{
			cerr << "could not open run/BirthsDeaths.log for writing. Exiting." << endl;
			exit(1);
		}

		switch( reason )
		{
		case LifeSpan::BR_NATURAL:
		case LifeSpan::BR_LOCKSTEP:
			fprintf( File,
					 "%ld BIRTH %ld %ld %ld\n",
					 fStep,
					 a->Number(),
					 a_parent1->Number(),
					 a_parent2->Number() );
			if( fEvents )
			{
				fEvents->AddEvent( fStep, a_parent1->Number(), 'm' );
				fEvents->AddEvent( fStep, a_parent2->Number(), 'm' );
			}
			break;
		case LifeSpan::BR_VIRTUAL:
			fprintf( File,
					 "%ld VIRTUAL %d %ld %ld\n",
					 fStep,
					 0,	// agent IDs start with 1, so this also identifies virtual births
					 a_parent1->Number(),
					 a_parent2->Number() );
			if( fEvents )
			{
				fEvents->AddEvent( fStep, a_parent1->Number(), 'm' );
				fEvents->AddEvent( fStep, a_parent2->Number(), 'm' );
			}
			break;
		case LifeSpan::BR_CREATE:
			fprintf( File,
					 "%ld CREATION %ld\n",
					 fStep,
					 a->Number() );
			break;
		default:
			assert( false );
		}

		fclose(File);
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
	fNumberAlive--;
	fNumberAliveWithMetabolism[c->GetMetabolism()->index]--;

	// ---
	// --- Update LifeSpan
	// ---
	c->GetLifeSpan()->set_death( fStep,
								 reason );

	UpdateLifeSpanLog( c );

	// ---
	// --- Update Separation Cache
	// ---
	fSeparationCache.death( c );

	if( reason == LifeSpan::DR_SIMEND )
	{
		c->Die();

		return;
	}

	Q_CHECK_PTR(c);
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

			if( foodEnergy.isDepleted(minFoodEnergy) )
			{
				FoodEnergyOut( foodEnergy );
			}
			else
			{
				food* f = new food( carcassFoodType, fStep, foodEnergy, c->x(), c->z() );
				Q_CHECK_PTR( f );
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
	c->Die();

	// ---
	// --- Remove From Stage
	// ---
    fStage.RemoveObject( c );

	// ---
	// --- Update Monitor Window
	// ---
	if (c == fMonitorAgent)
	{
		fMonitorAgent = NULL;

		// Stop monitoring agent brain
		if (fBrainMonitorWindow != NULL)
			fBrainMonitorWindow->StopMonitoring();
	}

	// ---
	// --- Update Overhead Agent
	// ---
	if (c == fOverheadAgent)
	{
		fOverheadAgent = NULL;
	}

	// ---
	// --- Births Deaths Log
	// ---
	if( fRecordBirthsDeaths )		// are we recording births, deaths, and creations?  If so, this agent will be included.
	{
		FILE * File;
		
		if( (File = fopen("run/BirthsDeaths.log", "a")) == NULL )
		{
			cerr << "could not open run/genome/run/BirthsDeaths.log for writing [1].  Exiting." << endl;
			exit(1);
		}
		
		fprintf( File, "%ld DEATH %ld\n", fStep, c->Number() );
		fclose( File );
	}
	
	// ---
	// --- x-sorted list
	// ---
	// following assumes (requires!) list to be currently pointing to c,
    // and will leave the list pointing to the previous agent
	// agent::gXSortedAgents.remove(); // get agent out of the list
	// objectxsortedlist::gXSortedObjects.removeCurrentObject(); // get agent out of the list
	
	// Following assumes (requires!) the agent to have stored c->listLink correctly
	objectxsortedlist::gXSortedObjects.removeObjectWithLink( (gobject*) c );

	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
	// ^^^ PARALLEL TASK UpdateBrainData
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
	class UpdateBrainData : public ITask
	{
	public:
		agent *a;
		UpdateBrainData( agent *a )
		{
			this->a = a;
		}

		virtual void task_exec( TSimulation *sim )
		{
			sim->Kill_UpdateBrainData( a );
		}
	};

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// !!! POST PARALLEL
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	fScheduler.postParallel( new UpdateBrainData(c) );

	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
	// ^^^ SERIAL TASK UpdateFittestAndDeleteAgent
	// ^^^
	// ^^^ It's important that we not deallocate the agent from
	// ^^^ memory until all parallel tasks are done. For example,
	// ^^^ The RecordGeneStats task might need this agent's
	// ^^^ genome.
	// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
	class UpdateFittestAndDeleteAgent : public ITask
	{
	public:
		agent *a;
		UpdateFittestAndDeleteAgent( agent *a )
		{
			this->a = a;
		}

		virtual void task_exec( TSimulation *sim )
		{
			sim->Kill_UpdateFittest( a );

			// Note: For the sake of computational efficiency, I used to never delete an agent,
			// but "reinit" and reuse them as new agents were born or created.  But Gene made
			// agents be allocated afresh on birth or creation, so we now need to delete the
			// old ones here when they die.  Remove this if I ever get a chance to go back to the
			// more efficient reinit and reuse technique.
			delete a;
		}
	};

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// !!! POST SERIAL
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	fScheduler.postSerial( new UpdateFittestAndDeleteAgent(c) );
}

#define HackForProcessingUnitComplexity 1

//---------------------------------------------------------------------------
// TSimulation::Kill_UpdateBrainData
//---------------------------------------------------------------------------
void TSimulation::Kill_UpdateBrainData( agent *c )
{
	//printf( "%s (beginning): %ld %g\n", __func__, c->Number(), c->Complexity() );

	c->EndFunctional();

#if HackForProcessingUnitComplexity
	if(  fBrainAnatomyRecordAll					||
		 fBestSoFarBrainAnatomyRecordFrequency	||
		 fBestRecentBrainAnatomyRecordFrequency	||
		 (fBrainAnatomyRecordSeeds && (c->Number() <= fInitNumAgents))
		 )
		c->GetBrain()->dumpAnatomical( "run/brain/anatomy", "death", c->Number(), c->HeuristicFitness() );
#endif

	if( RecordBrainFunction( c->Number() ) )
	{
		char s[256];
		char t[256];
		sprintf( s, "run/brain/function/incomplete_brainFunction_%ld.txt", c->Number() );
		sprintf( t, "run/brain/function/brainFunction_%ld.txt", c->Number() );
		rename( s, t );

		if ( fComplexityFitnessWeight != 0.0 || fRecordComplexity )		// Are we using Complexity as a Fitness Function, or computing and saving it on the fly?  If so, calculate Complexity here
		{
			if( fComplexityType == "D" )	// special case the difference of complexities case
			{
				float pComplexity = CalcComplexity_brainfunction( t, "P" );
				float iComplexity = CalcComplexity_brainfunction( t, "I" );
				c->SetComplexity( pComplexity - iComplexity );
			}
			else if( fComplexityType != "Z" )	// avoid special hack case to evolve towards zero max velocity, for testing purposes only
			{
				// otherwise, fComplexityType has the right string in it
				c->SetComplexity( CalcComplexity_brainfunction( t, fComplexityType.c_str(), fEvents ) );
			}
			if( fRecordComplexity )
				UpdateComplexityLog( c );
		}
		
		if( fBrainFunctionRecentRecordFrequency )
		{
			// Note: Everywhere else I use 's' for source and 't' for target, but in this case
			// what was just the target is now the source, so to avoid copying those bytes
			// again I'll make an exception and reverse the roles of s and t here for efficiency...
			sprintf( s, "run/brain/Recent/%ld/brainFunction_%ld.txt", fCurrentRecentEpoch, c->Number() );
			if( link( t, s ) )
				eprintf( "Error (%d) linking from \"%s\" to \"%s\"\n", errno, t, s );
			
			if( c->Number() <= fInitNumAgents )
			{
				sprintf( s, "run/brain/Recent/0/brainFunction_%ld.txt", c->Number() );
				if( link( t, s ) )
					eprintf( "Error (%d) linking from \"%s\" to \"%s\"\n", errno, t, s );
			}
		}
	}
	//printf( "%s (end): %ld %g\n", __func__, c->Number(), c->Complexity() );
}

//---------------------------------------------------------------------------
// TSimulation::Kill_UpdateFittest
//---------------------------------------------------------------------------
void TSimulation::Kill_UpdateFittest( agent *c )
{
	//printf( "%s (beginning): %ld %g\n", __func__, c->Number(), c->Complexity() );

	unsigned long loserIDBestSoFar = 0;	// the way this is used depends on fAgentNumber being 1-based in agent.cp
	unsigned long loserIDBestRecent = 0;	// the way this is used depends on fAgentNumber being 1-based in agent.cp
	bool oneOfTheBestSoFar = false;
	bool oneOfTheBestRecent = false;
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
	// First on a domain-by-domain basis...
	int newfit = 0;
	FitStruct* saveFit;
	float cFitness = AgentFitness( c );
	if( (fDomains[id].numdied <= fNumberFit) || (cFitness > fDomains[id].fittest[fNumberFit-1]->fitness) )
	{
		int limit = fDomains[id].numdied < fNumberFit ? fDomains[id].numdied : fNumberFit;
		newfit = limit - 1;
		for( int i = 0; i < limit; i++ )
		{
			if( cFitness > fDomains[id].fittest[i]->fitness )
			{
				newfit = i;
				break;
			}
		}
		
		// Note: This does some unnecessary work while numdied is less than fNumberFit,
		// but it's not a big deal and doesn't hurt anything, and I don't want to deal
		// with the logic to handle the newfit == limit case (adding a new one on the end)
		// right now.
		saveFit = fDomains[id].fittest[fNumberFit-1];				// this is to save the data structure, not its contents
		for( short i = fNumberFit - 1; i > newfit; i-- )
			fDomains[id].fittest[i] = fDomains[id].fittest[i-1];
		fDomains[id].fittest[newfit] = saveFit;						// reuse the old data structure, but replace its contents...
		fDomains[id].fittest[newfit]->fitness = cFitness;
		fDomains[id].fittest[newfit]->genes->copyFrom( c->Genes() );
		fDomains[id].fittest[newfit]->agentID = c->Number();
		fDomains[id].fittest[newfit]->complexity = c->Complexity();
		gaPrint( "%5ld: new domain %d fittest[%ld] id=%4ld fitness=%7.3f complexity=%7.3f\n", fStep, id, newfit, c->Number(), cFitness, c->Complexity() );
	}

	// Then on a whole-world basis...
	if( (fNumberDied <= fNumberFit) || (cFitness > fFittest[fNumberFit-1]->fitness) )
	{
		oneOfTheBestSoFar = true;
		
		int limit = fNumberDied < fNumberFit ? fNumberDied : fNumberFit;
		newfit = limit - 1;
		for( short i = 0; i < limit; i++ )
		{
			if( cFitness > fFittest[i]->fitness )
			{
				newfit = i;
				break;
			}
		}
				
		// Note: This does some unnecessary work while numdied is less than fNumberFit,
		// but it's not a big deal and doesn't hurt anything, and I don't want to deal
		// with the logic to handle the newfit == limit case (adding a new one on the end)
		// right now.
		if( fNumberDied > fNumberFit )
			loserIDBestSoFar = fFittest[fNumberFit - 1]->agentID;	// this is the ID of the agent that is being booted from the bestSoFar (fFittest[]) list
		else
			loserIDBestSoFar = 0;	// nobody is being booted, because the list isn't full yet
		saveFit = fFittest[fNumberFit - 1];		// this is to save the data structure, not its contents
		for( short i = fNumberFit - 1; i > newfit; i-- )
			fFittest[i] = fFittest[i - 1];
		fFittest[newfit] = saveFit;				// reuse the old data structure, but replace its contents...
		fFittest[newfit]->fitness = cFitness;
		fFittest[newfit]->genes->copyFrom( c->Genes() );
		fFittest[newfit]->agentID = c->Number();
		fFittest[newfit]->complexity = c->Complexity();
		gaPrint( "%5ld: new global   fittest[%ld] id=%4ld fitness=%7.3f complexity=%7.3f\n", fStep, newfit, c->Number(), cFitness, c->Complexity() );
	}

	if (cFitness > fMaxFitness)
		fMaxFitness = cFitness;
	
	// Keep a separate list of the recent fittest, purely for data-gathering purposes,
	// also based on complete fitness, however it is being currently being calculated
	// "Recent" means since the last archival recording of recent best, as determined by fBestRecentBrainAnatomyRecordFrequency
	// (Don't bother, if we're not gathering that kind of data)
	if( fBestRecentBrainAnatomyRecordFrequency && (cFitness > fRecentFittest[fNumberRecentFit-1]->fitness) )
	{
		oneOfTheBestRecent = true;
		
		for( short i = 0; i < fNumberRecentFit; i++ )
		{
			// If the agent booted off of the bestSoFar list happens to remain on the bestRecent list,
			// then we don't want to let it be unlinked below, so clear loserIDBestSoFar
			// This loop tests the first part of the fRecentFittest[] list, and the last part is tested
			// below, so we don't need a separate loop to make this determination
			if( loserIDBestSoFar == fRecentFittest[i]->agentID )
				loserIDBestSoFar = 0;
			
			if( cFitness > fRecentFittest[i]->fitness )
			{
				newfit = i;
				break;
			}
		}
				
		loserIDBestRecent = fRecentFittest[fNumberRecentFit - 1]->agentID;	// this is the ID of the agent that is being booted from the bestRecent (fRecentFittest[]) list
		saveFit = fRecentFittest[fNumberRecentFit - 1];		// this is to save the data structure, not its contents
		for( short i = fNumberRecentFit - 1; i > newfit; i-- )
		{
			// If the agent booted off of the bestSoFar list happens to remain on the bestRecent list,
			// then we don't want to let it be unlinked below, so clear loserIDBestSoFar
			// This loop tests the last part of the fRecentFittest[] list, and the first part was tested
			// above, so we don't need a separate loop to make this determination
			if( loserIDBestSoFar == fRecentFittest[i]->agentID )
				loserIDBestSoFar = 0;
			
			fRecentFittest[i] = fRecentFittest[i - 1];
		}
		fRecentFittest[newfit] = saveFit;				// reuse the old data structure, but replace its contents...
		fRecentFittest[newfit]->fitness = cFitness;
		//		fRecentFittest[newfit]->genes->copyFrom( c->Genes() );	// we don't save the genes in the bestRecent list
		fRecentFittest[newfit]->agentID = c->Number();
		fRecentFittest[newfit]->complexity = c->Complexity();
	}

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

	// If we're recording all anatomies or recording best anatomies and this was one of the fittest agents,
	// then dump the anatomy to the appropriate location on disk
	if(  fBrainAnatomyRecordAll ||
		 (fBestSoFarBrainAnatomyRecordFrequency && oneOfTheBestSoFar) ||
		 (fBestRecentBrainAnatomyRecordFrequency && oneOfTheBestRecent) ||
		 (fBrainAnatomyRecordSeeds && (c->Number() <= fInitNumAgents))
		 )
	{
#if ! HackForProcessingUnitComplexity
		c->GetBrain()->dumpAnatomical( "run/brain/anatomy", "death", c->Number(), c->HeuristicFitness() );
#endif
	}
	else if( fBestRecentBrainAnatomyRecordFrequency || fBestSoFarBrainAnatomyRecordFrequency )	// don't want brain anatomies for this agent, so must eliminate the "incept" and "birth" anatomies if they were recorded
	{
		char s[256];
	
		sprintf( s, "run/brain/anatomy/brainAnatomy_%lu_incept.txt", c->Number() );
		if( unlink( s ) )
			eprintf( "Error (%d) unlinking \"%s\"\n", errno, s );
		sprintf( s, "run/brain/anatomy/brainAnatomy_%lu_birth.txt", c->Number() );
		if( unlink( s ) )
			eprintf( "Error (%d) unlinking \"%s\"\n", errno, s );

#if HackForProcessingUnitComplexity
		// I'm pretty sure the following tests are unnecessary but don't hurt anything (we should be able to just unlink the _death file if we reach here) - lsy 4/9/10
		// Turns out we didn't want the "death" anatomy either, but we saved it above for the complexity measure
		if( (fBestSoFarBrainAnatomyRecordFrequency  && !oneOfTheBestSoFar  && (!fBestRecentBrainAnatomyRecordFrequency || !oneOfTheBestRecent))	||
			(fBestRecentBrainAnatomyRecordFrequency	&& !oneOfTheBestRecent && (!fBestSoFarBrainAnatomyRecordFrequency  || !oneOfTheBestSoFar ))
			)
		{
			sprintf( s, "run/brain/anatomy/brainAnatomy_%lu_death.txt", c->Number() );
			if( unlink( s ) )
				eprintf( "Error (%d) unlinking \"%s\"\n", errno, s );
		}
#endif
	}
		
	// If this was one of the seed agents and we're recording their anatomies, then save the data in the appropriate directory
	if( fBrainAnatomyRecordSeeds && (c->Number() <= fInitNumAgents) )
	{
		char s[256];	// source
		char t[256];	// target
		// Determine whether the original needs to stay around or not.  If so, create a hard link for the
		// copy in "seeds"; if not, just rename (mv) the file into seeds, thus removing it from the original location
		bool keep = ( fBrainAnatomyRecordAll || (fBestSoFarBrainAnatomyRecordFrequency && oneOfTheBestSoFar) || (fBestRecentBrainAnatomyRecordFrequency && oneOfTheBestRecent) );
		
		sprintf( s, "run/brain/anatomy/brainAnatomy_%ld_incept.txt", c->Number() );
		sprintf( t, "run/brain/seeds/anatomy/brainAnatomy_%ld_incept.txt", c->Number() );
		if( keep )
		{
			if( link( s, t ) )
				eprintf( "Error (%d) linking from \"%s\" to \"%s\"\n", errno, s, t );
		}
		else
		{
			if( rename( s, t ) )
				eprintf( "Error (%d) renaming from \"%s\" to \"%s\"\n", errno, s, t );
		}
		sprintf( s, "run/brain/anatomy/brainAnatomy_%ld_birth.txt", c->Number() );
		sprintf( t, "run/brain/seeds/anatomy/brainAnatomy_%ld_birth.txt", c->Number() );
		if( keep )
		{
			if( link( s, t ) )
				eprintf( "Error (%d) linking from \"%s\" to \"%s\"\n", errno, s, t );
		}
		else
		{
			if( rename( s, t ) )
				eprintf( "Error (%d) renaming from \"%s\" to \"%s\"\n", errno, s, t );
		}
		sprintf( s, "run/brain/anatomy/brainAnatomy_%ld_death.txt", c->Number() );
		sprintf( t, "run/brain/seeds/anatomy/brainAnatomy_%ld_death.txt", c->Number() );
		if( keep )
		{
			if( link( s, t ) )
				eprintf( "Error (%d) linking from \"%s\" to \"%s\"\n", errno, s, t );
		}
		else
		{
			if( rename( s, t ) )
				eprintf( "Error (%d) renaming from \"%s\" to \"%s\"\n", errno, s, t );
		}
	}
	
	// If this agent was so good it displaced another agent from the bestSoFar (fFittest[]) list,
	// then nuke the booted agent's brain anatomy & function recordings
	// Note: A agent may be booted from the bestSoFar list, yet remain on the bestRecent list;
	// if this happens, loserIDBestSoFar will be reset to zero above, so we won't execute this block
	// of code and delete files that are needed for the bestRecent data logging
	// In the rare case that a agent is simultaneously booted from both best lists,
	// don't delete it here (so we don't try to delete it more than once)
	if( loserIDBestSoFar && (loserIDBestSoFar != loserIDBestRecent) )	//  depends on fAgentNumber being 1-based in agent.cp
	{
		char s[256];
		if( (fBestRecentBrainAnatomyRecordFrequency || fBestSoFarBrainAnatomyRecordFrequency) && !fBrainAnatomyRecordAll )
		{
			sprintf( s, "run/brain/anatomy/brainAnatomy_%lu_incept.txt", loserIDBestSoFar );
			if( unlink( s ) )
				eprintf( "Error (%d) unlinking \"%s\"\n", errno, s );
			sprintf( s, "run/brain/anatomy/brainAnatomy_%lu_birth.txt", loserIDBestSoFar );
			if( unlink( s ) )
				eprintf( "Error (%d) unlinking \"%s\"\n", errno, s );
			sprintf( s, "run/brain/anatomy/brainAnatomy_%lu_death.txt", loserIDBestSoFar );
			if( unlink( s ) )
				eprintf( "Error (%d) unlinking \"%s\"\n", errno, s );
		}
		if( (fBestRecentBrainFunctionRecordFrequency || fBestSoFarBrainFunctionRecordFrequency) && !fBrainFunctionRecordAll && !fBrainFunctionRecentRecordFrequency )
		{
			sprintf( s, "run/brain/function/brainFunction_%lu.txt", loserIDBestSoFar );
			if( unlink( s ) )
				eprintf( "Error (%d) unlinking \"%s\"\n", errno, s );
		}
	}

	// If this agent was so good it displaced another agent from the bestRecent (fRecentFittest[]) list,
	// then nuke the booted agent's brain anatomy & function recordings
	// In the rare case that a agent is simultaneously booted from both best lists,
	// we will only delete it here
	if( loserIDBestRecent )	//  depends on fAgentNumber being 1-based in agent.cp
	{
		char s[256];
		if( (fBestRecentBrainAnatomyRecordFrequency || fBestSoFarBrainAnatomyRecordFrequency) && !fBrainAnatomyRecordAll )
		{
			sprintf( s, "run/brain/anatomy/brainAnatomy_%lu_incept.txt", loserIDBestRecent );
			if( unlink( s ) )
				eprintf( "Error (%d) unlinking \"%s\"\n", errno, s );
			sprintf( s, "run/brain/anatomy/brainAnatomy_%lu_birth.txt", loserIDBestRecent );
			if( unlink( s ) )
				eprintf( "Error (%d) unlinking \"%s\"\n", errno, s );
			sprintf( s, "run/brain/anatomy/brainAnatomy_%lu_death.txt", loserIDBestRecent );
			if( unlink( s ) )
				eprintf( "Error (%d) unlinking \"%s\"\n", errno, s );
		}
		if( (fBestRecentBrainFunctionRecordFrequency || fBestSoFarBrainFunctionRecordFrequency) && !fBrainFunctionRecordAll && !fBrainFunctionRecentRecordFrequency )
		{
			sprintf( s, "run/brain/function/brainFunction_%lu.txt", loserIDBestRecent );
			if( unlink( s ) )
				eprintf( "Error (%d) unlinking \"%s\"\n", errno, s );
		}
	}

	// If this was one of the seed agents and we're recording their functioning, then save the data in the appropriate directory
	// NOTE:  Must be done after c->Die(), as that is where the brainFunction file is closed
	if( fBrainFunctionRecordSeeds && (c->Number() <= fInitNumAgents) )
	{
		char s[256];	// source
		char t[256];	// target
		
		sprintf( s, "run/brain/function/brainFunction_%ld.txt", c->Number() );
		sprintf( t, "run/brain/seeds/function/brainFunction_%ld.txt", c->Number() );
		if( link( s, t ) )
			eprintf( "Error (%d) linking from \"%s\" to \"%s\"\n", errno, s, t );
	}
	
	// If we're only archiving the best, and this isn't one of them, then nuke its brainFunction recording
	if( (fBestSoFarBrainFunctionRecordFrequency || fBestRecentBrainFunctionRecordFrequency) &&
		(!oneOfTheBestSoFar && !oneOfTheBestRecent) &&
		!fBrainFunctionRecordAll && !fBrainFunctionRecentRecordFrequency )
	{
		char s[256];
		sprintf( s, "run/brain/function/brainFunction_%lu.txt", c->Number() );
		if( unlink( s ) )
			eprintf( "Error (%d) unlinking \"%s\"\n", errno, s );
	}

	//printf( "%s (end): %ld %g\n", __func__, c->Number(), c->Complexity() );
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
	//printf( "%s (beginning): %ld %g\n", __func__, c->Number(), c->Complexity() );
	
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
		//cout << "fitness" eql fitness sp "hwt" eql fHeuristicFitnessWeight sp "hf" eql c->HeuristicFitness()/fTotalHeuristicFitness sp "cwt" eql fComplexityFitnessWeight sp "cf" eql c->Complexity() nl;
	}
	
	//printf( "%s (end): %ld %g (%g)\n", __func__, c->Number(), c->Complexity(), fitness );

	return( fitness );
}

//-------------------------------------------------------------------------------------------
// TSimulation::ProcessWorldFile
//-------------------------------------------------------------------------------------------
void TSimulation::ProcessWorldFile( proplib::Document *docWorldFile )
{
	proplib::Document &doc = *docWorldFile;

	fLockStepWithBirthsDeathsLog = doc.get( "PassiveLockstep" );
	fMaxSteps = doc.get( "MaxSteps" );
	fEndOnPopulationCrash = doc.get( "EndOnPopulationCrash" );
	fDumpFrequency = doc.get( "CheckPointFrequency" );
	fStatusFrequency = doc.get( "StatusFrequency" );
	fStatusToStdout = doc.get( "StatusToStdout" );
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
	agent::gVision = doc.get( "Vision" );
	fShowVision = doc.get( "ShowVision" );
	fStaticTimestepGeometry = doc.get( "StaticTimestepGeometry" );
	fParallelInitAgents = doc.get( "ParallelInitAgents" );
	fParallelInteract = doc.get( "ParallelInteract" );
	fParallelCreateAgents = doc.get( "ParallelCreateAgents" );
	fParallelBrains = doc.get( "ParallelBrains" );
	brain::gMinWin = doc.get( "RetinaWidth" );
	agent::gMaxVelocity = doc.get( "MaxVelocity" );
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
	fFoodRemoveEnergy = doc.get( "FoodRemoveEnergy" );
	food::gMaxLifeSpan = doc.get( "FoodMaxLifeSpan" );
    fPositionSeed = doc.get( "PositionSeed" );
    fGenomeSeed = doc.get( "InitSeed" );
	fSimulationSeed = doc.get( "SimulationSeed" );
	{
		proplib::Property &rfood = doc.get( "AgentsAreFood" );
		if( (string)rfood == "Fight" )
			fAgentsRfood = RFOOD_TRUE__FIGHT_ONLY;
		else if ( (bool)rfood )
			fAgentsRfood = RFOOD_TRUE;
		else
			fAgentsRfood = RFOOD_FALSE;
	}
	fFitness1Frequency = doc.get( "EliteFrequency" );
    fFitness2Frequency = doc.get( "PairFrequency" );
    fNumberFit = doc.get( "NumberFittest" );
	fNumberRecentFit = doc.get( "NumberRecentFittest" );
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
		string layout = doc.get( "GenomeLayout" );
		if( layout == "N" )
			genome::gLayoutType = genome::GenomeLayout::NEURGROUP;
		else if( layout == "L" )
			genome::gLayoutType = genome::GenomeLayout::LEGACY;
		else
			assert( false );
	}
	genome::gEnableMateWaitFeedback = doc.get( "EnableMateWaitFeedback" );
	genome::gEnableSpeedFeedback = doc.get( "EnableSpeedFeedback" );
	genome::gEnableGive = doc.get( "EnableGive" );
	genome::gEnableCarry = doc.get( "EnableCarry" );
	agent::gMaxCarries = doc.get( "MaxCarries" );
	{
		fCarryObjects = 0;
#define __SET( PROP, MASK ) if( (bool)doc.get("Carry"PROP) ) fCarryObjects |= MASK##TYPE
		__SET( "Agents", AGENT );
		__SET( "Food", FOOD );
		__SET( "Bricks", BRICK );
#undef __SET
	}
	{
		fShieldObjects = 0;
#define __SET( PROP, MASK ) if( (bool)doc.get("Shield"PROP) ) fShieldObjects |= MASK##TYPE
		__SET( "Agents", AGENT );
		__SET( "Food", FOOD );
		__SET( "Bricks", BRICK );
#undef __SET
	}
	fCarryPreventsEat = doc.get( "CarryPreventsEat" );
	fCarryPreventsFight = doc.get( "CarryPreventsFight" );
	fCarryPreventsGive = doc.get( "CarryPreventsGive" );
	fCarryPreventsMate = doc.get( "CarryPreventsMate" );

	genome::gEnableVisionPitch = doc.get( "EnableVisionPitch" );
	agent::gMinVisionPitch = doc.get( "MinVisionPitch" );
	agent::gMaxVisionPitch = doc.get( "MaxVisionPitch" );
	genome::gEnableVisionYaw = doc.get( "EnableVisionYaw" );
	agent::gMinVisionYaw = doc.get( "MinVisionYaw" );
	agent::gMaxVisionYaw = doc.get( "MaxVisionYaw" );
	agent::gEyeHeight = doc.get( "EyeHeight" );

    genome::gMinvispixels = doc.get( "MinVisionNeuronsPerGroup" );
    genome::gMaxvispixels = doc.get( "MaxVisionNeuronsPerGroup" );
    genome::gMinMutationRate = doc.get( "MinMutationRate" );
    genome::gMaxMutationRate = doc.get( "MaxMutationRate" );
    genome::gMinNumCpts = doc.get( "MinCrossoverPoints" );
    genome::gMaxNumCpts = doc.get( "MaxCrossoverPoints" );
    genome::gMinLifeSpan = doc.get( "MinLifeSpan" );
    genome::gMaxLifeSpan = doc.get( "MaxLifeSpan" );
    fMateWait = doc.get( "MateWait" );
    agent::gInitMateWait = doc.get( "InitMateWait" );
	fEatMateSpan = doc.get( "EatMateWait" );
	{
		proplib::Property &propEatMateMinDistance = doc.get( "EatMateMinDistance" );

		proplib::Property &propConditions = propEatMateMinDistance.get( "Conditions" );
		condprop::ConditionList<float> *conditions = new condprop::ConditionList<float>();
		int nconditions = propConditions.size();

		for( int i = 0; i < nconditions; i++ )
		{
			proplib::Property &propCondition = propConditions.get( i );

			CouplingRange couplingRange;
			{
				string role = propCondition.get( "CouplingRange" );
				if( role == "Begin" )
					couplingRange = COUPLING_RANGE__BEGIN;
				else if( role == "End" )
					couplingRange = COUPLING_RANGE__END;
				else if( role == "None" )
					couplingRange = COUPLING_RANGE__NONE;
				else
					assert( false );
			}

			float value = propCondition.get( "Value" );
			string mode = propCondition.get( "Mode" );

			condprop::Condition<float> *condition;
			if( mode == "Time" )
			{
				long step = propCondition.get( "Time" );
				condition = new condprop::TimeCondition<float>( step, value, couplingRange );
			}
			else if( mode == "IntThreshold" )
			{
				proplib::Property &propCount = propCondition.get( "IntThreshold" );
				string countValue = propCount.get( "Value" );
				string opname = propCount.get( "Op" );
				int threshold = propCount.get( "Threshold" );
				long duration = propCount.get( "Duration" );
				
				const int *countPtr;
				if( countValue == "Agents" )
				{
					countPtr = objectxsortedlist::gXSortedObjects.getCountPtr( AGENTTYPE );
				}
				else
					assert( false );

				condprop::ThresholdCondition<float,int>::Op op;
				if( opname == "EQ" )
					op = condprop::ThresholdCondition<float,int>::EQ;
				else if( opname == "LT" )
					op = condprop::ThresholdCondition<float,int>::LT;
				else if( opname == "GT" )
					op = condprop::ThresholdCondition<float,int>::GT;
				else
					assert( false );

				condition = new condprop::ThresholdCondition<float,int>( countPtr,
																		 op,
																		 threshold,
																		 duration,
																		 value,
																		 couplingRange );
			}
			else
				assert( false );

			conditions->add( condition );
		}

		condprop::Logger<float> *logger = NULL;
		if( (bool)propEatMateMinDistance.get( "Record" ) )
		{
			logger = new condprop::ScalarLogger<float>("EatMateMinDistance",
													   datalib::FLOAT);
		}

		condprop::Property<float> *property =
			new condprop::Property<float>( "EatMateMinDistance",
										   &fEatMateMinDistance,
										   &condprop::InterpolateFunction_float,
										   &condprop::DistanceFunction_float,
										   conditions,
										   logger );

		fConditionalProps->add( property );
	}
	{
		proplib::Property &propMinEatVelocity = doc.get( "MinEatVelocity" );

		proplib::Property &propConditions = propMinEatVelocity.get( "Conditions" );
		condprop::ConditionList<float> *conditions = new condprop::ConditionList<float>();
		int nconditions = propConditions.size();

		for( int i = 0; i < nconditions; i++ )
		{
			proplib::Property &propCondition = propConditions.get( i );

			CouplingRange couplingRange;
			{
				string role = propCondition.get( "CouplingRange" );
				if( role == "Begin" )
					couplingRange = COUPLING_RANGE__BEGIN;
				else if( role == "End" )
					couplingRange = COUPLING_RANGE__END;
				else if( role == "None" )
					couplingRange = COUPLING_RANGE__NONE;
				else
					assert( false );
			}

			float value = propCondition.get( "Value" );
			string mode = propCondition.get( "Mode" );

			condprop::Condition<float> *condition;
			if( mode == "Time" )
			{
				long step = propCondition.get( "Time" );
				condition = new condprop::TimeCondition<float>( step, value, couplingRange );
			}
			else if( mode == "IntThreshold" )
			{
				proplib::Property &propCount = propCondition.get( "IntThreshold" );
				string countValue = propCount.get( "Value" );
				string opname = propCount.get( "Op" );
				int threshold = propCount.get( "Threshold" );
				long duration = propCount.get( "Duration" );
				
				const int *countPtr;
				if( countValue == "Agents" )
				{
					countPtr = objectxsortedlist::gXSortedObjects.getCountPtr( AGENTTYPE );
				}
				else
					assert( false );

				condprop::ThresholdCondition<float,int>::Op op;
				if( opname == "EQ" )
					op = condprop::ThresholdCondition<float,int>::EQ;
				else if( opname == "LT" )
					op = condprop::ThresholdCondition<float,int>::LT;
				else if( opname == "GT" )
					op = condprop::ThresholdCondition<float,int>::GT;
				else
					assert( false );

				condition = new condprop::ThresholdCondition<float,int>( countPtr,
																		 op,
																		 threshold,
																		 duration,
																		 value,
																		 couplingRange );
			}
			else if( mode == "FloatThreshold" )
			{
				proplib::Property &propCount = propCondition.get( "FloatThreshold" );
				string ratioValue = propCount.get( "Value" );
				string opname = propCount.get( "Op" );
				float threshold = propCount.get( "Threshold" );
				long duration = propCount.get( "Duration" );
				
				const float *ratioPtr = fEatStatistics.GetProperty( ratioValue );

				condprop::ThresholdCondition<float,float>::Op op;
				if( opname == "EQ" )
					op = condprop::ThresholdCondition<float,float>::EQ;
				else if( opname == "LT" )
					op = condprop::ThresholdCondition<float,float>::LT;
				else if( opname == "GT" )
					op = condprop::ThresholdCondition<float,float>::GT;
				else
					assert( false );

				condition = new condprop::ThresholdCondition<float,float>( ratioPtr,
																		   op,
																		   threshold,
																		   duration,
																		   value,
																		   couplingRange );
			}
			else
				assert( false );

			conditions->add( condition );
		}

		condprop::Logger<float> *logger = NULL;
		if( (bool)propMinEatVelocity.get( "Record" ) )
		{
			logger = new condprop::ScalarLogger<float>("MinEatVelocity",
													   datalib::FLOAT);
		}

		condprop::Property<float> *property =
			new condprop::Property<float>( "MinEatVelocity",
										   &fMinEatVelocity,
										   &condprop::InterpolateFunction_float,
										   &condprop::DistanceFunction_float,
										   conditions,
										   logger );

		fConditionalProps->add( property );
	}
	{
		proplib::Property &propMaxEatVelocity = doc.get( "MaxEatVelocity" );

		proplib::Property &propConditions = propMaxEatVelocity.get( "Conditions" );
		condprop::ConditionList<float> *conditions = new condprop::ConditionList<float>();
		int nconditions = propConditions.size();

		for( int i = 0; i < nconditions; i++ )
		{
			proplib::Property &propCondition = propConditions.get( i );

			CouplingRange couplingRange;
			{
				string role = propCondition.get( "CouplingRange" );
				if( role == "Begin" )
					couplingRange = COUPLING_RANGE__BEGIN;
				else if( role == "End" )
					couplingRange = COUPLING_RANGE__END;
				else if( role == "None" )
					couplingRange = COUPLING_RANGE__NONE;
				else
					assert( false );
			}

			float value = propCondition.get( "Value" );
			string mode = propCondition.get( "Mode" );

			condprop::Condition<float> *condition;
			if( mode == "Time" )
			{
				long step = propCondition.get( "Time" );
				condition = new condprop::TimeCondition<float>( step, value, couplingRange );
			}
			else if( mode == "IntThreshold" )
			{
				proplib::Property &propCount = propCondition.get( "IntThreshold" );
				string countValue = propCount.get( "Value" );
				string opname = propCount.get( "Op" );
				int threshold = propCount.get( "Threshold" );
				long duration = propCount.get( "Duration" );
				
				const int *countPtr;
				if( countValue == "Agents" )
				{
					countPtr = objectxsortedlist::gXSortedObjects.getCountPtr( AGENTTYPE );
				}
				else
					assert( false );

				condprop::ThresholdCondition<float,int>::Op op;
				if( opname == "EQ" )
					op = condprop::ThresholdCondition<float,int>::EQ;
				else if( opname == "LT" )
					op = condprop::ThresholdCondition<float,int>::LT;
				else if( opname == "GT" )
					op = condprop::ThresholdCondition<float,int>::GT;
				else
					assert( false );

				condition = new condprop::ThresholdCondition<float,int>( countPtr,
																		 op,
																		 threshold,
																		 duration,
																		 value,
																		 couplingRange );
			}
			else if( mode == "FloatThreshold" )
			{
				proplib::Property &propCount = propCondition.get( "FloatThreshold" );
				string ratioValue = propCount.get( "Value" );
				string opname = propCount.get( "Op" );
				float threshold = propCount.get( "Threshold" );
				long duration = propCount.get( "Duration" );
				
				const float *ratioPtr = fEatStatistics.GetProperty( ratioValue );

				condprop::ThresholdCondition<float,float>::Op op;
				if( opname == "EQ" )
					op = condprop::ThresholdCondition<float,float>::EQ;
				else if( opname == "LT" )
					op = condprop::ThresholdCondition<float,float>::LT;
				else if( opname == "GT" )
					op = condprop::ThresholdCondition<float,float>::GT;
				else
					assert( false );

				condition = new condprop::ThresholdCondition<float,float>( ratioPtr,
																		   op,
																		   threshold,
																		   duration,
																		   value,
																		   couplingRange );
			}
			else
				assert( false );

			conditions->add( condition );
		}

		condprop::Logger<float> *logger = NULL;
		if( (bool)propMaxEatVelocity.get( "Record" ) )
		{
			logger = new condprop::ScalarLogger<float>("MaxEatVelocity",
													   datalib::FLOAT);
		}

		condprop::Property<float> *property =
			new condprop::Property<float>( "MaxEatVelocity",
										   &fMaxEatVelocity,
										   &condprop::InterpolateFunction_float,
										   &condprop::DistanceFunction_float,
										   conditions,
										   logger );

		fConditionalProps->add( property );
	}
	{
		proplib::Property &propMaxEatYaw = doc.get( "MaxEatYaw" );

		proplib::Property &propConditions = propMaxEatYaw.get( "Conditions" );
		condprop::ConditionList<float> *conditions = new condprop::ConditionList<float>();
		int nconditions = propConditions.size();

		for( int i = 0; i < nconditions; i++ )
		{
			proplib::Property &propCondition = propConditions.get( i );

			CouplingRange couplingRange;
			{
				string role = propCondition.get( "CouplingRange" );
				if( role == "Begin" )
					couplingRange = COUPLING_RANGE__BEGIN;
				else if( role == "End" )
					couplingRange = COUPLING_RANGE__END;
				else if( role == "None" )
					couplingRange = COUPLING_RANGE__NONE;
				else
					assert( false );
			}

			float value = propCondition.get( "Value" );
			string mode = propCondition.get( "Mode" );

			condprop::Condition<float> *condition;
			if( mode == "Time" )
			{
				long step = propCondition.get( "Time" );
				condition = new condprop::TimeCondition<float>( step, value, couplingRange );
			}
			else if( mode == "IntThreshold" )
			{
				proplib::Property &propCount = propCondition.get( "IntThreshold" );
				string countValue = propCount.get( "Value" );
				string opname = propCount.get( "Op" );
				int threshold = propCount.get( "Threshold" );
				long duration = propCount.get( "Duration" );
				
				const int *countPtr;
				if( countValue == "Agents" )
				{
					countPtr = objectxsortedlist::gXSortedObjects.getCountPtr( AGENTTYPE );
				}
				else
					assert( false );

				condprop::ThresholdCondition<float,int>::Op op;
				if( opname == "EQ" )
					op = condprop::ThresholdCondition<float,int>::EQ;
				else if( opname == "LT" )
					op = condprop::ThresholdCondition<float,int>::LT;
				else if( opname == "GT" )
					op = condprop::ThresholdCondition<float,int>::GT;
				else
					assert( false );

				condition = new condprop::ThresholdCondition<float,int>( countPtr,
																		 op,
																		 threshold,
																		 duration,
																		 value,
																		 couplingRange );
			}
			else if( mode == "FloatThreshold" )
			{
				proplib::Property &propCount = propCondition.get( "FloatThreshold" );
				string ratioValue = propCount.get( "Value" );
				string opname = propCount.get( "Op" );
				float threshold = propCount.get( "Threshold" );
				long duration = propCount.get( "Duration" );
				
				const float *ratioPtr = fEatStatistics.GetProperty( ratioValue );

				condprop::ThresholdCondition<float,float>::Op op;
				if( opname == "EQ" )
					op = condprop::ThresholdCondition<float,float>::EQ;
				else if( opname == "LT" )
					op = condprop::ThresholdCondition<float,float>::LT;
				else if( opname == "GT" )
					op = condprop::ThresholdCondition<float,float>::GT;
				else
					assert( false );

				condition = new condprop::ThresholdCondition<float,float>( ratioPtr,
																		   op,
																		   threshold,
																		   duration,
																		   value,
																		   couplingRange );
			}

			else
				assert( false );

			conditions->add( condition );
		}

		condprop::Logger<float> *logger = NULL;
		if( (bool)propMaxEatYaw.get( "Record" ) )
		{
			logger = new condprop::ScalarLogger<float>("MaxEatYaw",
													   datalib::FLOAT);
		}

		condprop::Property<float> *property =
			new condprop::Property<float>( "MaxEatYaw",
										   &fMaxEatYaw,
										   &condprop::InterpolateFunction_float,
										   &condprop::DistanceFunction_float,
										   conditions,
										   logger );

		fConditionalProps->add( property );
	}
	{
		proplib::Property &propMaxMateVelocity = doc.get( "MaxMateVelocity" );

		proplib::Property &propConditions = propMaxMateVelocity.get( "Conditions" );
		condprop::ConditionList<float> *conditions = new condprop::ConditionList<float>();
		int nconditions = propConditions.size();

		for( int i = 0; i < nconditions; i++ )
		{
			proplib::Property &propCondition = propConditions.get( i );

			CouplingRange couplingRange;
			{
				string role = propCondition.get( "CouplingRange" );
				if( role == "Begin" )
					couplingRange = COUPLING_RANGE__BEGIN;
				else if( role == "End" )
					couplingRange = COUPLING_RANGE__END;
				else if( role == "None" )
					couplingRange = COUPLING_RANGE__NONE;
				else
					assert( false );
			}

			float value = propCondition.get( "Value" );
			string mode = propCondition.get( "Mode" );

			condprop::Condition<float> *condition;
			if( mode == "Time" )
			{
				long step = propCondition.get( "Time" );
				condition = new condprop::TimeCondition<float>( step, value, couplingRange );
			}
			else if( mode == "IntThreshold" )
			{
				proplib::Property &propCount = propCondition.get( "IntThreshold" );
				string countValue = propCount.get( "Value" );
				string opname = propCount.get( "Op" );
				int threshold = propCount.get( "Threshold" );
				long duration = propCount.get( "Duration" );
				
				const int *countPtr;
				if( countValue == "Agents" )
				{
					countPtr = objectxsortedlist::gXSortedObjects.getCountPtr( AGENTTYPE );
				}
				else
					assert( false );

				condprop::ThresholdCondition<float,int>::Op op;
				if( opname == "EQ" )
					op = condprop::ThresholdCondition<float,int>::EQ;
				else if( opname == "LT" )
					op = condprop::ThresholdCondition<float,int>::LT;
				else if( opname == "GT" )
					op = condprop::ThresholdCondition<float,int>::GT;
				else
					assert( false );

				condition = new condprop::ThresholdCondition<float,int>( countPtr,
																		 op,
																		 threshold,
																		 duration,
																		 value,
																		 couplingRange );
			}
			else
				assert( false );

			conditions->add( condition );
		}

		condprop::Logger<float> *logger = NULL;
		if( (bool)propMaxMateVelocity.get( "Record" ) )
		{
			logger = new condprop::ScalarLogger<float>("MaxMateVelocity",
													   datalib::FLOAT);
		}

		condprop::Property<float> *property =
			new condprop::Property<float>( "MaxMateVelocity",
										   &fMaxMateVelocity,
										   &condprop::InterpolateFunction_float,
										   &condprop::DistanceFunction_float,
										   conditions,
										   logger );

		fConditionalProps->add( property );
	}
    genome::gMinStrength = doc.get( "MinAgentStrength" );
    genome::gMaxStrength = doc.get( "MaxAgentStrength" );
    agent::gMinAgentSize = doc.get( "MinAgentSize" );
    agent::gMaxAgentSize = doc.get( "MaxAgentSize" );
    agent::gMinMaxEnergy = doc.get( "MinAgentMaxEnergy" );
    agent::gMaxMaxEnergy = doc.get( "MaxAgentMaxEnergy" );
    genome::gMinmateenergy = doc.get( "MinEnergyFractionToOffspring" );
    genome::gMaxmateenergy = doc.get( "MaxEnergyFractionToOffspring" );
    fMinMateFraction = doc.get( "MinMateEnergyFraction" );
    genome::gMinmaxspeed = doc.get( "MinAgentMaxSpeed" );
    genome::gMaxmaxspeed = doc.get( "MaxAgentMaxSpeed" );
    agent::gSpeed2DPosition = doc.get( "MotionRate" );
    agent::gYaw2DYaw = doc.get( "YawRate" );
	{
		string encoding = doc.get( "YawEncoding" );
		if( encoding == "Oppose" )
			agent::gYawEncoding = agent::YE_OPPOSE;
		else if( encoding == "Squash" )
			agent::gYawEncoding = agent::YE_SQUASH;
		else
			assert( false );
	}
    genome::gMinlrate = doc.get( "MinLearningRate" );
    genome::gMaxlrate = doc.get( "MaxLearningRate" );
    agent::gMinFocus = doc.get( "MinHorizontalFieldOfView" );
    agent::gMaxFocus = doc.get( "MaxHorizontalFieldOfView" );
    agent::gAgentFOV = doc.get( "VerticalFieldOfView" );
    agent::gMaxSizeAdvantage = doc.get( "MaxSizeFightAdvantage" );
	{
		proplib::Property &prop = doc.get( "BodyRedChannel" );
		if( (string)prop == "Fight" )
			agent::gBodyRedChannel = agent::BRC_FIGHT;
		else if( (string)prop == "Give" )
			agent::gBodyRedChannel = agent::BRC_GIVE;
		else
		{
			agent::gBodyRedChannel = agent::BRC_CONST;
			agent::gBodyRedChannelConstValue = (float)prop;
		}
	}
	{
		proplib::Property &prop = doc.get( "BodyGreenChannel" );
		if( (string)prop == "I" )
			agent::gBodyGreenChannel = agent::BGC_ID;
		else if( (string)prop == "L" )
			agent::gBodyGreenChannel = agent::BGC_LIGHT;
		else
		{
			agent::gBodyGreenChannel = agent::BGC_CONST;
			agent::gBodyGreenChannelConstValue = (float)prop;
		}
	}
	{
		proplib::Property &prop = doc.get( "BodyBlueChannel" );
		if( (string)prop == "Mate" )
			agent::gBodyBlueChannel = agent::BBC_MATE;
		else if( (string) prop == "Energy" )
			agent::gBodyBlueChannel = agent::BBC_ENERGY;
		else
		{
			agent::gBodyBlueChannel = agent::BBC_CONST;
			agent::gBodyBlueChannelConstValue = (float)prop;
		}
	}
	{
		proplib::Property &prop = doc.get( "NoseColor" );
		if( (string)prop == "L" )
			agent::gNoseColor = agent::NC_LIGHT;
		else
		{
			agent::gNoseColor = agent::NC_CONST;
			agent::gNoseColorConstValue = (float)prop;
		}
	}
    fPower2Energy = doc.get( "DamageRate" );
    agent::gEnergyUseMultiplier = doc.get( "EnergyUseMultiplier" );
    agent::gEat2Energy = doc.get( "EnergyUseEat" );

	// EnergyUseMate
	{
		string energyUseMateMode = doc.get( "EnergyUseMateMode" );
		if( energyUseMateMode == "Constant" )
		{
			agent::gMate2Energy = doc.get( "EnergyUseMate" );
		}
		else
		{
			proplib::Property &propEnergyUseMate = doc.get( "EnergyUseMateConditional" );

			proplib::Property &propConditions = propEnergyUseMate.get( "Conditions" );
			condprop::ConditionList<float> *conditions = new condprop::ConditionList<float>();
			int nconditions = propConditions.size();

			for( int i = 0; i < nconditions; i++ )
			{
				proplib::Property &propCondition = propConditions.get( i );

				CouplingRange couplingRange;
				{
					string role = propCondition.get( "CouplingRange" );
					if( role == "Begin" )
						couplingRange = COUPLING_RANGE__BEGIN;
					else if( role == "End" )
						couplingRange = COUPLING_RANGE__END;
					else if( role == "None" )
						couplingRange = COUPLING_RANGE__NONE;
					else
						assert( false );
				}

				float value = propCondition.get( "Value" );
				string mode = propCondition.get( "Mode" );

				condprop::Condition<float> *condition;
				if( mode == "Time" )
				{
					long step = propCondition.get( "Time" );
					condition = new condprop::TimeCondition<float>( step, value, couplingRange );
				}
				else if( mode == "IntThreshold" )
				{
					proplib::Property &propCount = propCondition.get( "IntThreshold" );
					string countValue = propCount.get( "Value" );
					string opname = propCount.get( "Op" );
					int threshold = propCount.get( "Threshold" );
					long duration = propCount.get( "Duration" );
				
					const int *countPtr;
					if( countValue == "Agents" )
					{
						countPtr = objectxsortedlist::gXSortedObjects.getCountPtr( AGENTTYPE );
					}
					else
						assert( false );

					condprop::ThresholdCondition<float,int>::Op op;
					if( opname == "EQ" )
						op = condprop::ThresholdCondition<float,int>::EQ;
					else if( opname == "LT" )
						op = condprop::ThresholdCondition<float,int>::LT;
					else if( opname == "GT" )
						op = condprop::ThresholdCondition<float,int>::GT;
					else
						assert( false );

					condition = new condprop::ThresholdCondition<float,int>( countPtr,
																			 op,
																			 threshold,
																			 duration,
																			 value,
																			 couplingRange );
				}
				else
					assert( false );

				conditions->add( condition );
			}

			condprop::Logger<float> *logger = NULL;
			if( (bool)propEnergyUseMate.get( "Record" ) )
			{
				logger = new condprop::ScalarLogger<float>("EnergyUseMate",
														   datalib::FLOAT);
			}

			condprop::Property<float> *property =
				new condprop::Property<float>( "EnergyUseMate",
											   &agent::gMate2Energy,
											   &condprop::InterpolateFunction_float,
											   &condprop::DistanceFunction_float,
											   conditions,
											   logger );

			fConditionalProps->add( property );
		}
	}

    agent::gFight2Energy = doc.get( "EnergyUseFight" );
    agent::gGive2Energy = doc.get( "EnergyUseGive" );
	agent::gMinSizePenalty = doc.get( "MinSizeEnergyPenalty" );
    agent::gMaxSizePenalty = doc.get( "MaxSizeEnergyPenalty" );
    agent::gSpeed2Energy = doc.get( "EnergyUseMove" );
    agent::gYaw2Energy = doc.get( "EnergyUseTurn" );
    agent::gLight2Energy = doc.get( "EnergyUseLight" );
    agent::gFocus2Energy = doc.get( "EnergyUseFocus" );
	agent::gPickup2Energy = doc.get( "EnergyUsePickup" );
	agent::gDrop2Energy = doc.get( "EnergyUseDrop" );
	agent::gCarryAgent2Energy = doc.get( "EnergyUseCarryAgent" );
	agent::gCarryAgentSize2Energy = doc.get( "EnergyUseCarryAgentSize" );
	food::gCarryFood2Energy = doc.get( "EnergyUseCarryFood" );
	brick::gCarryBrick2Energy = doc.get( "EnergyUseCarryBrick" );
    brain::gNeuralValues.maxsynapse2energy = doc.get( "EnergyUseSynapses" );
    agent::gFixedEnergyDrain = doc.get( "EnergyUseFixed" );
    brain::gDecayRate = doc.get( "SynapseWeightDecayRate" );
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
    genome::gMiscBias = doc.get( "MiscegenationFunctionBias" );
    genome::gMiscInvisSlope = doc.get( "MiscegenationFunctionInverseSlope" );
    brain::gLogisticSlope = doc.get( "LogisticSlope" );
    brain::gMaxWeight = doc.get( "MaxSynapseWeight" );

	brain::gEnableInitWeightRngSeed = doc.get( "EnableInitWeightRngSeed" );
	brain::gMinInitWeightRngSeed = doc.get( "MinInitWeightRngSeed" );
	brain::gMaxInitWeightRngSeed = doc.get( "MaxInitWeightRngSeed" );
	RandomNumberGenerator::set( RandomNumberGenerator::INIT_WEIGHT,
								RandomNumberGenerator::LOCAL );
    brain::gInitMaxWeight = doc.get( "MaxSynapseWeightInitial" );
    genome::gMinBitProb = doc.get( "MinInitialBitProb" );
    genome::gMaxBitProb = doc.get( "MaxInitialBitProb" );
	{
		fSolidObjects = 0;
#define __SET( PROP, MASK ) if( (bool)doc.get("Solid"PROP) ) fSolidObjects |= MASK##TYPE
		__SET( "Agents", AGENT );
		__SET( "Food", FOOD );
		__SET( "Bricks", BRICK );
#undef __SET
	}
    agent::gAgentHeight = doc.get( "AgentHeight" );
    food::gFoodHeight = doc.get( "FoodHeight" );
    food::gFoodColor = doc.get( "FoodColor" );
	brick::gBrickHeight = doc.get( "BrickHeight" );
    barrier::gBarrierHeight = doc.get( "BarrierHeight" );
    barrier::gBarrierColor = doc.get( "BarrierColor" );
	barrier::gStickyBarriers = doc.get( "StickyBarriers" );
    fGroundColor = doc.get( "GroundColor" );
    fGroundClearance = doc.get( "GroundClearance" );
    fCameraColor = doc.get( "CameraColor" );
    fCameraRadius = doc.get( "CameraRadius" );
    fCameraHeight = doc.get( "CameraHeight" );
    fCameraRotationRate = doc.get( "CameraRotationRate" );
    fRotateWorld = (fCameraRotationRate != 0.0);	//Boolean for enabling or disabling world roation (CMB 3/19/06)
    fCameraAngleStart = doc.get( "CameraAngleStart" );
    fCameraFOV = doc.get( "CameraFieldOfView" );
    fMonitorAgentRank = doc.get( "MonitorAgentRank" );
    if (!fGraphics)
        fMonitorAgentRank = 0; // cannot monitor agent brain without graphics
    fBrainMonitorStride = doc.get( "BrainMonitorFrequency" );
    globals::worldsize = doc.get( "WorldSize" );
	{
		bool ratioBarrierPositions = doc.get( "RatioBarrierPositions" );
		proplib::Property &propBarriers = doc.get( "Barriers" );

		fRecordBarrierPosition = doc.get( "RecordBarrierPosition" );

		itfor( proplib::PropertyMap, propBarriers.elements(), itBarrier )
		{
			barrier *b = new barrier();

			int id = barrier::gXSortedBarriers.count();
			char barrierName[128];
			sprintf( barrierName, "barrier%d", id );

			ConditionList<LineSegment> *conditions = new ConditionList<LineSegment>();

			LineSegmentLogger *logger = fRecordBarrierPosition
				? new LineSegmentLogger( barrierName )
				: NULL;

			proplib::Property &propKeyFrames = itBarrier->second->get( "KeyFrames" );

			itfor( proplib::PropertyMap, propKeyFrames.elements(), itKeyFrame )
			{
				float x1 = itKeyFrame->second->get( "X1" );
				float z1 = itKeyFrame->second->get( "Z1" );
				float x2 = itKeyFrame->second->get( "X2" );
				float z2 = itKeyFrame->second->get( "Z2" );
				if( ratioBarrierPositions )
				{
					x1 *= globals::worldsize;
					z1 *= globals::worldsize;
					x2 *= globals::worldsize;
					z2 *= globals::worldsize;
				}
				LineSegment position( x1, z1, x2, z2);

				Condition<LineSegment> *condition;

				CouplingRange couplingRange;
				{
					string role = itKeyFrame->second->get( "CouplingRange" );
					if( role == "Begin" )
						couplingRange = COUPLING_RANGE__BEGIN;
					else if( role == "End" )
						couplingRange = COUPLING_RANGE__END;
					else if( role == "None" )
						couplingRange = COUPLING_RANGE__NONE;
					else
						assert( false );
				}

				string mode = itKeyFrame->second->get( "Mode" );
				if( mode == "Time" )
				{
					long t = itKeyFrame->second->get( "Time" );
					condition = new TimeCondition<LineSegment>( t,
																position,
																couplingRange );
				}
				else if( mode == "Count" )
				{
					proplib::Property &propCount = itKeyFrame->second->get( "Count" );
					string value = propCount.get( "Value" );
					string opname = propCount.get( "Op" );
					int threshold = propCount.get( "Threshold" );
					long duration = propCount.get( "Duration" );

					const int *count;
					if( value == "Agents" )
					{
						count = objectxsortedlist::gXSortedObjects.getCountPtr( AGENTTYPE );
					}
					else
					{
						assert( false );
					}

					ThresholdCondition<LineSegment, int>::Op op;
					if( opname == "EQ" )
						op = ThresholdCondition<LineSegment, int>::EQ;
					else if( opname == "LT" )
						op = ThresholdCondition<LineSegment, int>::LT;
					else if( opname == "GT" )
						op = ThresholdCondition<LineSegment, int>::GT;
					else
						assert( false );

					
					condition = new ThresholdCondition<LineSegment, int>( count,
																		  op,
																		  threshold,
																		  duration,
																		  position,
																		  couplingRange );
				}
				else
				{
					assert( false );
				}

				conditions->add( condition );
			}

			Property<LineSegment> *property = 
				new Property<LineSegment>( barrierName,
										   b->getPosition(),
										   InterpolateFunction_LineSegment,
										   DistanceFunction_LineSegment,
										   conditions,
										   logger );

			fConditionalProps->add( property );

			barrier::gXSortedBarriers.add( b );
		}
	}
    fMonitorGeneSeparation = doc.get( "MonitorGeneSeparation" );
    fRecordGeneSeparation = doc.get( "RecordGeneSeparation" );
    fChartBorn = doc.get( "ChartBorn" );
    fChartFitness = doc.get( "ChartFitness" );
    fChartFoodEnergy = doc.get( "ChartFoodEnergy" );
    fChartPopulation = doc.get( "ChartPopulation" );

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

			bool overrideFoodColor = propFoodType.get( "OverrideFoodColor" );
			Color foodColor = overrideFoodColor
				? propFoodType.get( "FoodColor" )
				: doc.get( "FoodColor" );
			EnergyPolarity energyPolarity = propFoodType.get( "EnergyPolarity" );
			Energy depletionThreshold = Energy::createDepletionThreshold( fFoodRemoveEnergy, energyPolarity );

			FoodType::define( name, foodColor, energyPolarity, depletionThreshold );
		}

	}

	// Process AgentMetabolisms
	{
		proplib::Property &propAgentMetabolisms = doc.get( "AgentMetabolisms" );
		int numMetabolisms = propAgentMetabolisms.size();

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

			EnergyMultiplier *eatMultiplier = new EnergyMultiplier();
			{
				proplib::Property &propEatMultiplier = propMetabolism.get( "EatMultiplier" );

				proplib::Property &propConditions = propEatMultiplier.get( "Conditions" );
				condprop::ConditionList<EnergyMultiplier> *conditions = new condprop::ConditionList<EnergyMultiplier>();
				int nconditions = propConditions.size();

				for( int iCondition = 0; iCondition < nconditions; iCondition++ )
				{
					proplib::Property &propCondition = propConditions.get( iCondition );

					CouplingRange couplingRange;
					{
						string role = propCondition.get( "CouplingRange" );
						if( role == "Begin" )
							couplingRange = COUPLING_RANGE__BEGIN;
						else if( role == "End" )
							couplingRange = COUPLING_RANGE__END;
						else if( role == "None" )
							couplingRange = COUPLING_RANGE__NONE;
						else
							assert( false );
					}

					EnergyMultiplier value( propCondition.get( "Value" ) );
					string mode = propCondition.get( "Mode" );

					condprop::Condition<EnergyMultiplier> *condition;
					if( mode == "Time" )
					{
						long step = propCondition.get( "Time" );
						condition = new condprop::TimeCondition<EnergyMultiplier>( step, value, couplingRange );
					}
					else if( mode == "Count" )
					{
						proplib::Property &propCount = propCondition.get( "Count" );
						string countValue = propCount.get( "Value" );
						string opname = propCount.get( "Op" );
						long threshold = propCount.get( "Threshold" );
						long duration = propCount.get( "Duration" );
				
						const long *countPtr;
						if( countValue == "MetabolismAgents" )
						{
							countPtr = &( fNumberAliveWithMetabolism[iMetabolism] );
						}
						else
							assert( false );

						condprop::ThresholdCondition<EnergyMultiplier,long>::Op op;
						if( opname == "EQ" )
							op = condprop::ThresholdCondition<EnergyMultiplier,long>::EQ;
						else if( opname == "LT" )
							op = condprop::ThresholdCondition<EnergyMultiplier,long>::LT;
						else if( opname == "GT" )
							op = condprop::ThresholdCondition<EnergyMultiplier,long>::GT;
						else
							assert( false );

						condition = new condprop::ThresholdCondition<EnergyMultiplier,long>( countPtr,
																							 op,
																							 threshold,
																							 duration,
																							 value,
																							 couplingRange );
					}
					else
						assert( false );

					conditions->add( condition );
				}

				condprop::EnergyMultiplierLogger *logger = NULL;
				if( (bool)propEatMultiplier.get( "Record" ) )
				{
					char buf[128];
					sprintf( buf, "EatMultiplier%d", iMetabolism );
					logger = new condprop::EnergyMultiplierLogger( buf );
				}

				condprop::Property<EnergyMultiplier> *property =
					new condprop::Property<EnergyMultiplier>( "EatMultiplier",
															  eatMultiplier,
															  &condprop::InterpolateFunction_EnergyMultiplier,
															  &condprop::DistanceFunction_EnergyMultiplier,
															  conditions,
															  logger );

				fConditionalProps->add( property );

			}

			float *minEatAge = new float[1];
			{
				proplib::Property &propMinEatAge = propMetabolism.get( "MinEatAge" );

				proplib::Property &propConditions = propMinEatAge.get( "Conditions" );
				condprop::ConditionList<float> *conditions = new condprop::ConditionList<float>();
				int nconditions = propConditions.size();

				for( int iCondition = 0; iCondition < nconditions; iCondition++ )
				{
					proplib::Property &propCondition = propConditions.get( iCondition );

					CouplingRange couplingRange;
					{
						string role = propCondition.get( "CouplingRange" );
						if( role == "Begin" )
							couplingRange = COUPLING_RANGE__BEGIN;
						else if( role == "End" )
							couplingRange = COUPLING_RANGE__END;
						else if( role == "None" )
							couplingRange = COUPLING_RANGE__NONE;
						else
							assert( false );
					}

					float value = propCondition.get( "Value" );
					string mode = propCondition.get( "Mode" );

					condprop::Condition<float> *condition;
					if( mode == "Time" )
					{
						long step = propCondition.get( "Time" );
						condition = new condprop::TimeCondition<float>( step, value, couplingRange );
					}
					else if( mode == "Count" )
					{
						proplib::Property &propCount = propCondition.get( "Count" );
						string countValue = propCount.get( "Value" );
						string opname = propCount.get( "Op" );
						long threshold = propCount.get( "Threshold" );
						long duration = propCount.get( "Duration" );
				
						const long *countPtr;
						if( countValue == "MetabolismAgents" )
						{
							countPtr = &( fNumberAliveWithMetabolism[iMetabolism] );
						}
						else
							assert( false );

						condprop::ThresholdCondition<float,long>::Op op;
						if( opname == "EQ" )
							op = condprop::ThresholdCondition<float,long>::EQ;
						else if( opname == "LT" )
							op = condprop::ThresholdCondition<float,long>::LT;
						else if( opname == "GT" )
							op = condprop::ThresholdCondition<float,long>::GT;
						else
							assert( false );

						condition = new condprop::ThresholdCondition<float,long>( countPtr,
																				  op,
																				  threshold,
																				  duration,
																				  value,
																				  couplingRange );
					}
					else
						assert( false );

					conditions->add( condition );
				}

				condprop::Logger<float> *logger = NULL;
				if( (bool)propMinEatAge.get( "Record" ) )
				{
					char buf[128];
					sprintf( buf, "MinEatAge%d", iMetabolism );
					logger = new condprop::ScalarLogger<float>( buf, datalib::FLOAT );
				}

				condprop::Property<float> *property =
					new condprop::Property<float>( "MinEatAge",
												 minEatAge,
												 &condprop::InterpolateFunction_float,
												 &condprop::DistanceFunction_float,
												 conditions,
												 logger );

				fConditionalProps->add( property );

			}

			Metabolism::define( name, energyPolarity, *eatMultiplier, *minEatAge, carcassFoodType );
		}

		for( int i = 0; i < Metabolism::getNumberOfDefinitions(); i++ )
		{
			fMinNumAgentsWithMetabolism[i] = (int)doc.get("MinAgents") / Metabolism::getNumberOfDefinitions();
		}
	}

	// Process Domains
	{
		proplib::Property &propDomains = doc.get( "Domains" );
		fNumDomains = propDomains.size();
		FoodPatch::MaxPopGroupOnCondition::Group *maxPopGroup_World = NULL;

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

			// Process FoodPatches
			{
				FoodPatch::MaxPopGroupOnCondition::Group *maxPopGroup_Domain = NULL;

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

					foodFraction = propPatch.get( "FoodFraction" );

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
					nhsize = propPatch.get( "NeighborhoodSize" );
					removeFood = propPatch.get( "RemoveFood" );
					if( removeFood )
						numFoodPatchesNeedingRemoval++;

					FoodPatch::OnCondition *onCondition;
					{
						proplib::Property &propOnCondition = propPatch.get( "OnCondition" );
						string mode = propOnCondition.get( "Mode" );

						if( mode == "Time" )
						{
							proplib::Property &propTime = propOnCondition.get( "Time" );

							int period = propTime.get( "Period" );
							float onFraction = propTime.get( "OnFraction" );
							float phase = propTime.get( "Phase" );

							onCondition = new FoodPatch::TimeOnCondition( period,
																		  onFraction,
																		  phase );
						}
						else if( mode == "MaxPopGroup" )
						{

							proplib::Property &propMaxPopGroup = propOnCondition.get( "MaxPopGroup" );
							int maxAgents = propMaxPopGroup.get( "MaxAgents" );
							int timeout = propMaxPopGroup.get( "Timeout" );
							int delay = propMaxPopGroup.get( "Delay" );
							
							FoodPatch::MaxPopGroupOnCondition::Group *group;
							string groupType = propMaxPopGroup.get( "Group" );
							if( groupType == "Domain" )
							{
								if( maxPopGroup_Domain == NULL )
								{
									maxPopGroup_Domain = new FoodPatch::MaxPopGroupOnCondition::Group();
								}
								group = maxPopGroup_Domain;
							}
							else if( groupType == "World" )
							{
								if( maxPopGroup_World == NULL )
								{
									maxPopGroup_World = new FoodPatch::MaxPopGroupOnCondition::Group();
								}
								group = maxPopGroup_World;
							}
							else
							{
								assert( false );
							}

							fCalcFoodPatchAgentCounts = true;

							onCondition = new FoodPatch::MaxPopGroupOnCondition( &(fDomains[id].fFoodPatches[i]),
																				 group,
																				 maxAgents,
																				 timeout,
																				 delay );
						}
						else
						{
							assert( false );
						}
					}

					fDomains[id].fFoodPatches[i].init(foodType, centerX, centerZ, sizeX, sizeZ, foodRate, initFood, minFood, maxFood, maxFoodGrown, foodFraction, shape, distribution, nhsize, onCondition, removeFood, &fStage, &(fDomains[id]), id);

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

				FoodPatch::MaxPopGroupOnCondition::Group::validate( maxPopGroup_Domain );
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

					if( (bool)propPatch.get( "OverrideBrickColor" ) )
					{
						color = propPatch.get( "BrickColor" );
					}
					else
					{
						color = doc.get( "BrickColor" );
					}

					FoodPatch *onSyncFoodPatch = NULL;
					int onSyncFoodPatchIndex = propPatch.get( "OnSyncFoodPatchIndex" );
					if( onSyncFoodPatchIndex > -1 )
					{
						assert( onSyncFoodPatchIndex < fDomains[id].numFoodPatches );
						onSyncFoodPatch = &( fDomains[id].fFoodPatches[ onSyncFoodPatchIndex ] );
					}

					fDomains[id].fBrickPatches[i].init(color, centerX, centerZ, sizeX, sizeZ, brickCount, shape, distribution, nhsize, &fStage, &(fDomains[id]), id, onSyncFoodPatch);

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
	
		FoodPatch::MaxPopGroupOnCondition::Group::validate( maxPopGroup_World );

		for (int id = 0; id < fNumDomains; id++)
		{
			fDomains[id].numAgents = 0;
			fDomains[id].numcreated = 0;
			fDomains[id].numborn = 0;
			fDomains[id].numbornsincecreated = 0;
			fDomains[id].numdied = 0;
			fDomains[id].lastcreate = 0;
			fDomains[id].maxgapcreate = 0;
			fDomains[id].ifit = 0;
			fDomains[id].jfit = 1;
			fDomains[id].fittest = NULL;
		}
	} // Domains
	
	fUseProbabilisticFoodPatches = doc.get( "ProbabilisticFoodPatches" );
    fChartGeneSeparation = doc.get( "ChartGeneSeparation" );
    if (fChartGeneSeparation)
        fMonitorGeneSeparation = true;

	{
		string val = doc.get( "NeuronModel" );
		if( val == "F" )
			brain::gNeuralValues.model = brain::NeuralValues::FIRING_RATE;
		else if( val == "T" )
			brain::gNeuralValues.model = brain::NeuralValues::TAU;
		else if( val == "S" )
			brain::gNeuralValues.model = brain::NeuralValues::SPIKING;
		else
			assert( false );
	}

    brain::gNeuralValues.enableSpikingGenes = doc.get( "EnableSpikingGenes" );

	brain::gNeuralValues.Spiking.aMinVal = doc.get( "SpikingAMin" );
	brain::gNeuralValues.Spiking.aMaxVal = doc.get( "SpikingAMax" );
	brain::gNeuralValues.Spiking.bMinVal = doc.get( "SpikingBMin" );
	brain::gNeuralValues.Spiking.bMaxVal = doc.get( "SpikingBMax" );
	brain::gNeuralValues.Spiking.cMinVal = doc.get( "SpikingCMin" );
	brain::gNeuralValues.Spiking.cMaxVal = doc.get( "SpikingCMax" );
	brain::gNeuralValues.Spiking.dMinVal = doc.get( "SpikingDMin" );
	brain::gNeuralValues.Spiking.dMaxVal = doc.get( "SpikingDMax" );

	brain::gNeuralValues.Tau.minVal = doc.get( "TauMin" );
	brain::gNeuralValues.Tau.maxVal = doc.get( "TauMax" );
	brain::gNeuralValues.Tau.seedVal = doc.get( "TauSeed" );

    brain::gNeuralValues.mininternalneurgroups = doc.get( "MinInternalNeuralGroups" );
    brain::gNeuralValues.maxinternalneurgroups = doc.get( "MaxInternalNeuralGroups" );
    brain::gNeuralValues.mineneurpergroup = doc.get( "MinExcitatoryNeuronsPerGroup" );
    brain::gNeuralValues.maxeneurpergroup = doc.get( "MaxExcitatoryNeuronsPerGroup" );
    brain::gNeuralValues.minineurpergroup = doc.get( "MinInhibitoryNeuronsPerGroup" );
    brain::gNeuralValues.maxineurpergroup = doc.get( "MaxInhibitoryNeuronsPerGroup" );
    brain::gNeuralValues.maxbias = doc.get( "MaxBiasWeight" );
    brain::gNeuralValues.minbiaslrate = doc.get( "MinBiasLrate" );
    brain::gNeuralValues.maxbiaslrate = doc.get( "MaxBiasLrate" );
    brain::gNeuralValues.minconnectiondensity = doc.get( "MinConnectionDensity" );
    brain::gNeuralValues.maxconnectiondensity = doc.get( "MaxConnectionDensity" );
    brain::gNeuralValues.mintopologicaldistortion = doc.get( "MinTopologicalDistortion" );
    brain::gNeuralValues.maxtopologicaldistortion = doc.get( "MaxTopologicalDistortion" );
	brain::gNeuralValues.enableTopologicalDistortionRngSeed = doc.get( "EnableTopologicalDistortionRngSeed" );
	brain::gNeuralValues.minTopologicalDistortionRngSeed = doc.get( "MinTopologicalDistortionRngSeed" );
	brain::gNeuralValues.maxTopologicalDistortionRngSeed = doc.get( "MaxTopologicalDistortionRngSeed" );

	RandomNumberGenerator::set( RandomNumberGenerator::TOPOLOGICAL_DISTORTION,
								RandomNumberGenerator::LOCAL );
	brain::gNeuralValues.maxneuron2energy = doc.get( "EnergyUseNeurons" );
	brain::gNumPrebirthCycles = doc.get( "PreBirthCycles" );

	genome::gSeedFightBias = doc.get( "SeedFightBias" );
	genome::gSeedFightExcitation = doc.get( "SeedFightExcitation" );
	genome::gSeedGiveBias = doc.get( "SeedGiveBias" );
	genome::gSeedPickupBias = doc.get( "SeedPickupBias" );
	genome::gSeedDropBias = doc.get( "SeedDropBias" );
	genome::gSeedPickupExcitation = doc.get( "SeedPickupExcitation" );
	genome::gSeedDropExcitation = doc.get( "SeedDropExcitation" );

    InitNeuralValues();

    fOverHeadRank = doc.get( "OverHeadRank" );
    fAgentTracking = doc.get( "AgentTracking" );
    fMinFoodEnergyAtDeath = doc.get( "MinFoodEnergyAtDeath" );
    genome::gGrayCoding = doc.get( "GrayCoding" );
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
	agent::gNumDepletionSteps = doc.get( "NumDepletionSteps" );
	if( agent::gNumDepletionSteps )
		agent::gMaxPopulationPenaltyFraction = 1.0 / (float) agent::gNumDepletionSteps;

	fApplyLowPopulationAdvantage = doc.get( "ApplyLowPopulationAdvantage" );
	fRecordBirthsDeaths = doc.get( "RecordBirthsDeaths" );
	fRecordPosition = doc.get( "RecordPosition" );
	fRecordContacts = doc.get( "RecordContacts" );
	fRecordCollisions = doc.get( "RecordCollisions" );
	fRecordCarry = doc.get( "RecordCarry" );
	fRecordEnergy = doc.get( "RecordEnergy" );
	fBrainAnatomyRecordAll = doc.get( "BrainAnatomyRecordAll" );
	fBrainFunctionRecordAll = doc.get( "BrainFunctionRecordAll" );
	fBrainAnatomyRecordSeeds = doc.get( "BrainAnatomyRecordSeeds" );
	fBrainFunctionRecordSeeds = doc.get( "BrainFunctionRecordSeeds" );
	fBrainFunctionRecentRecordFrequency = doc.get( "BrainFunctionRecentRecordFrequency" );
	fBestSoFarBrainAnatomyRecordFrequency = doc.get( "BestSoFarBrainAnatomyRecordFrequency" );
	fBestSoFarBrainFunctionRecordFrequency = doc.get( "BestSoFarBrainFunctionRecordFrequency" );
	fBestRecentBrainAnatomyRecordFrequency = doc.get( "BestRecentBrainAnatomyRecordFrequency" );
	fBestRecentBrainFunctionRecordFrequency = doc.get( "BestRecentBrainFunctionRecordFrequency" );
	
	fRecordGeneStats = doc.get( "RecordGeneStats" );
	fRecordPerformanceStats = doc.get( "RecordPerformanceStats" );
	fRecordFoodPatchStats = doc.get( "RecordFoodPatchStats" );
	fCalcFoodPatchAgentCounts |= fRecordFoodPatchStats;
	fRecordComplexity = doc.get( "RecordComplexity" );
	if( fRecordComplexity )
	{
		if( ! fBrainFunctionRecentRecordFrequency )
		{
			if( fBestRecentBrainFunctionRecordFrequency )
				fBrainFunctionRecentRecordFrequency = fBestRecentBrainFunctionRecordFrequency;
			else if( fBestSoFarBrainFunctionRecordFrequency )
				fBrainFunctionRecentRecordFrequency = fBestSoFarBrainFunctionRecordFrequency;
			else if( fBestRecentBrainAnatomyRecordFrequency )
				fBrainFunctionRecentRecordFrequency = fBestRecentBrainAnatomyRecordFrequency;
			else if( fBestSoFarBrainAnatomyRecordFrequency )
				fBrainFunctionRecentRecordFrequency = fBestSoFarBrainAnatomyRecordFrequency;
			else
				fBrainFunctionRecentRecordFrequency = 1000;
			cerr << "Warning: Attempted to record Complexity without recording \"Recent\" brain function.  Setting BrainFunctionRecentRecordFrequency to " << fBrainFunctionRecentRecordFrequency nl;
		}

		// TODO: I don't think we need BestSoFar or BestRecent with the new RecordComplexity behavior - lsy 3 Dec 2011
		// It also ignores the existence of fBrainFunctionRecentRecordFrequency, and probably shouldn't.
		if( ! fBestSoFarBrainFunctionRecordFrequency )
		{
			if( fBestRecentBrainFunctionRecordFrequency )
				fBestSoFarBrainFunctionRecordFrequency = fBestRecentBrainFunctionRecordFrequency;
			else
				fBestSoFarBrainFunctionRecordFrequency = 1000;
			cerr << "Warning: Attempted to record Complexity without recording \"best so far\" brain function.  Setting BestSoFarBrainFunctionRecordFrequency to " << fBestSoFarBrainFunctionRecordFrequency nl;
		}
			
		if( ! fBestSoFarBrainAnatomyRecordFrequency )
		{
			if( fBestRecentBrainAnatomyRecordFrequency )
				fBestSoFarBrainAnatomyRecordFrequency = fBestRecentBrainAnatomyRecordFrequency;
			else
				fBestSoFarBrainAnatomyRecordFrequency = 1000;				
			cerr << "Warning: Attempted to record Complexity without recording \"best so far\" brain anatomy.  Setting BestSoFarBrainAnatomyRecordFrequency to " << fBestSoFarBrainAnatomyRecordFrequency nl;
		}

		if( ! fBestRecentBrainFunctionRecordFrequency )
		{
			if( fBestSoFarBrainAnatomyRecordFrequency )
				fBestRecentBrainFunctionRecordFrequency = fBestSoFarBrainAnatomyRecordFrequency;
			else
				fBestRecentBrainFunctionRecordFrequency = 1000;
			cerr << "Warning: Attempted to record Complexity without recording \"best recent\" brain function.  Setting BestRecentBrainFunctionRecordFrequency to " << fBestRecentBrainFunctionRecordFrequency nl;
		}
			
		if( ! fBestRecentBrainAnatomyRecordFrequency )
		{
			if( fBestSoFarBrainAnatomyRecordFrequency )
				fBestRecentBrainAnatomyRecordFrequency = fBestSoFarBrainAnatomyRecordFrequency;
			else
				fBestRecentBrainAnatomyRecordFrequency = 1000;				
			cerr << "Warning: Attempted to record Complexity without recording \"best recent\" brain anatomy.  Setting BestRecentBrainAnatomyRecordFrequency to " << fBestRecentBrainAnatomyRecordFrequency nl;
		}
	}
		
	fComplexityType = (string)doc.get( "ComplexityType" );
	fComplexityFitnessWeight = doc.get( "ComplexityFitnessWeight" );
	fHeuristicFitnessWeight = doc.get( "HeuristicFitnessWeight" );
	if( fComplexityFitnessWeight != 0.0 )
	{
		if( ! fRecordComplexity )		//Not Recording Complexity?
		{
			cerr << "Warning: Attempted to use Complexity as fitness func without recording Complexity.  Turning on RecordComplexity." nl;
			fRecordComplexity = true;
		}
		if( ! fBrainFunctionRecordAll )	//Not recording BrainFunction?
		{
			cerr << "Warning: Attempted to use Complexity as fitness func without recording brain function.  Turning on RecordBrainFunctionAll." nl;
			fBrainFunctionRecordAll = true;
		}
		if( ! fBrainAnatomyRecordAll )
		{
			cerr << "Warning: Attempted to use Complexity as fitness func without recording brain anatomy.  Turning on RecordBrainAnatomyAll." nl;
			fBrainAnatomyRecordAll = true;				
		}
	}
	fTournamentSize = doc.get( "TournamentSize" );
		
	fRecordGenomes = doc.get( "RecordGenomes" );
	fRecordSeparations = doc.get( "RecordSeparations" );
	fRecordAdamiComplexity = doc.get( "RecordAdamiComplexity" );
	fAdamiComplexityRecordFrequency = doc.get( "AdamiComplexityRecordFrequency" );
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
	
	fRecordMovie = doc.get( "RecordMovie" );

	// If this is a lockstep run, then we need to force certain parameter values (and warn the user)
	if( fLockStepWithBirthsDeathsLog )
	{
		genome::gMinLifeSpan = fMaxSteps;
		genome::gMaxLifeSpan = fMaxSteps;
		
		agent::gEat2Energy = 0.0;
		agent::gMate2Energy = 0.0;
		agent::gFight2Energy = 0.0;
		agent::gMaxSizePenalty = 0.0;
		agent::gSpeed2Energy = 0.0;
		agent::gYaw2Energy = 0.0;
		agent::gLight2Energy = 0.0;
		agent::gFocus2Energy = 0.0;
		agent::gPickup2Energy = 0.0;
		agent::gDrop2Energy = 0.0;
		agent::gCarryAgent2Energy = 0.0;
		agent::gCarryAgentSize2Energy = 0.0;
		food::gCarryFood2Energy = 0.0;
		brick::gCarryBrick2Energy = 0.0;
		agent::gFixedEnergyDrain = 0.0;

		agent::gNumDepletionSteps = 0;
		agent::gMaxPopulationPenaltyFraction = 0.0;
		
		fApplyLowPopulationAdvantage = false;
		
		cout << "Due to running in LockStepWithBirthsDeathsLog mode, the following parameter values have been forcibly reset as indicated:" nl;
		cout << "  MinLifeSpan" ses genome::gMinLifeSpan nl;
		cout << "  MaxLifeSpan" ses genome::gMaxLifeSpan nl;
		cout << "  Eat2Energy" ses agent::gEat2Energy nl;
		cout << "  Mate2Energy" ses agent::gMate2Energy nl;
		cout << "  Fight2Energy" ses agent::gFight2Energy nl;
		cout << "  MaxSizePenalty" ses agent::gMaxSizePenalty nl;
		cout << "  Speed2Energy" ses agent::gSpeed2Energy nl;
		cout << "  Yaw2Energy" ses agent::gYaw2Energy nl;
		cout << "  Light2Energy" ses agent::gLight2Energy nl;
		cout << "  Focus2Energy" ses agent::gFocus2Energy nl;
		cout << "  Pickup2Energy" ses agent::gPickup2Energy nl;
		cout << "  Drop2Energy" ses agent::gDrop2Energy nl;
		cout << "  CarryAgent2Energy" ses agent::gCarryAgent2Energy nl;
		cout << "  CarryAgentSize2Energy" ses agent::gCarryAgentSize2Energy nl;
		cout << "  CarryFood2Energy" ses food::gCarryFood2Energy nl;
		cout << "  CarryBrick2Energy" ses brick::gCarryBrick2Energy nl;
		cout << "  FixedEnergyDrain" ses agent::gFixedEnergyDrain nl;
		cout << "  NumDepletionSteps" ses agent::gNumDepletionSteps nl;
		cout << "  .MaxPopulationPenaltyFraction" ses agent::gMaxPopulationPenaltyFraction nl;
		cout << "  ApplyLowPopulationAdvantage" ses fApplyLowPopulationAdvantage nl;
	}
	
	// If this is a steady-state GA run, then we need to force certain parameter values (and warn the user)
	if( (fHeuristicFitnessWeight != 0.0) || (fComplexityFitnessWeight != 0) )
	{
		fNumberToSeed = lrint( fMaxNumAgents * (float) fNumberToSeed / fInitNumAgents );	// same proportion as originally specified (must calculate before changing fInitNumAgents)
		if( fNumberToSeed > fMaxNumAgents )	// just to be safe
			fNumberToSeed = fMaxNumAgents;
		fInitNumAgents = fMaxNumAgents;	// population starts at maximum
		fMinNumAgents = fMaxNumAgents;		// population stays at mximum
		fProbabilityOfMutatingSeeds = 1.0;	// so there is variation in the initial population
//		fMateThreshold = 1.5;				// so they can't reproduce on their own

		for( int i = 0; i < fNumDomains; i++ )	// over all domains
		{
			fDomains[i].numberToSeed = lrint( fDomains[i].maxNumAgents * (float) fDomains[i].numberToSeed / fDomains[i].initNumAgents );	// same proportion as originally specified (must calculate before changing fInitNumAgents)
			if( fDomains[i].numberToSeed > fDomains[i].maxNumAgents )	// just to be safe
				fDomains[i].numberToSeed = fDomains[i].maxNumAgents;
			fDomains[i].initNumAgents = fDomains[i].maxNumAgents;	// population starts at maximum
			fDomains[i].minNumAgents  = fDomains[i].maxNumAgents;	// population stays at maximum
			fDomains[i].probabilityOfMutatingSeeds = 1.0;				// so there is variation in the initial population
		}

		agent::gNumDepletionSteps = 0;				// turn off the high-population penalty
		agent::gMaxPopulationPenaltyFraction = 0.0;	// ditto
		fApplyLowPopulationAdvantage = false;			// turn off the low-population advantage
		
		cout << "Due to running as a steady-state GA with a fitness function, the following parameter values have been forcibly reset as indicated:" nl;
		cout << "  InitNumAgents" ses fInitNumAgents nl;
		cout << "  MinNumAgents" ses fMinNumAgents nl;
		cout << "  NumberToSeed" ses fNumberToSeed nl;
		cout << "  ProbabilityOfMutatingSeeds" ses fProbabilityOfMutatingSeeds nl;
//		cout << "  MateThreshold" ses fMateThreshold nl;
		for( int i = 0; i < fNumDomains; i++ )
		{
			cout << "  Domain " << i << ":" nl;
			cout << "    initNumAgents" ses fDomains[i].initNumAgents nl;
			cout << "    minNumAgents" ses fDomains[i].minNumAgents nl;
			cout << "    numberToSeed" ses fDomains[i].numberToSeed nl;
			cout << "    probabilityOfMutatingSeeds" ses fDomains[i].probabilityOfMutatingSeeds nl;
		}
		cout << "  NumDepletionSteps" ses agent::gNumDepletionSteps nl;
		cout << "  .MaxPopulationPenaltyFraction" ses agent::gMaxPopulationPenaltyFraction nl;
		cout << "  ApplyLowPopulationAdvantage" ses fApplyLowPopulationAdvantage nl;
		
		if( fComplexityFitnessWeight != 0.0 )
		{
			cout << "Due to running with Complexity as a fitness function, the following parameter values have been forcibly reset as indicated:" nl;
			fRecordComplexity = true;						// record it, since we have to compute it
			cout << "  RecordComplexity" ses fRecordComplexity nl;
		}
	}	
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
    out << fCameraAngle nl;
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

    out << fMonitorAgentRank nl;
    out << fMonitorAgentRankOld nl;
    out << fAgentTracking nl;
    out << fOverHeadRank nl;
    out << fOverHeadRankOld nl;
    out << fMaxGeneSeparation nl;
    out << fMinGeneSeparation nl;
    out << fAverageGeneSeparation nl;
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
					
    out << fNumberFit nl;
    out << fFitI nl;
    out << fFitJ nl;
    for (int index = 0; index < fNumberFit; index++)
    {
		out << fFittest[index]->agentID nl;
        out << fFittest[index]->fitness nl;
        assert(false); // fFittest[index]->genes->dump(out); // port to AbstractFile
    }
	out << fNumberRecentFit nl;
    for (int index = 0; index < fNumberRecentFit; index++)
    {
		out << fRecentFittest[index]->agentID nl;
		out << fRecentFittest[index]->fitness nl;
//		fRecentFittest[index]->genes->Dump(out);	// we don't save the genes in the bestRecent list
    }

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

        int numfitid = 0;
        if (fDomains[id].fittest)
        {
            int i;
            for (i = 0; i < fNumberFit; i++)
            {
                if (fDomains[id].fittest[i])
                    numfitid++;
                else
                    break;
            }
            out << numfitid nl;

            for (i = 0; i < numfitid; i++)
            {
				out << fDomains[id].fittest[i]->agentID nl;
                out << fDomains[id].fittest[i]->fitness nl;
                assert(false); // fDomains[id].fittest[i]->genes->dump(out); // port to AbstractFile
            }
        }
        else
            out << numfitid nl;
    }
	
	// Dump monitor windows
	if (fBirthrateWindow != NULL)
		fBirthrateWindow->Dump(out);

	if (fFitnessWindow != NULL)
		fFitnessWindow->Dump(out);

	if (fFoodEnergyWindow != NULL)
		fFoodEnergyWindow->Dump(out);

	if (fPopulationWindow != NULL)
		fPopulationWindow->Dump(out);

	if (fGeneSeparationWindow != NULL)
		fGeneSeparationWindow->Dump(out);
		

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
// TSimulation::PopulateStatusList
//---------------------------------------------------------------------------
void TSimulation::PopulateStatusList(TStatusList& list)
{
	char t[256];
	char t2[256];
	short id;
	
	// TODO: If we're neither updating the window, nor writing to the stat file,
	// then we shouldn't sprintf all these strings, or put them in the list
	// (but for now, the window always draws anyway, so it's not a big deal)
	
	sprintf( t, "step = %ld", fStep );
	list.push_back( strdup( t ) );
	
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
	list.push_back( strdup( t ) );
	if( Metabolism::getNumberOfDefinitions() > 1 )
	{
		for( int i = 0; i < Metabolism::getNumberOfDefinitions(); i++ )
		{
			sprintf( t, " -%s = %4ld", Metabolism::get(i)->name.c_str(), fNumberAliveWithMetabolism[i] );
			list.push_back( strdup( t ) );
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
	list.push_back( strdup( t ) );

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
	list.push_back( strdup( t ) );

	sprintf( t, " -random = %4ld", fNumberCreatedRandom );
	list.push_back( strdup( t ) );

	sprintf( t, " -two    = %4ld", fNumberCreated2Fit );
	list.push_back( strdup( t ) );

	sprintf( t, " -one    = %4ld", fNumberCreated1Fit );
	list.push_back( strdup( t ) );

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
	list.push_back( strdup( t ) );
	
	if( (fHeuristicFitnessWeight != 0.0) || (fComplexityFitnessWeight != 0.0) )
	{
		sprintf( t, "born_v  = %4ld", fNumberBornVirtual );
		list.push_back( strdup( t ) );
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
	list.push_back( strdup( t ) );


	sprintf( t, " -age    = %4ld", fNumberDiedAge );
	list.push_back( strdup( t ) );
	
	sprintf( t, " -energy = %4ld", fNumberDiedEnergy );
	list.push_back( strdup( t ) );
	
	sprintf( t, " -fight  = %4ld", fNumberDiedFight );
	list.push_back( strdup( t ) );
	
	sprintf( t, " -eat  = %4ld", fNumberDiedEat );
	list.push_back( strdup( t ) );
	
	sprintf( t, " -edge   = %4ld", fNumberDiedEdge );
	list.push_back( strdup( t ) );

	sprintf( t, " -smite  = %4ld", fNumberDiedSmite );
	list.push_back( strdup( t ) );

	sprintf( t, " -patch  = %4ld", fNumberDiedPatch );
	list.push_back( strdup( t ) );

    sprintf( t, "birthDenials = %ld", fBirthDenials );
	list.push_back( strdup( t ) );

    sprintf( t, "miscDenials = %ld", fMiscDenials );
	list.push_back( strdup( t ) );

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
	list.push_back( strdup( t ) );

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
	list.push_back( strdup( t ) );

	if( (fHeuristicFitnessWeight != 0.0) || (fComplexityFitnessWeight != 0.0) )
		sprintf( t, "born_v/(c+bv) = %.2f", float(fNumberBornVirtual) / float(fNumberCreated + fNumberBornVirtual) );
	else
		sprintf( t, "born/total = %.2f", float(fNumberBorn) / float(fNumberCreated + fNumberBorn) );
	list.push_back( strdup( t ) );

	sprintf( t, "Fitness m=%.2f, c=%.2f, a=%.2f", fMaxFitness, fCurrentMaxFitness[0] / fTotalHeuristicFitness, fAverageFitness );
	list.push_back( strdup( t ) );
	
//	sprintf( t, "NormFit m=%.2f, c=%.2f, a=%.2f", fMaxFitness / fTotalHeuristicFitness, fCurrentMaxFitness[0] / fTotalHeuristicFitness, fAverageFitness / fTotalHeuristicFitness );
//	list.push_back( strdup( t ) );
	
	sprintf( t, "Fittest =" );
	int fittestCount = min( 5, min( fNumberFit, (int) fNumberDied ));
	for( int i = 0; i < fittestCount; i++ )
	{
		sprintf( t2, " %lu", fFittest[i]->agentID );
		strcat( t, t2 );
	}
	list.push_back( strdup( t ) );
	
	if( fittestCount > 0 )
	{
		sprintf( t, " " );
		for( int i = 0; i < fittestCount; i++ )
		{
			sprintf( t2, "  %.2f", fFittest[i]->fitness );
			strcat( t, t2 );
		}
		list.push_back( strdup( t ) );
	}
	
	sprintf( t, "CurFit =" );
	for( int i = 0; i < fCurrentFittestCount; i++ )
	{
		sprintf( t2, " %lu", fCurrentFittestAgent[i]->Number() );
		strcat( t, t2 );
	}
	list.push_back( strdup( t ) );
	
	if( fCurrentFittestCount > 0 )
	{
		sprintf( t, " " );
		for( int i = 0; i < fCurrentFittestCount; i++ )
		{
			sprintf( t2, "  %.2f", fCurrentFittestAgent[i]->HeuristicFitness() / fTotalHeuristicFitness );
			strcat( t, t2 );
		}
		list.push_back( strdup( t ) );
	}
	
	sprintf( t, "avgFoodEnergy = %.2f", (fAverageFoodEnergyIn - fAverageFoodEnergyOut) / (fAverageFoodEnergyIn + fAverageFoodEnergyOut) );
	list.push_back( strdup( t ) );
	
	sprintf( t, "totFoodEnergy = %.2f", (fTotalFoodEnergyIn - fTotalFoodEnergyOut) / (fTotalFoodEnergyIn + fTotalFoodEnergyOut) );
	list.push_back( strdup( t ) );
	
	sprintf( t, "totEnergyEaten = %.1f", fTotalEnergyEaten[0] );
	for( int i = 1; i < globals::numEnergyTypes; i++ )
	{
		sprintf( t2, ", %.1f", fTotalEnergyEaten[i] );
		strcat( t, t2 );
	}
	list.push_back( strdup( t ) );
	
	static Energy lastTotalEnergyEaten;
	static Energy deltaEnergy;
    if( !(fStep % fStatusFrequency) )
    {
		deltaEnergy = fTotalEnergyEaten - lastTotalEnergyEaten;
		lastTotalEnergyEaten = fTotalEnergyEaten;
	}
	sprintf( t, "EatRate = %.1f", deltaEnergy[0] / fStatusFrequency );
	for( int i = 1; i < globals::numEnergyTypes; i++ )
	{
		sprintf( t2, ", %.1f", deltaEnergy[i] / fStatusFrequency );
		strcat( t, t2 );
	}
	list.push_back( strdup( t ) );
	
	static long lastNumberBorn = 0;
	static long deltaBorn;
	long numberBorn;
	if( (fComplexityFitnessWeight == 0.0) && (fHeuristicFitnessWeight == 0.0) )
		numberBorn = fNumberBorn;
	else
		numberBorn = fNumberBornVirtual;
    if( !(fStep % fStatusFrequency) )
    {
		deltaBorn = numberBorn - lastNumberBorn;
		lastNumberBorn = numberBorn;
	}
	sprintf( t, "MateRate = %.2f", (double) deltaBorn / fStatusFrequency );
	list.push_back( strdup( t ) );
	
	if (fMonitorGeneSeparation)
	{
		sprintf( t, "GeneSep = %5.3f [%5.3f, %5.3f]", fAverageGeneSeparation, fMinGeneSeparation, fMaxGeneSeparation );
		list.push_back( strdup( t ) );
	}

	sprintf( t, "LifeSpan = %lu ± %lu [%lu, %lu]", nint( fLifeSpanStats.mean() ), nint( fLifeSpanStats.stddev() ), (ulong) fLifeSpanStats.min(), (ulong) fLifeSpanStats.max() );
	list.push_back( strdup( t ) );

	sprintf( t, "RecLifeSpan = %lu ± %lu [%lu, %lu]", nint( fLifeSpanRecentStats.mean() ), nint( fLifeSpanRecentStats.stddev() ), (ulong) fLifeSpanRecentStats.min(), (ulong) fLifeSpanRecentStats.max() );
	list.push_back( strdup( t ) );

	sprintf( t, "CurNeurons = %.1f ± %.1f [%lu, %lu]", fCurrentNeuronCountStats.mean(), fCurrentNeuronCountStats.stddev(), (ulong) fCurrentNeuronCountStats.min(), (ulong) fCurrentNeuronCountStats.max() );
	list.push_back( strdup( t ) );

	sprintf( t, "NeurGroups = %.1f ± %.1f [%lu, %lu]", fNeuronGroupCountStats.mean(), fNeuronGroupCountStats.stddev(), (ulong) fNeuronGroupCountStats.min(), (ulong) fNeuronGroupCountStats.max() );
	list.push_back( strdup( t ) );

	sprintf( t, "CurNeurGroups = %.1f ± %.1f [%lu, %lu]", fCurrentNeuronGroupCountStats.mean(), fCurrentNeuronGroupCountStats.stddev(), (ulong) fCurrentNeuronGroupCountStats.min(), (ulong) fCurrentNeuronGroupCountStats.max() );
	list.push_back( strdup( t ) );

	sprintf( t, "CurSynapses = %.1f ± %.1f [%lu, %lu]", fCurrentSynapseCountStats.mean(), fCurrentSynapseCountStats.stddev(), (ulong) fCurrentSynapseCountStats.min(), (ulong) fCurrentSynapseCountStats.max() );
	list.push_back( strdup( t ) );

	sprintf( t, "Rate %2.1f (%2.1f) %2.1f (%2.1f) %2.1f (%2.1f)",
			 fFramesPerSecondInstantaneous, fSecondsPerFrameInstantaneous,
			 fFramesPerSecondRecent,        fSecondsPerFrameRecent,
			 fFramesPerSecondOverall,       fSecondsPerFrameOverall  );
	list.push_back( strdup( t ) );
	
	if( fRecordFoodPatchStats )
	{
		int numAgentsInAnyFoodPatchInAnyDomain = 0;
		int numAgentsInOuterRangesInAnyDomain = 0;

		for( int domainNumber = 0; domainNumber < fNumDomains; domainNumber++ )
		{
			sprintf( t, "Domain %d", domainNumber);
			list.push_back( strdup( t ) );
			
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
				list.push_back( strdup( t ) );
			}
			
			
			sprintf( t, "  FP* %3d %3d  %4.1f %4.1f 100.0",
					 numAgentsInAnyFoodPatch,
					 numAgentsInAnyFoodPatch + numAgentsInOuterRanges,
					 numAgentsInAnyFoodPatch * makePercent,
					(numAgentsInAnyFoodPatch + numAgentsInOuterRanges) * makePercent );
			list.push_back( strdup( t ) );

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
			list.push_back( strdup( t ) );
		}
	}
	
    if( !(fStep % fStatusFrequency) || (fStep == 1) )
    {
		char statusFileName[256];
		
		sprintf( statusFileName, "run/stats/stat.%ld", fStep );
        FILE *statusFile = fopen( statusFileName, "w" );
		Q_CHECK_PTR( statusFile );
		
		if( fStatusToStdout )
			printf( "------------------------------------------------------------\n" );
		
		TStatusList::const_iterator iter = list.begin();
		for( ; iter != list.end(); ++iter )
		{
			bool record = true;
			if( ! fRecordPerformanceStats && (0 == strncmp( *iter, "Rate", 4 )) )
				record = false;
			if( record )
				fprintf( statusFile, "%s\n", *iter );
			if( fStatusToStdout )
				printf( "%s\n", *iter );
		}

        fclose( statusFile );
    }
}


//---------------------------------------------------------------------------
// TSimulation::Update
//
// Do not call this function as part of the simulation.  It is only used to
// refresh all windows after something external, like a screen saver,
// overwrote all the windows.
//---------------------------------------------------------------------------
void TSimulation::Update()
{
	// Redraw main simulation window
	fSceneView->updateGL();

	// Update agent POV window
	if (fShowVision && fAgentPOVWindow != NULL && !fAgentPOVWindow->isHidden())
		fAgentPOVWindow->updateGL();
		//QApplication::postEvent(fAgentPOVWindow, new QCustomEvent(kUpdateEventType));	
	
	// Update chart windows
	if (fChartBorn && fBirthrateWindow != NULL && fBirthrateWindow->isVisible())
		fBirthrateWindow->updateGL();
		//QApplication::postEvent(fBirthrateWindow, new QCustomEvent(kUpdateEventType));	
			
	if (fChartFitness && fFitnessWindow != NULL && fFitnessWindow->isVisible())
		fFitnessWindow->updateGL();
		//QApplication::postEvent(fFitnessWindow, new QCustomEvent(kUpdateEventType));	
			
	if (fChartFoodEnergy && fFoodEnergyWindow != NULL && fFoodEnergyWindow->isVisible())
		fFoodEnergyWindow->updateGL();
		//QApplication::postEvent(fFoodEnergyWindow, new QCustomEvent(kUpdateEventType));	
	
	if (fChartPopulation && fPopulationWindow != NULL && fPopulationWindow->isVisible())
		fPopulationWindow->updateGL();
		//QApplication::postEvent(fPopulationWindow, new QCustomEvent(kUpdateEventType));	

	if (fBrainMonitorWindow != NULL && fBrainMonitorWindow->isVisible())
		fBrainMonitorWindow->updateGL();
		//QApplication::postEvent(fBrainMonitorWindow, new QCustomEvent(kUpdateEventType));	

	if (fChartGeneSeparation && fGeneSeparationWindow != NULL)
		fGeneSeparationWindow->updateGL();
		//QApplication::postEvent(fGeneSeparationWindow, new QCustomEvent(kUpdateEventType));	
		
	if (fShowTextStatus && fTextStatusWindow != NULL)
		fTextStatusWindow->Draw();
		//QApplication::postEvent(fTextStatusWindow, new QCustomEvent(kUpdateEventType));	
}


int TSimulation::getRandomPatch( int domainNumber )
{
	int patch;
	float ranval;
	float maxFractions = 0.0;
	
	// Since not all patches may be "on", we need to calculate the maximum fraction
	// attainable by those patches that are on, and therefore allowed to grow food
	for( short i = 0; i < fDomains[domainNumber].numFoodPatches; i++ )
		if( fDomains[domainNumber].fFoodPatches[i].on( fStep ) )
			maxFractions += fDomains[domainNumber].fFoodPatches[i].fraction;
	
	if( maxFractions > 0.0 )	// there is an active patch in this domain
	{
		float sumFractions = 0.0;
		
		// Weight the random value by the maximum attainable fraction, so we always get
		// a valid patch selection (if possible--they could all be off)
		ranval = randpw() * maxFractions;

		for( short i = 0; i < fDomains[domainNumber].numFoodPatches; i++ )
		{
			if( fDomains[domainNumber].fFoodPatches[i].on( fStep ) )
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

#if 0
/********************************************************************************
 ********************************************************************************
 ***
 *** BEGIN LEGACY WORLDFILE CODE
 ***
 ********************************************************************************
 ********************************************************************************/

//-------------------------------------------------------------------------------------------
// TSimulation::ReadWorldFile
//-------------------------------------------------------------------------------------------
void TSimulation::ReadWorldFile(const char* filename)
{

#define __PROP(LABEL,VAR)in >> VAR; ReadLabel(in, LABEL); cout << LABEL ses VAR nl;
#define PROP(NAME) __PROP(#NAME,f##NAME)
#define __VPROP(LABEL,VAR,VERSION,DEFAULT) \
	if( version >= VERSION ) \
	{						 \
		__PROP(LABEL,VAR);	 \
	}						 \
	else					 \
	{						 \
		VAR = DEFAULT;		 \
	}
#define VPROP(NAME,VERSION,DEFAULT) __VPROP(#NAME,f##NAME,VERSION,DEFAULT)
		

    filebuf fb;
	if( !Resources::openWorldFile( &fb, filename ) )
	{
		return;
	}
	
	istream in( &fb );
    short version;
    char label[128];

    in >> version; in >> label;
	cout << "version" ses version nl;
	
    if( (version < 1) || (version > CurrentWorldfileVersion) )
    {
		if( version <  1 )
		{
			eprintf( "Invalid worldfile version (%d) < 1\n", version );
		}
		else
		{
			eprintf( "Invalid worldfile version (%d) > CurrentWorldfileVersion (%d)\n", version, CurrentWorldfileVersion );
		}
		cout nlf;
     	fb.close();
        exit( 1 );
	}
	
	if( version >= 25 )
	{
		in >> fLockStepWithBirthsDeathsLog; in >> label;
		cout << "LockStepWithBirthsDeathsLog" ses fLockStepWithBirthsDeathsLog nl;
	}
	else
	{
		fLockStepWithBirthsDeathsLog = false;
		cout << "+LockStepWithBirthsDeathsLog" ses fLockStepWithBirthsDeathsLog nl;
	}
	
	if( version >= 18 )
	{
		in >> fMaxSteps; in >> label;
		cout << "maxSteps" ses fMaxSteps nl;
	}
	else
		fMaxSteps = 0;  // don't terminate automatically

	VPROP( EndOnPopulationCrash, 48, false );
	
	bool ignoreBool;
    in >> ignoreBool; in >> label;
    cout << "fDoCPUWork (ignored)" ses ignoreBool nl;

    in >> fDumpFrequency; in >> label;
    cout << "fDumpFrequency" ses fDumpFrequency nl;
    in >> fStatusFrequency; in >> label;
    cout << "fStatusFrequency" ses fStatusFrequency nl;
    in >> globals::blockedEdges; in >> label;
    cout << "edges" ses globals::blockedEdges nl;

	// TODO figure out how to config agent using this
    in >> globals::wraparound; in >> label;
    cout << "wraparound" ses globals::wraparound nl;

    in >> agent::gVision; in >> label;
    if (!fGraphics)
        agent::gVision = false;
    cout << "vision" ses agent::gVision nl;
    in >> fShowVision; in >> label;
    if (!fGraphics)
        fShowVision = false;
    cout << "showvision" ses fShowVision nl;

	VPROP( StaticTimestepGeometry, 34, false );
	VPROP( ParallelInitAgents, 55, false );
	VPROP( ParallelInteract, 55, false );
	VPROP( ParallelBrains, 55, true );

    in >> brain::gMinWin; in >> label;
    cout << "minwin" ses brain::gMinWin nl;
    in >> agent::gMaxVelocity; in >> label;
    cout << "maxvel" ses agent::gMaxVelocity nl;

    in >> fMinNumAgents; in >> label;
    cout << "minNumAgents" ses fMinNumAgents nl;
    in >> fMaxNumAgents; in >> label;
    cout << "maxNumAgents" ses fMaxNumAgents nl;
    in >> fInitNumAgents; in >> label;
    cout << "initNumAgents" ses fInitNumAgents nl;
	if( version >= 9 )
	{
		in >> fNumberToSeed; in >> label;
		cout << "numberToSeed" ses fNumberToSeed nl;
		in >> fProbabilityOfMutatingSeeds; in >> label;
		cout << "probabilityOfMutatingSeeds" ses fProbabilityOfMutatingSeeds nl;
	}
	else
	{
		fNumberToSeed = 0;
		fProbabilityOfMutatingSeeds = 0.0;
	}

	VPROP( SeedFromFile, 45, false );
	VPROP( PositionSeedsFromFile, 49, false );

    in >> fMiscAgents; in >> label;
    cout << "miscagents" ses fMiscAgents nl;

	if( version >= 32 )
	{
		in >> fInitFoodCount; in >> label;
		cout << "initFoodCount" ses fInitFoodCount nl;
		in >> fMinFoodCount; in >> label;
		cout << "minFoodCount" ses fMinFoodCount nl;
		in >> fMaxFoodCount; in >> label;
		cout << "maxFoodCount" ses fMaxFoodCount nl;
		in >> fMaxFoodGrownCount; in >> label;
		cout << "maxFoodGrown" ses fMaxFoodGrownCount nl;
		in >> fFoodRate; in >> label;
		cout << "foodrate" ses fFoodRate nl;
	}

	VPROP( FoodRemoveEnergy, 46, 0.0f );

    in >> fPositionSeed; in >> label;
    cout << "positionSeed" ses fPositionSeed nl;
    in >> fGenomeSeed; in >> label;
    cout << "genomeSeed" ses fGenomeSeed nl;


	VPROP( SimulationSeed, 44, 0 );

	if( version < 32 )
	{
		in >> fMinFoodCount; in >> label;
		cout << "minFoodCount" ses fMinFoodCount nl;
		in >> fMaxFoodCount; in >> label;
		cout << "maxFoodCount" ses fMaxFoodCount nl;
		in >> fMaxFoodGrownCount; in >> label;
		cout << "maxFoodGrown" ses fMaxFoodGrownCount nl;
		in >> fInitFoodCount; in >> label;
		cout << "initFoodCount" ses fInitFoodCount nl;
		in >> fFoodRate; in >> label;
		cout << "foodrate" ses fFoodRate nl;
	}

	if( version < 51 )
	{
		bool rfood;

		in >> rfood; in >> label;
		fAgentsRfood = rfood ? RFOOD_TRUE : RFOOD_FALSE;
		cout << "agentsRfood" ses rfood nl;
	}
	else
	{
		string rfood;

		__PROP( "agentsRfood", rfood );
		if( rfood == "F" )
		{
			fAgentsRfood = RFOOD_TRUE__FIGHT_ONLY;
		}
		else if( rfood == "1" )
		{
			fAgentsRfood = RFOOD_TRUE;
		}
		else if( rfood == "0" )
		{
			fAgentsRfood = RFOOD_FALSE;
		}
		else
		{
			cerr << "Invalid agentsRfood value. Should be in {0,1,F}" << endl;
			exit( 1 );
		}
	}
    in >> fFitness1Frequency; in >> label;
    cout << "fit1freq" ses fFitness1Frequency nl;
    in >> fFitness2Frequency; in >> label;
    cout << "fit2freq" ses fFitness2Frequency nl;
    in >> fNumberFit; in >> label;
    cout << "numfit" ses fNumberFit nl;
	
	if( version >= 14 )
	{
		in >> fNumberRecentFit; in >> label;
		cout << "numRecentFit" ses fNumberRecentFit nl;
	}
	
    in >> fEatFitnessParameter; in >> label;
    cout << "eatfitparam" ses fEatFitnessParameter nl;
    in >> fMateFitnessParameter; in >> label;
    cout << "matefitparam" ses fMateFitnessParameter nl;
    in >> fMoveFitnessParameter; in >> label;
    cout << "movefitparam" ses fMoveFitnessParameter nl;
    in >> fEnergyFitnessParameter; in >> label;
    cout << "energyfitparam" ses fEnergyFitnessParameter nl;
    in >> fAgeFitnessParameter; in >> label;
    cout << "agefitparam" ses fAgeFitnessParameter nl;
  	fTotalHeuristicFitness = fEatFitnessParameter + fMateFitnessParameter + fMoveFitnessParameter + fEnergyFitnessParameter + fAgeFitnessParameter;
    in >> food::gMinFoodEnergy; in >> label;
    cout << "minfoodenergy" ses food::gMinFoodEnergy nl;
    in >> food::gMaxFoodEnergy; in >> label;
    cout << "maxfoodenergy" ses food::gMaxFoodEnergy nl;
    in >> food::gSize2Energy; in >> label;
    cout << "size2energy" ses food::gSize2Energy nl;
    in >> fEat2Consume; in >> label;
    cout << "eat2consume" ses fEat2Consume nl;

	if( version >= 53 )
	{
		string layout;

		__PROP( "genomeLayout", layout );

		if( layout == "L" )
		{
			genome::gLayoutType = genome::GenomeLayout::LEGACY;
		}
		else if( layout == "N" )
		{
			genome::gLayoutType = genome::GenomeLayout::NEURGROUP;
		}
		else
		{
			cerr << "bad value for genomeLayout." << endl;
			exit( 1 );
		}
	}
	else
	{
		genome::gLayoutType = genome::GenomeLayout::LEGACY;
	}

	if( version >= 48 )
	{
		// For earlier versions these will be initialized in initworld()
		__PROP( "enableMateWaitFeedback", genome::gEnableMateWaitFeedback );
	}

	if( version >= 38 )
	{
		// For earlier versions these will be initialized in initworld()
		__PROP( "enableSpeedFeedback", genome::gEnableSpeedFeedback );
		__PROP( "enableGive", genome::gEnableGive );
	}
	
	if( version >= 39 )
	{
		__PROP( "enableCarry", genome::gEnableCarry );
		__PROP( "maxCarries", agent::gMaxCarries );
	}

	if( version >= 41 )
	{
		int agents, food, bricks;

		fCarryObjects = 0;

		__PROP( "carryAgents", agents );
		__PROP( "carryFood", food );
		__PROP( "carryBricks", bricks );

#define __SET( VAR, MASK ) 	if( VAR ) fCarryObjects |= MASK##TYPE
		__SET( agents, AGENT );
		__SET( food, FOOD );
		__SET( bricks, BRICK );
#undef __SET

		fShieldObjects = 0;

		__PROP( "shieldAgents", agents );
		__PROP( "shieldFood", food );
		__PROP( "shieldBricks", bricks );

#define __SET( VAR, MASK ) 	if( VAR ) fShieldObjects |= MASK##TYPE
		__SET( agents, AGENT );
		__SET( food, FOOD );
		__SET( bricks, BRICK );
#undef __SET

		PROP( CarryPreventsEat );
		PROP( CarryPreventsFight );
		PROP( CarryPreventsGive );
		PROP( CarryPreventsMate );
	}
	else
	{
		fCarryObjects = ANYTYPE;
		fShieldObjects = 0;
		fCarryPreventsEat = 0;
		fCarryPreventsFight = 0;
		fCarryPreventsGive = 0;
		fCarryPreventsMate = 0;
	}

	
    int ignoreShort1, ignoreShort2, ignoreShort3, ignoreShort4;
    in >> ignoreShort1; in >> label;
    cout << "minintneurons" ses ignoreShort1 nl;
    in >> ignoreShort2; in >> label;
    cout << "maxintneurons" ses ignoreShort2 nl;
    // note: minintneurons & maxintneurons are no longer used
    cout << "note: minintneurons & maxintneurons are no longer used" nl;

    in >> genome::gMinvispixels; in >> label;
    cout << "minvispixels" ses genome::gMinvispixels nl;
    in >> genome::gMaxvispixels; in >> label;
    cout << "maxvispixels" ses genome::gMaxvispixels nl;
    in >> genome::gMinMutationRate; in >> label;
    cout << "minmrate" ses genome::gMinMutationRate nl;
    in >> genome::gMaxMutationRate; in >> label;
    cout << "maxmrate" ses genome::gMaxMutationRate nl;
    in >> genome::gMinNumCpts; in >> label;
    cout << "minNumCpts" ses genome::gMinNumCpts nl;
    in >> genome::gMaxNumCpts; in >> label;
    cout << "maxNumCpts" ses genome::gMaxNumCpts nl;
    in >> genome::gMinLifeSpan; in >> label;
    cout << "minlifespan" ses genome::gMinLifeSpan nl;
    in >> genome::gMaxLifeSpan; in >> label;
    cout << "maxlifespan" ses genome::gMaxLifeSpan nl;
    in >> fMateWait; in >> label;
    cout << "matewait" ses fMateWait nl;
    in >> agent::gInitMateWait; in >> label;
    cout << "initmatewait" ses agent::gInitMateWait nl;
	if( version >= 29 )
	{
		in >> fEatMateSpan; in >> label;
		cout << "EatMateSpan" ses fEatMateSpan nl;
	}
	else
	{
		fEatMateSpan = 0;
		cout << "+EatMateSpan" ses fEatMateSpan nl;
	}
    in >> genome::gMinStrength; in >> label;
    cout << "minstrength" ses genome::gMinStrength nl;
    in >> genome::gMaxStrength; in >> label;
    cout << "maxstrength" ses genome::gMaxStrength nl;
    in >> agent::gMinAgentSize; in >> label;
    cout << "minAgentSize" ses agent::gMinAgentSize nl;
    in >> agent::gMaxAgentSize; in >> label;
    cout << "maxAgentSize" ses agent::gMaxAgentSize nl;
    in >> agent::gMinMaxEnergy; in >> label;
    cout << "minmaxenergy" ses agent::gMinMaxEnergy nl;
    in >> agent::gMaxMaxEnergy; in >> label;
    cout << "maxmaxenergy" ses agent::gMaxMaxEnergy nl;
    in >> genome::gMinmateenergy; in >> label;
    cout << "minmateenergy" ses genome::gMinmateenergy nl;
    in >> genome::gMaxmateenergy; in >> label;
    cout << "maxmateenergy" ses genome::gMaxmateenergy nl;
    in >> fMinMateFraction; in >> label;
    cout << "minmatefrac" ses fMinMateFraction nl;
    in >> genome::gMinmaxspeed; in >> label;
    cout << "minmaxspeed" ses genome::gMinmaxspeed nl;
    in >> genome::gMaxmaxspeed; in >> label;
    cout << "maxmaxspeed" ses genome::gMaxmaxspeed nl;
    in >> agent::gSpeed2DPosition; in >> label;
    cout << "speed2dpos" ses agent::gSpeed2DPosition nl;
    in >> agent::gYaw2DYaw; in >> label;
    cout << "yaw2dyaw" ses agent::gYaw2DYaw nl;
    in >> genome::gMinlrate; in >> label;
    cout << "minlrate" ses genome::gMinlrate nl;
    in >> genome::gMaxlrate; in >> label;
    cout << "maxlrate" ses genome::gMaxlrate nl;
    in >> agent::gMinFocus; in >> label;
    cout << "minfocus" ses agent::gMinFocus nl;
    in >> agent::gMaxFocus; in >> label;
    cout << "maxfocus" ses agent::gMaxFocus nl;
    in >> agent::gAgentFOV; in >> label;
    cout << "agentfovy" ses agent::gAgentFOV nl;
    in >> agent::gMaxSizeAdvantage; in >> label;
    cout << "maxsizeadvantage" ses agent::gMaxSizeAdvantage nl;

	if( version >= 38 )
	{
		string value;

		__PROP("bodyGreenChannel", value);

		if( value == "I" )
		{
			agent::gBodyGreenChannel = agent::BGC_ID;
		}
		else if( value == "L" )
		{
			agent::gBodyGreenChannel = agent::BGC_LIGHT;
		}
		else
		{
			float fval = -1;
			istrstream sin(value.c_str());
			sin >> fval;
			if( sin.fail() )
			{
				error(2, "Invalid float for property bodyGreenChannel (", value.c_str(), ")");
			}

			agent::gBodyGreenChannel = agent::BGC_CONST;
			agent::gBodyGreenChannelConstValue = fval;
		}

		__PROP("noseColor", value);

		if( value == "L" )
		{
			agent::gNoseColor = agent::NC_LIGHT;
		}
		else
		{
			float fval = -1;
			istrstream sin(value.c_str());
			sin >> fval;
			if( sin.fail() )
			{
				error(2, "Invalid float for property noseColor (", value.c_str(), ")");
			}

			agent::gNoseColor = agent::NC_CONST;
			agent::gNoseColorConstValue = fval;
		}
		
	}
	else
	{
		agent::gBodyGreenChannel = agent::BGC_ID;
		agent::gNoseColor = agent::NC_LIGHT;
	}

    in >> fPower2Energy; in >> label;
    cout << "power2energy" ses fPower2Energy nl;
    in >> agent::gEat2Energy; in >> label;
    cout << "eat2energy" ses agent::gEat2Energy nl;
    in >> agent::gMate2Energy; in >> label;
    cout << "mate2energy" ses agent::gMate2Energy nl;
    in >> agent::gFight2Energy; in >> label;
    cout << "fight2energy" ses agent::gFight2Energy nl;
	if( version >= 47 )
	{
		__PROP( "minsizepenalty", agent::gMinSizePenalty );
	}
	else
	{
		agent::gMinSizePenalty = 0.0;
	}
    in >> agent::gMaxSizePenalty; in >> label;
    cout << "maxsizepenalty" ses agent::gMaxSizePenalty nl;
    in >> agent::gSpeed2Energy; in >> label;
    cout << "speed2energy" ses agent::gSpeed2Energy nl;
    in >> agent::gYaw2Energy; in >> label;
    cout << "yaw2energy" ses agent::gYaw2Energy nl;
    in >> agent::gLight2Energy; in >> label;
    cout << "light2energy" ses agent::gLight2Energy nl;
    in >> agent::gFocus2Energy; in >> label;
    cout << "focus2energy" ses agent::gFocus2Energy nl;

	if( version >= 39 )
	{
		// For earlier versions these will be initialized in initworld()
		__PROP( "pickup2energy", agent::gPickup2Energy );
		__PROP( "drop2energy", agent::gDrop2Energy );
		__PROP( "carryAgent2energy", agent::gCarryAgent2Energy );
		__PROP( "carryAgentSize2energy", agent::gCarryAgentSize2Energy );
		__PROP( "carryFood2energy", food::gCarryFood2Energy );
		__PROP( "carryBrick2energy", brick::gCarryBrick2Energy );
	}
	
    in >> brain::gNeuralValues.maxsynapse2energy; in >> label;
    cout << "maxsynapse2energy" ses brain::gNeuralValues.maxsynapse2energy nl;
    in >> agent::gFixedEnergyDrain; in >> label;
    cout << "fixedenergydrain" ses agent::gFixedEnergyDrain nl;
    in >> brain::gDecayRate; in >> label;
    cout << "decayrate" ses brain::gDecayRate nl;

	if( version >= 18 )
	{
		in >> fAgentHealingRate; in >> label;
		
		if( fAgentHealingRate > 0.0 )
			fHealing = 1;					// a bool flag to check to see if healing is turned on is faster than always comparing a float.
		cout << "AgentHealingRate" ses fAgentHealingRate nl;
	}
	
    in >> fEatThreshold; in >> label;
    cout << "eatthreshold" ses fEatThreshold nl;
    in >> fMateThreshold; in >> label;
    cout << "matethreshold" ses fMateThreshold nl;
    in >> fFightThreshold; in >> label;
    cout << "fightthreshold" ses fFightThreshold nl;
	if( version >= 43 )
	{
		PROP( FightFraction );
	}
	if( version >= 38 )
	{

		PROP(GiveThreshold);
		PROP(GiveFraction);
	}
	if( version >= 39 )
	{
		PROP(PickupThreshold);
		PROP(DropThreshold);
	}
    in >> genome::gMiscBias; in >> label;
    cout << "miscbias" ses genome::gMiscBias nl;
    in >> genome::gMiscInvisSlope; in >> label;
    cout << "miscinvslope" ses genome::gMiscInvisSlope nl;
    in >> brain::gLogisticSlope; in >> label;
    cout << "logisticslope" ses brain::gLogisticSlope nl;
    in >> brain::gMaxWeight; in >> label;
    cout << "maxweight" ses brain::gMaxWeight nl;
	if( version >= 52 )
	{
		__PROP( "enableInitWeightRngSeed", brain::gEnableInitWeightRngSeed );
		__PROP( "minInitWeightRngSeed", brain::gMinInitWeightRngSeed );
		__PROP( "maxInitWeightRngSeed", brain::gMaxInitWeightRngSeed );

		RandomNumberGenerator::set( RandomNumberGenerator::INIT_WEIGHT,
									RandomNumberGenerator::LOCAL );

	}
	else
	{
		brain::gEnableInitWeightRngSeed = false;
	}
    in >> brain::gInitMaxWeight; in >> label;
    cout << "initmaxweight" ses brain::gInitMaxWeight nl;
    in >> genome::gMinBitProb; in >> label;
    cout << "minbitprob" ses genome::gMinBitProb nl;
    in >> genome::gMaxBitProb; in >> label;
    cout << "maxbitprob" ses genome::gMaxBitProb nl;
	
	if( version >= 31 )
	{
		fSolidObjects = 0;
		int solidAgents, solidFood, solidBricks;
		in >> solidAgents; in >> label;
		cout << "solidAgents" ses solidAgents nl;
		if( solidAgents ) fSolidObjects |= AGENTTYPE;
		in >> solidFood; in >> label;
		cout << "solidFood" ses solidFood nl;
		if( solidFood ) fSolidObjects |= FOODTYPE;
		in >> solidBricks; in >> label;
		cout << "solidBricks" ses solidBricks nl;
		if( solidBricks ) fSolidObjects |= BRICKTYPE;
	}
	else
		fSolidObjects = BRICKTYPE;

	printf( "\tfSolidObjects = 0x" );
	for( unsigned int i = 0; i < sizeof(fSolidObjects)*8; i++ )
		printf( "%d", (fSolidObjects>>(31-i)) & 1 );
	printf( "\n" );
	
    in >> agent::gAgentHeight; in >> label;
    cout << "agentheight" ses agent::gAgentHeight nl;
    in >> food::gFoodHeight; in >> label;
    cout << "foodheight" ses food::gFoodHeight nl;
    in >> food::gFoodColor.r >> food::gFoodColor.g >> food::gFoodColor.b >> label;
    cout << "foodcolor = (" << food::gFoodColor.r cms
                               food::gFoodColor.g cms
                               food::gFoodColor.b pnl;
	if( version >= 47 )
	{
		__PROP( "brickheight", brick::gBrickHeight );
	    __PROP( "brickcolor", brick::gBrickColor );
	}
	else
	{
		brick::gBrickColor.set( 0.6, 0.2, 0.2 );
		brick::gBrickHeight = 0.5;
	}
    in >> barrier::gBarrierHeight; in >> label;
    cout << "barrierheight" ses barrier::gBarrierHeight nl;
    in >> barrier::gBarrierColor.r >> barrier::gBarrierColor.g >> barrier::gBarrierColor.b >> label;
    cout << "barriercolor = (" << barrier::gBarrierColor.r cms
                                  barrier::gBarrierColor.g cms
                                  barrier::gBarrierColor.b pnl;
    in >> fGroundColor.r >> fGroundColor.g >> fGroundColor.b >> label;
    cout << "groundcolor = (" << fGroundColor.r cms
                                 fGroundColor.g cms
                                 fGroundColor.b pnl;
    in >> fGroundClearance; in >> label;
    cout << "fGroundClearance" ses fGroundClearance nl;
    in >> fCameraColor.r >> fCameraColor.g >> fCameraColor.b >> label;
    cout << "cameracolor = (" << fCameraColor.r cms
                                 fCameraColor.g cms
                                 fCameraColor.b pnl;
    in >> fCameraRadius; in >> label;
    cout << "camradius" ses fCameraRadius nl;
    in >> fCameraHeight; in >> label;
    cout << "camheight" ses fCameraHeight nl;
    in >> fCameraRotationRate; in >> label;
    cout << "camrotationrate" ses fCameraRotationRate nl;
    fRotateWorld = (fCameraRotationRate != 0.0);	//Boolean for enabling or disabling world roation (CMB 3/19/06)
    in >> fCameraAngleStart; in >> label;
    cout << "camanglestart" ses fCameraAngleStart nl;
    in >> fCameraFOV; in >> label;
    cout << "camfov" ses fCameraFOV nl;
    in >> fMonitorAgentRank; in >> label;
    if (!fGraphics)
        fMonitorAgentRank = 0; // cannot monitor agent brain without graphics
    cout << "fMonitorAgentRank" ses fMonitorAgentRank nl;

    in >> ignoreShort3; in >> label;
    cout << "fMonitorCritWinWidth (ignored)" ses ignoreShort3 nl;
    in >> ignoreShort4; in >> label;
    cout << "fMonitorCritWinHeight (ignored)" ses ignoreShort4 nl;

    in >> fBrainMonitorStride; in >> label;
    cout << "fBrainMonitorStride" ses fBrainMonitorStride nl;
    in >> globals::worldsize; in >> label;
    cout << "worldsize" ses globals::worldsize nl;
    long numbarriers;
    float x1, z1, x2, z2;
    in >> numbarriers; in >> label;
    cout << "numbarriers = " << numbarriers nl;
	if( version >= 27 )
	{
		bool ratioBarrierPositions = false;
		
		if( version >= 47 )
		{
			__PROP( "ratioBarrierPositions", ratioBarrierPositions );
		}

		for( int i = 0; i < numbarriers; i++ )
		{
			int numKeyFrames;
			in >> numKeyFrames >> label;
			cout << "barrier[" << i << "].numKeyFrames" ses numKeyFrames nl;
			barrier* b = new barrier( numKeyFrames );
			for( int j = 0; j < numKeyFrames; j++ )
			{
				long t;
				in >> t >> x1 >> z1 >> x2 >> z2 >> label;
				if( ratioBarrierPositions )
				{
					x1 *= globals::worldsize;
					x2 *= globals::worldsize;
					z1 *= globals::worldsize;
					z2 *= globals::worldsize;
				}
				cout << "barrier[" << i << "].keyFrame[" << j << "]" ses t sp x1 sp z1 sp x2 sp z2 nl;
				b->setKeyFrame( t, x1, z1, x2, z2 );
			}
			barrier::gXSortedBarriers.add( b );
		}
	}
	else
	{
		for( int i = 0; i < numbarriers; i++ )
		{
			in >> x1 >> z1 >> x2 >> z2 >> label;
			cout << "barrier #" << i << " position = ("
				 << x1 cms z1 << ") to (" << x2 cms z2 pnl;
			barrier* b = new barrier( 1 );
			b->setKeyFrame( 0, x1, z1, x2, z2 );
			barrier::gXSortedBarriers.add( b );
		}
	}
	
    if (version < 2)
    {
		cout nlf;
     	fb.close();
        return;
	}

    in >> fMonitorGeneSeparation; in >> label;
    cout << "genesepmon" ses fMonitorGeneSeparation nl;
    in >> fRecordGeneSeparation; in >> label;
    cout << "geneseprec" ses fRecordGeneSeparation nl;

    if (version < 3)
    {
		cout nlf;
     	fb.close();
        return;
	}

    in >> fChartBorn; in >> label;
    cout << "fChartBorn" ses fChartBorn nl;
    in >> fChartFitness; in >> label;
    cout << "fChartFitness" ses fChartFitness nl;
    in >> fChartFoodEnergy; in >> label;
    cout << "fChartFoodEnergy" ses fChartFoodEnergy nl;
    in >> fChartPopulation; in >> label;
    cout << "fChartPopulation" ses fChartPopulation nl;

    if (version < 4)
    {
		cout nlf;
     	fb.close();
        return;
	}

    in >> fNumDomains; in >> label;
    cout << "numdomains" ses fNumDomains nl;
    if (fNumDomains < 1)
    {
		char tempstring[256];
        sprintf(tempstring,"%s %ld %s", "Number of fitness domains must always be > 0 (not ", fNumDomains,").\nIt will be reset to 1.");
        error(1, tempstring);
        fNumDomains = 1;
    }
    if (fNumDomains > MAXDOMAINS)
    {
		char tempstring[256];
        sprintf(tempstring, "Too many fitness domains requested (%ld > %d)", fNumDomains, MAXDOMAINS);
        error(2,tempstring);
    }

    short id;
	long totmaxnumagents = 0;
	long totminnumagents = 0;
	int	numFoodPatchesNeedingRemoval = 0;
	
	for (id = 0; id < fNumDomains; id++)
	{
		if( version < 32 )
		{
			in >> fDomains[id].minNumAgents; in >> label;
			cout << "minNumAgents in domains[" << id << "]" ses fDomains[id].minNumAgents nl;
			in >> fDomains[id].maxNumAgents; in >> label;
			cout << "maxNumAgents in domains[" << id << "]" ses fDomains[id].maxNumAgents nl;
			in >> fDomains[id].initNumAgents; in >> label;
			cout << "initNumAgents in domains[" << id << "]" ses fDomains[id].initNumAgents nl;
			if( version >= 9 )
			{
				in >> fDomains[id].numberToSeed; in >> label;
				cout << "numberToSeed in domains[" << id << "]" ses fDomains[id].numberToSeed nl;
				in >> fDomains[id].probabilityOfMutatingSeeds; in >> label;
				cout << "probabilityOfMutatingSeeds in domains [" << id << "]" ses fDomains[id].probabilityOfMutatingSeeds nl;
			}
			else
			{
				fDomains[id].numberToSeed = 0;
				fDomains[id].probabilityOfMutatingSeeds = 0.0;
			}
		}
		
		if (version >= 19)
		{
			in >> fDomains[id].centerX; in >> label;
			cout << "centerX of domains[" << id << "]" ses fDomains[id].centerX nl;
			in >> fDomains[id].centerZ; in >> label;
			cout << "centerZ of domains[" << id << "]" ses fDomains[id].centerZ nl;
			in >> fDomains[id].sizeX; in >> label;
			cout << "sizeX of domains[" << id << "]" ses fDomains[id].sizeX nl;
			in >> fDomains[id].sizeZ; in >> label;
			cout << "sizeZ of domains[" << id << "]" ses fDomains[id].sizeZ nl;

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

			printf("NEW DOMAIN\n");
			printf("\tstartX: %.2f\n", fDomains[id].startX);
			printf("\tstartZ: %.2f\n", fDomains[id].startZ);
			printf("\tendX: %.2f\n", fDomains[id].endX);
			printf("\tendZ: %.2f\n", fDomains[id].endZ);
			printf("\tabsoluteX: %.2f\n", fDomains[id].absoluteSizeX);
			printf("\tabsoluteZ: %.2f\n", fDomains[id].absoluteSizeZ);

			if( version >= 32 )
			{
				float minAgentsFraction, maxAgentsFraction, initAgentsFraction, initSeedsFraction;
				
				in >> minAgentsFraction; in >> label;
				cout << "minAgentsFraction in domains[" << id << "]" ses minAgentsFraction nl;
				if( minAgentsFraction < 0.0 )
				{
					minAgentsFraction = fDomains[id].sizeX * fDomains[id].sizeZ;
					cout << "\tminAgentsFraction in domains[" << id << "]" ses minAgentsFraction nl;
				}
				fDomains[id].minNumAgents = nint( minAgentsFraction * fMinNumAgents );
				cout << "\tminNumAgents in domains[" << id << "]" ses fDomains[id].minNumAgents nl;
				
				in >> maxAgentsFraction; in >> label;
				cout << "maxAgentsFraction in domains[" << id << "]" ses maxAgentsFraction nl;
				if( maxAgentsFraction < 0.0 )
				{
					maxAgentsFraction = fDomains[id].sizeX * fDomains[id].sizeZ;
					cout << "\tmaxAgentsFraction in domains[" << id << "]" ses maxAgentsFraction nl;
				}
				fDomains[id].maxNumAgents = nint( maxAgentsFraction * fMaxNumAgents );
				cout << "\tmaxNumAgents in domains[" << id << "]" ses fDomains[id].maxNumAgents nl;
				
				in >> initAgentsFraction; in >> label;
				cout << "initAgentsFraction in domains[" << id << "]" ses initAgentsFraction nl;
				if( initAgentsFraction < 0.0 )
				{
					initAgentsFraction = fDomains[id].sizeX * fDomains[id].sizeZ;
					cout << "\tinitAgentsFraction in domains[" << id << "]" ses initAgentsFraction nl;
				}
				fDomains[id].initNumAgents = nint( initAgentsFraction * fInitNumAgents );
				cout << "\tinitNumAgents in domains[" << id << "]" ses fDomains[id].initNumAgents nl;
				
				in >> initSeedsFraction; in >> label;
				cout << "initSeedsFraction in domains[" << id << "]" ses initSeedsFraction nl;
				if( initSeedsFraction < 0.0 )
				{
					initSeedsFraction = fDomains[id].sizeX * fDomains[id].sizeZ;
					cout << "\tinitSeedsFraction in domains[" << id << "]" ses initSeedsFraction nl;
				}
				fDomains[id].numberToSeed = nint( initSeedsFraction * fNumberToSeed );
				cout << "\tnumberToSeed in domains[" << id << "]" ses fDomains[id].numberToSeed nl;
				
				in >> fDomains[id].probabilityOfMutatingSeeds; in >> label;
				cout << "probabilityOfMutatingSeeds in domains [" << id << "]" ses fDomains[id].probabilityOfMutatingSeeds nl;
				if( fDomains[id].probabilityOfMutatingSeeds < 0.0 )
				{
					fDomains[id].probabilityOfMutatingSeeds = fProbabilityOfMutatingSeeds;
					cout << "\tprobabilityOfMutatingSeeds in domains [" << id << "]" ses fDomains[id].probabilityOfMutatingSeeds nl;
				}

				float initFoodFraction;
				in >> initFoodFraction; in >> label;
				cout << "initFoodFraction of domains[" << id << "]" ses initFoodFraction nl;
				if( initFoodFraction >= 0.0 )
					fDomains[id].initFoodCount = nint( initFoodFraction * fInitFoodCount );
				else
					fDomains[id].initFoodCount = nint( fDomains[id].sizeX * fDomains[id].sizeZ * fInitFoodCount );
				cout << "\tinitFoodCount of domains[" << id << "]" ses fDomains[id].initFoodCount nl;
				float minFoodFraction;
				in >> minFoodFraction; in >> label;
				cout << "minFoodFraction of domains[" << id << "]" ses minFoodFraction nl;
				if( minFoodFraction >= 0.0 )
					fDomains[id].minFoodCount = nint( minFoodFraction * fMinFoodCount );
				else
					fDomains[id].minFoodCount = nint( fDomains[id].sizeX * fDomains[id].sizeZ * fMinFoodCount );
				cout << "\tminFoodCount of domains[" << id << "]" ses fDomains[id].minFoodCount nl;
				float maxFoodFraction;
				in >> maxFoodFraction; in >> label;
				cout << "maxFoodFraction of domains[" << id << "]" ses maxFoodFraction nl;
				if( maxFoodFraction >= 0.0 )
					fDomains[id].maxFoodCount = nint( maxFoodFraction * fMaxFoodCount );
				else
					fDomains[id].maxFoodCount = nint( fDomains[id].sizeX * fDomains[id].sizeZ * fMaxFoodCount );
				cout << "\tmaxFoodCount of domains[" << id << "]" ses fDomains[id].maxFoodCount nl;
				float maxFoodGrownFraction;
				in >> maxFoodGrownFraction; in >> label;
				cout << "maxFoodGrownFraction of domains[" << id << "]" ses maxFoodGrownFraction nl;
				if( maxFoodGrownFraction >= 0.0 )
					fDomains[id].maxFoodGrownCount = nint( maxFoodGrownFraction * fMaxFoodGrownCount );
				else
					fDomains[id].maxFoodGrownCount = nint( fDomains[id].sizeX * fDomains[id].sizeZ * fMaxFoodGrownCount );
				cout << "\tmaxFoodGrownCount of domains[" << id << "]" ses fDomains[id].maxFoodGrownCount nl;
				in >> fDomains[id].foodRate; in >> label;
				if( fDomains[id].foodRate < 0.0 )
					fDomains[id].foodRate = fFoodRate;
				cout << "foodRate of domains[" << id << "]" ses fDomains[id].foodRate nl;
				in >> fDomains[id].numFoodPatches; in >> label;
				cout << "numFoodPatches of domains[" << id << "]" ses fDomains[id].numFoodPatches nl;
				in >> fDomains[id].numBrickPatches; in >> label;
				cout << "numBrickPatches of domains[" << id << "]" ses fDomains[id].numBrickPatches nl;
			}
			else
			{
				in >> fDomains[id].numFoodPatches; in >> label;
				cout << "numFoodPatches of domains[" << id << "]" ses fDomains[id].numFoodPatches nl;
				in >> fDomains[id].numBrickPatches; in >> label;
				cout << "numBrickPatches of domains[" << id << "]" ses fDomains[id].numBrickPatches nl;
				in >> fDomains[id].foodRate; in >> label;
				if( fDomains[id].foodRate < 0.0 )
					fDomains[id].foodRate = fFoodRate;
				cout << "foodRate of domains[" << id << "]" ses fDomains[id].foodRate nl;
				in >> fDomains[id].initFoodCount; in >> label;
				cout << "initFoodCount of domains[" << id << "]" ses fDomains[id].initFoodCount nl;
				in >> fDomains[id].minFoodCount; in >> label;
				cout << "minFoodCount of domains[" << id << "]" ses fDomains[id].minFoodCount nl;
				in >> fDomains[id].maxFoodCount; in >> label;
				cout << "maxFoodCount of domains[" << id << "]" ses fDomains[id].maxFoodCount nl;
				in >> fDomains[id].maxFoodGrownCount; in >> label;
				cout << "maxFoodGrownCount of domains[" << id << "]" ses fDomains[id].maxFoodGrownCount nl;
			}
			
			// Create an array of FoodPatches
			fDomains[id].fFoodPatches = new FoodPatch[fDomains[id].numFoodPatches];
			float patchFractionSpecified = 0.0;

			for (int i = 0; i < fDomains[id].numFoodPatches; i++)
			{
				float foodFraction, foodRate;
				float centerX, centerZ;  // should be from 0.0 -> 1.0
				float sizeX, sizeZ;  // should be from 0.0 -> 1.0
				float nhsize;
				float initFoodFraction, minFoodFraction, maxFoodFraction, maxFoodGrownFraction;
				int initFood, minFood, maxFood, maxFoodGrown;
				int shape, distribution;
				int period;
				float onFraction, phase;
				bool removeFood;

				in >> foodFraction; in >> label;
				cout << "foodFraction" ses foodFraction nl;

				if( version >= 32 )
				{
					in >> initFoodFraction; in >> label;
					cout << "initFoodFraction" ses initFoodFraction nl;
					if( initFoodFraction < 0.0 )
					{
						initFoodFraction = foodFraction;
						cout << "\tinitFoodFraction" ses initFoodFraction nl;
					}
					initFood = nint( initFoodFraction * fDomains[id].initFoodCount );
					cout << "\tinitFood" ses initFood nl;
					
					in >> minFoodFraction; in >> label;
					cout << "minFoodFraction" ses minFoodFraction nl;
					if( minFoodFraction < 0.0 )
					{
						minFoodFraction = foodFraction;
						cout << "\tminFoodFraction" ses minFoodFraction nl;
					}
					minFood = nint( minFoodFraction * fDomains[id].minFoodCount );
					cout << "\tminFood" ses minFood nl;
					
					in >> maxFoodFraction; in >> label;
					cout << "maxFoodFraction" ses maxFoodFraction nl;
					if( maxFoodFraction < 0.0 )
					{
						maxFoodFraction = foodFraction;
						cout << "\tmaxFoodFraction" ses maxFoodFraction nl;
					}
					maxFood = nint( maxFoodFraction * fDomains[id].maxFoodCount );
					cout << "\tmaxFood" ses maxFood nl;
					
					in >> maxFoodGrownFraction; in >> label;
					cout << "maxFoodFractionGrown" ses maxFoodGrownFraction nl;
					if( maxFoodGrownFraction < 0.0 )
					{
						maxFoodGrownFraction = foodFraction;
						cout << "\tmaxFoodGrownFraction" ses maxFoodGrownFraction nl;
					}
					maxFoodGrown = nint( maxFoodGrownFraction * fDomains[id].maxFoodGrownCount );
					cout << "\tmaxFoodGrown" ses maxFoodGrown nl;
					
					in >> foodRate; in >> label;
					cout << "foodRate" ses foodRate nl;
					if (foodRate < 0.0)
					{
						foodRate = fDomains[id].foodRate;
						cout << "\tfoodRate" ses foodRate nl;
					}
				}
				else
				{
					in >> foodRate; in >> label;
					cout << "foodRate" ses foodRate nl;
					if (foodRate == 0.0)
					{
						foodRate = fDomains[id].foodRate;
						cout << "\tfoodRate" ses foodRate nl;
					}
					in >> initFood; in >> label;
					cout << "initFood" ses initFood nl;
					in >> minFood; in >> label;
					cout << "minFood" ses minFood nl;
					in >> maxFood; in >> label;
					cout << "maxFood" ses maxFood nl;
					in >> maxFoodGrown; in >> label;
					cout << "maxFoodGrown" ses maxFoodGrown nl;
					// If we have a legitimate fraction specified, see if we need to set the food limits
					if( foodFraction > 0.0 )
					{
						// Check for negative values...if found then use global food counts and this patch's fraction
						if (initFood < 0) initFood = (int)(foodFraction * fDomains[id].initFoodCount);
						if (minFood < 0) minFood = (int)(foodFraction * fDomains[id].minFoodCount);
						if (maxFood < 0) maxFood = (int)(foodFraction * fDomains[id].maxFoodCount);
						if (maxFoodGrown < 0) maxFoodGrown = (int)(foodFraction * fDomains[id].maxFoodGrownCount);
					}
				}
				in >> centerX; in >> label;
				cout << "centerX" ses centerX nl;
				in >> centerZ; in >> label;
				cout << "centerZ" ses centerZ nl;
				in >> sizeX; in >> label;
				cout << "sizeX" ses sizeX nl;
				in >> sizeZ; in >> label;
				cout << "sizeZ" ses sizeZ nl;
				in >> shape; in >> label;
				cout << "shape" ses shape nl;
				in >> distribution; in >> label;
				cout << "distribution" ses distribution nl;
				in >> nhsize; in >> label;
				cout << "neighborhoodSize" ses nhsize nl;
				if( version >= 26 )
				{
					in >> period; in >> label;
					cout << "period" ses period nl;
					in >> onFraction; in >> label;
					cout << "onFraction" ses onFraction nl;
					in >> phase; in >> label;
					cout << "phase" ses phase nl;
					in >> removeFood; in >> label;
					cout << "removeFood" ses removeFood nl;
					if( removeFood )
						numFoodPatchesNeedingRemoval++;
				}
				else
				{
					period = 0;
					cout << "+period" ses period nl;
					onFraction = 1.0;
					cout << "+onFraction" ses onFraction nl;
					phase = 0.0;
					cout << "+phase" ses phase nl;
					removeFood = false;
					cout << "+removeFood" ses removeFood nl;
				}
				
				fDomains[id].fFoodPatches[i].init(centerX, centerZ, sizeX, sizeZ, foodRate, initFood, minFood, maxFood, maxFoodGrown, foodFraction, shape, distribution, nhsize, period, onFraction, phase, removeFood, &fStage, &(fDomains[id]), id);

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

			// Create an array of BrickPatches
			fDomains[id].fBrickPatches = new BrickPatch[fDomains[id].numBrickPatches];
			for (int i = 0; i < fDomains[id].numBrickPatches; i++)
			{
				float centerX, centerZ;  // should be from 0.0 -> 1.0
				float sizeX, sizeZ;  // should be from 0.0 -> 1.0
				float nhsize;
				int shape, distribution, brickCount;

				in >> centerX; in >> label;
				cout << "centerX" ses centerX nl;
				in >> centerZ; in >> label;
				cout << "centerZ" ses centerZ nl;
				in >> sizeX; in >> label;
				cout << "sizeX" ses sizeX nl;
				in >> sizeZ; in >> label;
				cout << "sizeZ" ses sizeZ nl;
				in >> brickCount; in >> label;
				cout << "brickCount" ses brickCount nl;
				in >> shape; in >> label;
				cout << "shape" ses shape nl;
				in >> distribution; in >> label;
				cout << "distribution" ses distribution nl;
				in >> nhsize; in >> label;
				cout << "neighborhoodSize" ses nhsize nl;

				fDomains[id].fBrickPatches[i].init(centerX, centerZ, sizeX, sizeZ, brickCount, shape, distribution, nhsize, &fStage, &(fDomains[id]), id);
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
	
    for (id = 0; id < fNumDomains; id++)
    {
        fDomains[id].numAgents = 0;
        fDomains[id].numcreated = 0;
        fDomains[id].numborn = 0;
        fDomains[id].numbornsincecreated = 0;
        fDomains[id].numdied = 0;
        fDomains[id].lastcreate = 0;
        fDomains[id].maxgapcreate = 0;
        fDomains[id].ifit = 0;
        fDomains[id].jfit = 1;
        fDomains[id].fittest = NULL;
	}
	
	if( version >= 17 )
	{
		in >> fUseProbabilisticFoodPatches; in >> label;
		cout << "useProbabilisticFoodPatches" ses fUseProbabilisticFoodPatches nl;
	}
	else
		fUseProbabilisticFoodPatches = false;

    if (version < 5)
    {
		cout nlf;
     	fb.close();
        return;
	}

    in >> fChartGeneSeparation; in >> label;
    cout << "fChartGeneSeparation" ses fChartGeneSeparation nl;
    if (fChartGeneSeparation)
        fMonitorGeneSeparation = true;

    if (version < 6)
	{
		cout nlf;
     	fb.close();
        return;
	}

	if( version >= 36 )
	{
		string model;
		in >> model; in >> label;

		if( model == "F" )
		{
			brain::gNeuralValues.model = brain::NeuralValues::FIRING_RATE;
		}
		else if( model == "S" )
		{
			brain::gNeuralValues.model = brain::NeuralValues::SPIKING;			
		}
		else if( model == "T" )
		{
			assert( version >= 50 );
			brain::gNeuralValues.model = brain::NeuralValues::TAU;
		}
		else
		{
			error(1, "Invalid NeuronModel in worldfile (%s)", model.c_str());
		}
	}
	else
	{
		brain::gNeuralValues.model = brain::NeuralValues::FIRING_RATE;			
	}
	cout << "neuronModel" ses brain::gNeuralValues.model nl;

	if( version >= 50 )
	{
		__PROP( "tauMin", brain::gNeuralValues.Tau.minVal );
		__PROP( "tauMax", brain::gNeuralValues.Tau.maxVal );
		__PROP( "tauSeed", brain::gNeuralValues.Tau.seedVal );
	}

    in >> brain::gNeuralValues.mininternalneurgroups; in >> label;
    cout << "mininternalneurgroups" ses brain::gNeuralValues.mininternalneurgroups nl;
    in >> brain::gNeuralValues.maxinternalneurgroups; in >> label;
    cout << "maxinternalneurgroups" ses brain::gNeuralValues.maxinternalneurgroups nl;
    in >> brain::gNeuralValues.mineneurpergroup; in >> label;
    cout << "mineneurpergroup" ses brain::gNeuralValues.mineneurpergroup nl;
    in >> brain::gNeuralValues.maxeneurpergroup; in >> label;
    cout << "maxeneurpergroup" ses brain::gNeuralValues.maxeneurpergroup nl;
    in >> brain::gNeuralValues.minineurpergroup; in >> label;
    cout << "minineurpergroup" ses brain::gNeuralValues.minineurpergroup nl;
    in >> brain::gNeuralValues.maxineurpergroup; in >> label;
    cout << "maxineurpergroup" ses brain::gNeuralValues.maxineurpergroup nl;
	float unusedFloat;
    in >> unusedFloat; in >> label;
    cout << "minbias (not used)" ses unusedFloat nl;
    in >> brain::gNeuralValues.maxbias; in >> label;
    cout << "maxbias" ses brain::gNeuralValues.maxbias nl;
    in >> brain::gNeuralValues.minbiaslrate; in >> label;
    cout << "minbiaslrate" ses brain::gNeuralValues.minbiaslrate nl;
    in >> brain::gNeuralValues.maxbiaslrate; in >> label;
    cout << "maxbiaslrate" ses brain::gNeuralValues.maxbiaslrate nl;
    in >> brain::gNeuralValues.minconnectiondensity; in >> label;
    cout << "minconnectiondensity" ses brain::gNeuralValues.minconnectiondensity nl;
    in >> brain::gNeuralValues.maxconnectiondensity; in >> label;
    cout << "maxconnectiondensity" ses brain::gNeuralValues.maxconnectiondensity nl;
    in >> brain::gNeuralValues.mintopologicaldistortion; in >> label;
    cout << "mintopologicaldistortion" ses brain::gNeuralValues.mintopologicaldistortion nl;
    in >> brain::gNeuralValues.maxtopologicaldistortion; in >> label;
    cout << "maxtopologicaldistortion" ses brain::gNeuralValues.maxtopologicaldistortion nl;
	if( version >= 50 )
	{
		__PROP( "enableTopologicalDistortionRngSeed", brain::gNeuralValues.enableTopologicalDistortionRngSeed );
		__PROP( "minTopologicalDistortionRngSeed", brain::gNeuralValues.minTopologicalDistortionRngSeed );
		__PROP( "maxTopologicalDistortionRngSeed", brain::gNeuralValues.maxTopologicalDistortionRngSeed );

		RandomNumberGenerator::set( RandomNumberGenerator::TOPOLOGICAL_DISTORTION,
									RandomNumberGenerator::LOCAL );
	}
	else
	{
		brain::gNeuralValues.enableTopologicalDistortionRngSeed = false;
	}
    in >> brain::gNeuralValues.maxneuron2energy; in >> label;
    cout << "maxneuron2energy" ses brain::gNeuralValues.maxneuron2energy nl;

    in >> brain::gNumPrebirthCycles; in >> label;
    cout << "numprebirthcycles" ses brain::gNumPrebirthCycles nl;

	if( version >= 40 )
	{
		__PROP( "seedFightBias", genome::gSeedFightBias );
		__PROP( "seedFightExcitation", genome::gSeedFightExcitation );
		__PROP( "seedGiveBias", genome::gSeedGiveBias );
	}
	else
	{
		genome::gSeedFightBias = 0.5;
		genome::gSeedFightExcitation = 1.0;
		genome::gSeedGiveBias = 0.0;
	}

	if( version >= 39 )
	{
		// For earlier versions these will be initialized in initworld()
		__PROP( "seedPickupBias", genome::gSeedPickupBias );
		__PROP( "seedDropBias", genome::gSeedDropBias );
		__PROP( "seedPickupExcitation", genome::gSeedPickupExcitation );
		__PROP( "seedDropExcitation", genome::gSeedDropExcitation );
	}

    InitNeuralValues();

    cout << "maxneurgroups" ses brain::gNeuralValues.maxneurgroups nl;
    cout << "maxneurons" ses brain::gNeuralValues.maxneurons nl;
    cout << "maxsynapses" ses brain::gNeuralValues.maxsynapses nl;

    in >> fOverHeadRank; in >> label;
    cout << "fOverHeadRank" ses fOverHeadRank nl;
    in >> fAgentTracking; in >> label;
    cout << "fAgentTracking" ses fAgentTracking nl;
    in >> fMinFoodEnergyAtDeath; in >> label;
    cout << "minfoodenergyatdeath" ses fMinFoodEnergyAtDeath nl;

    in >> genome::gGrayCoding; in >> label;
    cout << "graycoding" ses genome::gGrayCoding nl;

    if (version < 7)
    {
		cout nlf;
		fb.close();
		return;
	}
	if( version >= 21 )
	{
		in >> fSmiteMode; in >> label;
		cout << "smiteMode" ses fSmiteMode nl;
		assert( fSmiteMode == 'L' || fSmiteMode == 'R' || fSmiteMode == 'O' );		// fSmiteMode must be 'L', 'R', or 'O'
	}
    in >> fSmiteFrac; in >> label;
	
    cout << "smiteFrac" ses fSmiteFrac nl;
    in >> fSmiteAgeFrac; in >> label;
    cout << "smiteAgeFrac" ses fSmiteAgeFrac nl;
	
	// agent::gNumDepletionSteps and agent:gMaxPopulationFraction must both be initialized to zero
	// for backward compatibility, and we depend on that fact here, so don't change it
	if( version >= 23 )
	{
		in >> agent::gNumDepletionSteps; in >> label;
		cout << "NumDepletionSteps" ses agent::gNumDepletionSteps nl;
		if( agent::gNumDepletionSteps )
			agent::gMaxPopulationPenaltyFraction = 1.0 / (float) agent::gNumDepletionSteps;
		cout << ".MaxPopulationPenaltyFraction" ses agent::gMaxPopulationPenaltyFraction nl;
	}
	else
	{
		cout << "+NumDepletionSteps" ses agent::gNumDepletionSteps nl;
		cout << ".MaxPopulationPenaltyFraction" ses agent::gMaxPopulationPenaltyFraction nl;
	}
	
	if( version >= 24 )
	{
		in >> fApplyLowPopulationAdvantage; in >> label;
		cout << "ApplyLowPopulationAdvantage" ses fApplyLowPopulationAdvantage nl;
	}
	else
	{
		fApplyLowPopulationAdvantage = false;
		cout << "+ApplyLowPopulationAdvantage" ses fApplyLowPopulationAdvantage nl;
	}

	if( version < 8 )
	{
		cout nlf;
		fb.close();
		return;
	}
	if( version >= 22 )
	{
		in >> fRecordBirthsDeaths; in >> label;
		cout << "RecordBirthsDeaths" ses fRecordBirthsDeaths nl;
	}

	if( version >= 33 )
	{
		PROP( RecordPosition );
	}

	if( version >= 38 )
	{
		PROP( RecordContacts );
		PROP( RecordCollisions );
	}

	if( version >= 42 )
	{
		PROP( RecordCarry );
	}
	
	if( version >= 11 )
	{
		in >> fBrainAnatomyRecordAll; in >> label;
		cout << "brainAnatomyRecordAll" ses fBrainAnatomyRecordAll nl;
		in >> fBrainFunctionRecordAll; in >> label;
		cout << "brainFunctionRecordAll" ses fBrainFunctionRecordAll nl;
		
		if( version >= 12 )
		{
			in >> fBrainAnatomyRecordSeeds; in >> label;
			cout << "brainAnatomyRecordSeeds" ses fBrainAnatomyRecordSeeds nl;
			in >> fBrainFunctionRecordSeeds; in >> label;
			cout << "brainFunctionRecordSeeds" ses fBrainFunctionRecordSeeds nl;
		}
		
		in >> fBestSoFarBrainAnatomyRecordFrequency; in >> label;
		cout << "bestSoFarBrainAnatomyRecordFrequency" ses fBestSoFarBrainAnatomyRecordFrequency nl;
		in >> fBestSoFarBrainFunctionRecordFrequency; in >> label;
		cout << "bestSoFarBrainFunctionRecordFrequency" ses fBestSoFarBrainFunctionRecordFrequency nl;
		
		if( version >= 14 )
		{
			in >> fBestRecentBrainAnatomyRecordFrequency; in >> label;
			cout << "bestRecentBrainAnatomyRecordFrequency" ses fBestRecentBrainAnatomyRecordFrequency nl;
			in >> fBestRecentBrainFunctionRecordFrequency; in >> label;
			cout << "bestRecentBrainFunctionRecordFrequency" ses fBestRecentBrainFunctionRecordFrequency nl;
		}
	}
	
	if( version >= 12 )
	{
		in >> fRecordGeneStats; in >> label;
		cout << "recordGeneStats" ses fRecordGeneStats nl;
	}

	if( version >= 35 )
	{
		PROP( RecordPerformanceStats );
	}

	if( version >= 15 )
	{
		in >> fRecordFoodPatchStats; in >> label;
		cout << "recordFoodPatchStats" ses fRecordFoodPatchStats nl;
	}


	if( version >= 18 )
	{
	

		in >> fRecordComplexity; in >> label;
		cout << "recordComplexity" ses fRecordComplexity nl;
	
		if( fRecordComplexity )
		{
			if( ! fBestSoFarBrainFunctionRecordFrequency )
			{
				if( fBestRecentBrainFunctionRecordFrequency )
					fBestSoFarBrainFunctionRecordFrequency = fBestRecentBrainFunctionRecordFrequency;
				else
					fBestSoFarBrainFunctionRecordFrequency = 1000;
				cerr << "Warning: Attempted to record Complexity without recording \"best so far\" brain function.  Setting BestSoFarBrainFunctionRecordFrequency to " << fBestSoFarBrainFunctionRecordFrequency nl;
			}
			
			if( ! fBestSoFarBrainAnatomyRecordFrequency )
			{
				if( fBestRecentBrainAnatomyRecordFrequency )
					fBestSoFarBrainAnatomyRecordFrequency = fBestRecentBrainAnatomyRecordFrequency;
				else
					fBestSoFarBrainAnatomyRecordFrequency = 1000;				
				cerr << "Warning: Attempted to record Complexity without recording \"best so far\" brain anatomy.  Setting BestSoFarBrainAnatomyRecordFrequency to " << fBestSoFarBrainAnatomyRecordFrequency nl;
			}

			if( ! fBestRecentBrainFunctionRecordFrequency )
			{
				if( fBestSoFarBrainAnatomyRecordFrequency )
					fBestRecentBrainFunctionRecordFrequency = fBestSoFarBrainAnatomyRecordFrequency;
				else
					fBestRecentBrainFunctionRecordFrequency = 1000;
				cerr << "Warning: Attempted to record Complexity without recording \"best recent\" brain function.  Setting BestRecentBrainFunctionRecordFrequency to " << fBestRecentBrainFunctionRecordFrequency nl;
			}
			
			if( ! fBestRecentBrainAnatomyRecordFrequency )
			{
				if( fBestSoFarBrainAnatomyRecordFrequency )
					fBestRecentBrainAnatomyRecordFrequency = fBestSoFarBrainAnatomyRecordFrequency;
				else
					fBestRecentBrainAnatomyRecordFrequency = 1000;				
				cerr << "Warning: Attempted to record Complexity without recording \"best recent\" brain anatomy.  Setting BestRecentBrainAnatomyRecordFrequency to " << fBestRecentBrainAnatomyRecordFrequency nl;
			}
		}
		
		in >> fComplexityType; in >> label;
		if( version < 28 )
		{
			if( fComplexityType == "0" )	// zero used to mean off
				fComplexityType = "O";
			else
				fComplexityType = "P";	// on used to assume processing complexity
		}
		cout << "complexityType" ses fComplexityType nl;

		if( version >= 30 )
		{
			in >> fComplexityFitnessWeight; in >> label;
			in >> fHeuristicFitnessWeight; in >> label;
		}
		else
		{
			if( fComplexityType == "O" )		// 'O' meant complexity as a fitness function was off
				fComplexityFitnessWeight = 0.0;	// so set the complexity fitness weight to zero (turn it off)
			else
				fComplexityFitnessWeight = 1.0;	// any other complexity type used to mean use it as the fitness function
			fHeuristicFitnessWeight = 0.0;		// heuristic fitness as an actual fitness function didn't used to exist
		}
		cout << "complexityFitnessWeight" ses fComplexityFitnessWeight nl;
		cout << "heuristicFitnessWeight" ses fHeuristicFitnessWeight nl;

		if( fComplexityFitnessWeight > 0.0 )
		{
			if( ! fRecordComplexity )		//Not Recording Complexity?
			{
				cerr << "Warning: Attempted to use Complexity as fitness func without recording Complexity.  Turning on RecordComplexity." nl;
				fRecordComplexity = true;
			}
			if( ! fBrainFunctionRecordAll )	//Not recording BrainFunction?
			{
				cerr << "Warning: Attempted to use Complexity as fitness func without recording brain function.  Turning on RecordBrainFunctionAll." nl;
				fBrainFunctionRecordAll = true;
			}
			if( ! fBrainAnatomyRecordAll )
			{
				cerr << "Warning: Attempted to use Complexity as fitness func without recording brain anatomy.  Turning on RecordBrainAnatomyAll." nl;
				fBrainAnatomyRecordAll = true;				
			}
		}
		
		if( version >= 37 )
		{
			PROP( RecordGenomes );
		}
		else
		{
			fRecordGenomes = false;
		}

		if( version >= 38 )
		{
			PROP( RecordSeparations );
		}
		else
		{
			fRecordSeparations = false;
		}

		if( version >= 20 )
		{
			in >> fRecordAdamiComplexity; in >> label;
			cout << "recordAdamiComplexity" ses fRecordAdamiComplexity nl;
			
			in >> fAdamiComplexityRecordFrequency; in >> label;
			cout << "AdamiComplexityRecordFrequency" ses fAdamiComplexityRecordFrequency nl;
		}
		
	}

	if( version >= 54 )
	{
		bool compress;
		__PROP( "compressFiles", compress );

		globals::recordFileType = compress ? AbstractFile::TYPE_GZIP_FILE : AbstractFile::TYPE_FILE;
	}
	else
	{
		globals::recordFileType = AbstractFile::TYPE_FILE;
	}

	if( version >= 18 )
	{
		in >> fFogFunction; in >> label;
		
		if( toupper(fFogFunction) == 'E' )			//Exponential
			fFogFunction = 'E';
		else if( toupper(fFogFunction) == 'L' )			//Linear
			fFogFunction = 'L';
		else
			fFogFunction = 'O';				// No Fog (OFF)

		assert( (fFogFunction == 'O' || fFogFunction == 'E' || fFogFunction == 'L') );
		cout << "FogFunction" ses fFogFunction nl;
		assert( glFogFunction() == fFogFunction );
			
	
		// This value only does something if Fog Function is exponential
		// Acceptable values are between 0 and 1 (inclusive)
		in >> fExpFogDensity; in >> label;
		cout << "ExpFogDensity" ses fExpFogDensity nl;

		// This value only does something if Fog Function is linear
		// It defines the maximum distance a agent can see.
		in >> fLinearFogEnd; in >> label;
		cout << "LinearFogEnd" ses fLinearFogEnd nl;

	}	
	
	
	in >> fRecordMovie; in >> label;
	cout << "recordMovie" ses fRecordMovie nl;
	
	// If this is a lockstep run, then we need to force certain parameter values (and warn the user)
	if( fLockStepWithBirthsDeathsLog )
	{
		genome::gMinLifeSpan = fMaxSteps;
		genome::gMaxLifeSpan = fMaxSteps;
		
		agent::gEat2Energy = 0.0;
		agent::gMate2Energy = 0.0;
		agent::gFight2Energy = 0.0;
		agent::gMaxSizePenalty = 0.0;
		agent::gSpeed2Energy = 0.0;
		agent::gYaw2Energy = 0.0;
		agent::gLight2Energy = 0.0;
		agent::gFocus2Energy = 0.0;
		agent::gPickup2Energy = 0.0;
		agent::gDrop2Energy = 0.0;
		agent::gCarryAgent2Energy = 0.0;
		agent::gCarryAgentSize2Energy = 0.0;
		food::gCarryFood2Energy = 0.0;
		brick::gCarryBrick2Energy = 0.0;
		agent::gFixedEnergyDrain = 0.0;

		agent::gNumDepletionSteps = 0;
		agent::gMaxPopulationPenaltyFraction = 0.0;
		
		fApplyLowPopulationAdvantage = false;
		
		cout << "Due to running in LockStepWithBirthsDeathsLog mode, the following parameter values have been forcibly reset as indicated:" nl;
		cout << "  MinLifeSpan" ses genome::gMinLifeSpan nl;
		cout << "  MaxLifeSpan" ses genome::gMaxLifeSpan nl;
		cout << "  Eat2Energy" ses agent::gEat2Energy nl;
		cout << "  Mate2Energy" ses agent::gMate2Energy nl;
		cout << "  Fight2Energy" ses agent::gFight2Energy nl;
		cout << "  MaxSizePenalty" ses agent::gMaxSizePenalty nl;
		cout << "  Speed2Energy" ses agent::gSpeed2Energy nl;
		cout << "  Yaw2Energy" ses agent::gYaw2Energy nl;
		cout << "  Light2Energy" ses agent::gLight2Energy nl;
		cout << "  Focus2Energy" ses agent::gFocus2Energy nl;
		cout << "  Pickup2Energy" ses agent::gPickup2Energy nl;
		cout << "  Drop2Energy" ses agent::gDrop2Energy nl;
		cout << "  CarryAgent2Energy" ses agent::gCarryAgent2Energy nl;
		cout << "  CarryAgentSize2Energy" ses agent::gCarryAgentSize2Energy nl;
		cout << "  CarryFood2Energy" ses food::gCarryFood2Energy nl;
		cout << "  CarryBrick2Energy" ses brick::gCarryBrick2Energy nl;
		cout << "  FixedEnergyDrain" ses agent::gFixedEnergyDrain nl;
		cout << "  NumDepletionSteps" ses agent::gNumDepletionSteps nl;
		cout << "  .MaxPopulationPenaltyFraction" ses agent::gMaxPopulationPenaltyFraction nl;
		cout << "  ApplyLowPopulationAdvantage" ses fApplyLowPopulationAdvantage nl;
	}
	
	// If this is a steady-state GA run, then we need to force certain parameter values (and warn the user)
	if( (fHeuristicFitnessWeight != 0.0) || (fComplexityFitnessWeight != 0) )
	{
		fNumberToSeed = lrint( fMaxNumAgents * (float) fNumberToSeed / fInitNumAgents );	// same proportion as originally specified (must calculate before changing fInitNumAgents)
		if( fNumberToSeed > fMaxNumAgents )	// just to be safe
			fNumberToSeed = fMaxNumAgents;
		fInitNumAgents = fMaxNumAgents;	// population starts at maximum
		fMinNumAgents = fMaxNumAgents;		// population stays at mximum
		fProbabilityOfMutatingSeeds = 1.0;	// so there is variation in the initial population
//		fMateThreshold = 1.5;				// so they can't reproduce on their own

		for( int i = 0; i < fNumDomains; i++ )	// over all domains
		{
			fDomains[i].numberToSeed = lrint( fDomains[i].maxNumAgents * (float) fDomains[i].numberToSeed / fDomains[i].initNumAgents );	// same proportion as originally specified (must calculate before changing fInitNumAgents)
			if( fDomains[i].numberToSeed > fDomains[i].maxNumAgents )	// just to be safe
				fDomains[i].numberToSeed = fDomains[i].maxNumAgents;
			fDomains[i].initNumAgents = fDomains[i].maxNumAgents;	// population starts at maximum
			fDomains[i].minNumAgents  = fDomains[i].maxNumAgents;	// population stays at maximum
			fDomains[i].probabilityOfMutatingSeeds = 1.0;				// so there is variation in the initial population
		}

		agent::gNumDepletionSteps = 0;				// turn off the high-population penalty
		agent::gMaxPopulationPenaltyFraction = 0.0;	// ditto
		fApplyLowPopulationAdvantage = false;			// turn off the low-population advantage
		
		cout << "Due to running as a steady-state GA with a fitness function, the following parameter values have been forcibly reset as indicated:" nl;
		cout << "  InitNumAgents" ses fInitNumAgents nl;
		cout << "  MinNumAgents" ses fMinNumAgents nl;
		cout << "  NumberToSeed" ses fNumberToSeed nl;
		cout << "  ProbabilityOfMutatingSeeds" ses fProbabilityOfMutatingSeeds nl;
//		cout << "  MateThreshold" ses fMateThreshold nl;
		for( int i = 0; i < fNumDomains; i++ )
		{
			cout << "  Domain " << i << ":" nl;
			cout << "    initNumAgents" ses fDomains[i].initNumAgents nl;
			cout << "    minNumAgents" ses fDomains[i].minNumAgents nl;
			cout << "    numberToSeed" ses fDomains[i].numberToSeed nl;
			cout << "    probabilityOfMutatingSeeds" ses fDomains[i].probabilityOfMutatingSeeds nl;
		}
		cout << "  NumDepletionSteps" ses agent::gNumDepletionSteps nl;
		cout << "  .MaxPopulationPenaltyFraction" ses agent::gMaxPopulationPenaltyFraction nl;
		cout << "  ApplyLowPopulationAdvantage" ses fApplyLowPopulationAdvantage nl;
		
		if( fComplexityFitnessWeight != 0.0 )
		{
			cout << "Due to running with Complexity as a fitness function, the following parameter values have been forcibly reset as indicated:" nl;
			fRecordComplexity = true;						// record it, since we have to compute it
			cout << "  RecordComplexity" ses fRecordComplexity nl;
		}
	}

	cout nlf;
    fb.close();
	return;

#undef PROP
}


//-------------------------------------------------------------------------------------------
// TSimulation::ReadLabel
//-------------------------------------------------------------------------------------------
void TSimulation::ReadLabel(istream &in, const char *name)
{
	string label;
	
	in >> label;
	
	size_t i = label.find('_');
	if( i != string::npos )
	{
		label = label.substr( 0, i );
	}

	if(0 != strcasecmp(name, label.c_str()))
	{
	  error(2, "expecting '", name, "' found '", label.c_str(), "'");
	}
}
/********************************************************************************
 ********************************************************************************
 ***
 *** END LEGACY WORLDFILE CODE
 ***
 ********************************************************************************
 ********************************************************************************/
#endif // if 0 -- old worldfile code
