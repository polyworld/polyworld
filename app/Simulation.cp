#define DebugMaxFitness 0
#define TEXTTRACE 0
#define DebugSmite 0

// Self
#include "Simulation.h"

// System
#include <fstream>
#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>

// qt
#include <qapplication.h>
#include <QDesktopWidget.h>

// Local
#include "barrier.h"
#include "brain.h"
#include "BrainMonitorWindow.h"
#include "ChartWindow.h"
#include "CritterPOVWindow.h"
#include "debug.h"
#include "food.h"
#include "globals.h"
#include "food.h"
#include "SceneView.h"
#include "TextStatusWindow.h"
#include "PwMovieTools.h"

using namespace std;

//===========================================================================
// TSimulation
//===========================================================================

static const long kMaxLoops = 1000000;
static long numglobalcreated = 0;    // needs to be static so we only get warned about influece of global creations once ever

long TSimulation::fMaxCritters;
long TSimulation::fStep;
short TSimulation::fOverHeadRank = 1;
critter* TSimulation::fMonitorCritter = NULL;
double TSimulation::fFramesPerSecondOverall;
double TSimulation::fSecondsPerFrameOverall;
double TSimulation::fFramesPerSecondRecent;
double TSimulation::fSecondsPerFrameRecent;
double TSimulation::fFramesPerSecondInstantaneous;
double TSimulation::fSecondsPerFrameInstantaneous;
double TSimulation::fTimeStart;

//---------------------------------------------------------------------------
// Macros
//---------------------------------------------------------------------------

#if TEXTTRACE
	#define ttPrint( x... ) printf( x )
#else
	#define ttPrint( x... )
#endif

#if DebugSmite
	#define smPrint( x... ) printf( x )
#else
	#define smPrint( x... )
#endif

#define DebugLinksEtAl 0
#if DebugLinksEtAl
	#define link( x... ) ( printf( "link %s to %s (%s/%d)\n", s, t, __FUNCTION__, __LINE__ ), link( x ) )
	#define unlink( x... ) ( printf( "unlink %s (%s/%d)\n", s, __FUNCTION__, __LINE__ ), unlink( x ) )
	#define rename( x... ) ( printf( "rename %s to %s (%s/%d)\n", s, t, __FUNCTION__, __LINE__ ), rename( x ) )
#endif

//---------------------------------------------------------------------------
// TSimulation::TSimulation
//---------------------------------------------------------------------------
TSimulation::TSimulation( TSceneView* sceneView, TSceneWindow* sceneWindow )
	:	fBestSoFarBrainAnatomyRecordFrequency(0),
		fBestSoFarBrainFunctionRecordFrequency(0),
		fBestRecentBrainAnatomyRecordFrequency(0),
		fBestRecentBrainFunctionRecordFrequency(0),
		fBrainAnatomyRecordAll(false),
		fBrainFunctionRecordAll(false),
		fBrainAnatomyRecordSeeds(false),
		fBrainFunctionRecordSeeds(false),
		fSceneView(sceneView),
		fSceneWindow(sceneWindow),
		fBirthrateWindow(NULL),
		fFitnessWindow(NULL),
		fFoodEnergyWindow(NULL),
		fPopulationWindow(NULL),
		fBrainMonitorWindow(NULL),
		fGeneSeparationWindow(NULL),
		fPaused(false),
		fDelay(0),
		fDumpFrequency(500),
		fStatusFrequency(100),
		fLoadState(false),
		inited(false),
		fMonitorCritterRank(0),
		fMonitorCritterRankOld(0),
//		fMonitorCritter(NULL),
		fCritterTracking(false),
		fGroundClearance(0.0),
//		fOverHeadRank(0),
		fOverHeadRankOld(0),
		fOverheadCritter(NULL),
		fChartBorn(true),
		fChartFitness(true),
		fChartFoodEnergy(true),
		fChartPopulation(true),
//		fShowBrain(false),
		fShowTextStatus(true),
		fRecordGeneStats(false),
		fRecordFoodBandStats(false),
		fNewDeaths(0),
		fNumberFit(0),
		fFittest(NULL),
		fNumberRecentFit(0),
		fRecentFittest(NULL),
		fBrainMonitorStride(25),
		fGeneSum(NULL),
		fGeneSum2(NULL),
		fGeneStatsFile(NULL),
		fNumCrittersNotInOrNearAnyFoodBand(0),
		fNumCrittersInFoodBand(NULL),
		fNumCrittersWithin5UnitsOfFoodBand(NULL),
		fNumCrittersWithin10UnitsOfFoodBand(NULL)
{
	Init();
}


//---------------------------------------------------------------------------
// TSimulation::~TSimulation
//---------------------------------------------------------------------------
TSimulation::~TSimulation()
{
	Stop();
	
	if( fGeneSum )
		free( fGeneSum );

	if( fGeneSum2 )
		free( fGeneSum2 );
		
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

	if (fCritterPOVWindow != NULL)
		delete fCritterPOVWindow;

	if (fTextStatusWindow != NULL)
		delete fTextStatusWindow;
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
	// delete all food
	food* f = NULL;
	food::gXSortedFood.reset();
	while (food::gXSortedFood.next(f))
		delete f;
	food::gXSortedFood.clear();

	// delete all barriers
	barrier* b = NULL;
	barrier::gXSortedBarriers.reset();
	while (barrier::gXSortedBarriers.next(b))
		delete b;
	barrier::gXSortedBarriers.clear();
	
	// delete all critters
	// all the critters are deleted in critterdestruct
	// rather than cycling through them in xsortedcritters here
	critter::gXSortedCritters.clear();
	
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
	
	if( fLeastFit )
		delete fLeastFit;
    
    genome::genomedestruct();
    
    brain::braindestruct();
    
    critter::critterdestruct();
}


//---------------------------------------------------------------------------
// TSimulation::Step
//---------------------------------------------------------------------------
void TSimulation::Step()
{
	if( !inited )
	{
		printf( "%s: called before TSimulation::Init()\n", __FUNCTION__ );
		return;
	}
	
	static unsigned long frame = 0;
	frame++;
//	printf( "%s: frame = %lu\n", __FUNCTION__, frame );
	
	#define RecentSteps 10
	static double	sTimePrevious[RecentSteps];
	double			timeNow;
	
	fStep++;
	
	if (fStep > kMaxLoops)
	{
		// Stop simulation
		ttPrint( "Simulation stopped at age %ld\n", fStep );
		Stop(); // will set fDone = true
		return;
	}
		
#ifdef DEBUGCHECK
	char debugstring[256];
	sprintf(debugstring, "in main loop at age %ld", fStep);
	debugcheck(debugstring);
#endif

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

#ifdef DEBUGCHECK
	long critnum = 0;
#endif

	// Update all critters, using their neurally controlled behaviors
	{
		// Make the critter POV window the current GL context
		fCritterPOVWindow->makeCurrent();
		
		// Clear the window's color and depth buffers
		fCritterPOVWindow->qglClearColor( Qt::black );
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		
		critter* c;
		critter::gXSortedCritters.reset();
		while (critter::gXSortedCritters.next(c))
		{
		#ifdef DEBUGCHECK
			debugstring[256];
			sprintf(debugstring,"in critter loop at age %ld, critnum = %ld", fStep, critnum);
			debugcheck(debugstring);
			critnum++;
		#endif DEBUGCHECK
			fFoodEnergyOut += c->Update(fMoveFitnessParameter, critter::gSpeed2DPosition);
		}
		
		// Swap buffers for the critter POV window when they're all done
		fCritterPOVWindow->swapBuffers();
	}

					
	long oldNumBorn = fNumberBorn;
	long oldNumCreated = fNumberCreated;
	
//  if( fDoCPUWork )
	{
		// Handles collisions, matings, fights, deaths, births, etc
		Interact();
	}
		
#ifdef DEBUGCHECK
	debugstring[256];
	sprintf(debugstring, "after interact at age %ld", fStep);
	debugcheck(debugstring);
#endif DEBUGCHECK

	fTotalFoodEnergyIn += fFoodEnergyIn;
	fTotalFoodEnergyOut += fFoodEnergyOut;

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
			if (!fCritterTracking
				|| (fOverHeadRank != fOverHeadRankOld)
				|| !fOverheadCritter
				|| !(fOverheadCritter->Alive()))
			{
				fOverheadCritter = fCurrentFittestCritter[fOverHeadRank - 1];
				fOverHeadRankOld = fOverHeadRank;
			}
			
			if (fOverheadCritter != NULL)
			{
				//camera[1].setx(fOverheadCritter->x());
				//camera[1].setz(fOverheadCritter->z()); TODO
			}
		}	
//		fOverHeadWindow->updateGL(); TODO
		
		// Born / (Born + Created) window
		if (fChartBorn
			&& fBirthrateWindow != NULL
		/*  && fBirthrateWindow->isVisible() */
			&& ((oldNumBorn != fNumberBorn) || (oldNumCreated != fNumberCreated)))
		{
			fBirthrateWindow->AddPoint(float(fNumberBorn) / float(fNumberBorn + fNumberCreated));
		}

		// Fitness window
		if( fChartFitness && fFitnessWindow != NULL /* && fFitnessWindow->isVisible() */ )
		{
			fFitnessWindow->AddPoint(0, fMaxFitness / fTotalFitness);
			fFitnessWindow->AddPoint(1, fCurrentMaxFitness[0] / fTotalFitness);
			fFitnessWindow->AddPoint(2, fAverageFitness / fTotalFitness);
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
				fPopulationWindow->AddPoint(0, float(critter::gXSortedCritters.count()));
			}				
			
			if (fNumDomains > 1)
			{
				for (int id = 0; id < fNumDomains; id++)
					fPopulationWindow->AddPoint((id + 1), float(fDomains[id].numcritters));
			}
		}

//		dbprintf( "age=%ld, rank=%d, rankOld=%d, tracking=%s, fittest=%08lx, monitored=%08lx, alive=%s\n",
//				  fStep, fMonitorCritterRank, fMonitorCritterRankOld, BoolString( fCritterTracking ), (ulong) fCurrentFittestCritter[fMonitorCritterRank-1], (ulong) fMonitorCritter, BoolString( !(!fMonitorCritter || !fMonitorCritter->Alive()) ) );

		// Brain window
		if ((fMonitorCritterRank != fMonitorCritterRankOld)
			 || (fMonitorCritterRank && !fCritterTracking && (fCurrentFittestCritter[fMonitorCritterRank - 1] != fMonitorCritter))
			 || (fMonitorCritterRank && fCritterTracking && (!fMonitorCritter || !fMonitorCritter->Alive())))
		{			
			if (fMonitorCritter != NULL)
			{
				if (fBrainMonitorWindow != NULL)
					fBrainMonitorWindow->StopMonitoring();
			}					
							
			if (fMonitorCritterRank && fBrainMonitorWindow != NULL && fBrainMonitorWindow->isVisible() )
			{
				Q_CHECK_PTR(fBrainMonitorWindow);
				fMonitorCritter = fCurrentFittestCritter[fMonitorCritterRank - 1];
				fBrainMonitorWindow->StartMonitoring(fMonitorCritter);					
			}
			else
			{
				fMonitorCritter = NULL;
			}
		
			fMonitorCritterRankOld = fMonitorCritterRank;			
		}
		if( fBrainMonitorWindow && fBrainMonitorWindow->isVisible() && fMonitorCritter && (fStep % fBrainMonitorStride == 0) )
		{
			char title[64];
			sprintf( title, "Brain Monitor (%ld:%ld)", fMonitorCritterRank, fMonitorCritter->Number() );
			fBrainMonitorWindow->setWindowTitle( QString(title) );
			fBrainMonitorWindow->Draw();
		}
	}

#ifdef DEBUGCHECK
	debugstring[256];
	sprintf(debugstring,"after extra graphics at age %ld", fStep);
	debugcheck(debugstring);
#endif DEBUGCHECK

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
			sprintf( s, "run/brain/anatomy/brainAnatomy_%ld_incept.txt", fFittest[i]->critterID );
			sprintf( t, "run/brain/bestSoFar/%ld/%d_brainAnatomy_%ld_incept.txt", fStep, i, fFittest[i]->critterID );
			if( link( s, t ) )
				eprintf( "Error (%d) linking from \"%s\" to \"%s\"\n", errno, s, t );
			sprintf( s, "run/brain/anatomy/brainAnatomy_%ld_birth.txt", fFittest[i]->critterID );
			sprintf( t, "run/brain/bestSoFar/%ld/%d_brainAnatomy_%ld_birth.txt", fStep, i, fFittest[i]->critterID );
			if( link( s, t ) )
				eprintf( "Error (%d) linking from \"%s\" to \"%s\"\n", errno, s, t );
			sprintf( s, "run/brain/anatomy/brainAnatomy_%ld_death.txt", fFittest[i]->critterID );
			sprintf( t, "run/brain/bestSoFar/%ld/%d_brainAnatomy_%ld_death.txt", fStep, i, fFittest[i]->critterID );
			if( link( s, t ) )
				eprintf( "Error (%d) linking from \"%s\" to \"%s\"\n", errno, s, t );
			sprintf( s, "run/brain/function/brainFunction_%ld.txt", fFittest[i]->critterID );
			sprintf( t, "run/brain/bestSoFar/%ld/%d_brainFunction_%ld.txt", fStep, i, fFittest[i]->critterID );
			if( link( s, t ) )
				eprintf( "Error (%d) linking from \"%s\" to \"%s\"\n", errno, s, t );
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
			if( fRecentFittest[i]->critterID > 0 )
			{
				char t[256];	// target (use s for source)
				sprintf( s, "run/brain/anatomy/brainAnatomy_%ld_incept.txt", fRecentFittest[i]->critterID );
				sprintf( t, "run/brain/bestRecent/%ld/%d_brainAnatomy_%ld_incept.txt", fStep, i, fRecentFittest[i]->critterID );
				if( link( s, t ) )
					eprintf( "Error (%d) linking from \"%s\" to \"%s\"\n", errno, s, t );
				sprintf( s, "run/brain/anatomy/brainAnatomy_%ld_birth.txt", fRecentFittest[i]->critterID );
				sprintf( t, "run/brain/bestRecent/%ld/%d_brainAnatomy_%ld_birth.txt", fStep, i, fRecentFittest[i]->critterID );
				if( link( s, t ) )
					eprintf( "Error (%d) linking from \"%s\" to \"%s\"\n", errno, s, t );
				sprintf( s, "run/brain/anatomy/brainAnatomy_%ld_death.txt", fRecentFittest[i]->critterID );
				sprintf( t, "run/brain/bestRecent/%ld/%d_brainAnatomy_%ld_death.txt", fStep, i, fRecentFittest[i]->critterID );
				if( link( s, t ) )
					eprintf( "Error (%d) linking from \"%s\" to \"%s\"\n", errno, s, t );
				sprintf( s, "run/brain/function/brainFunction_%ld.txt", fRecentFittest[i]->critterID );
				sprintf( t, "run/brain/bestRecent/%ld/%d_brainFunction_%ld.txt", fStep, i, fRecentFittest[i]->critterID );
				if( link( s, t ) )
					eprintf( "Error (%d) linking from \"%s\" to \"%s\"\n", errno, s, t );
			}
		}
		
		// Now delete all bestRecent critter files, unless they are also on the bestSoFar list
		// Also empty the bestRecent list here, so we start over each epoch
		int limit2 = fNumberDied < fNumberFit ? fNumberDied : fNumberFit;
		for( int i = 0; i < limit; i++ )
		{
			// First determine whether or not each bestRecent critter is in the bestSoFar list or not
			bool inBestSoFarList = false;
			for( int j = 0; j < limit2; j++ )
			{
				if( fRecentFittest[i]->critterID == fFittest[j]->critterID )
				{
					inBestSoFarList = true;
					break;
				}
			}
			
			// If each bestRecent critter is NOT in the bestSoFar list, then unlink all its files from their original location
			if( !inBestSoFarList && (fRecentFittest[i]->critterID > 0) )
			{
				sprintf( s, "run/brain/anatomy/brainAnatomy_%ld_incept.txt", fRecentFittest[i]->critterID );
				if( unlink( s ) )
					eprintf( "Error (%d) unlinking \"%s\"\n", errno, s );
				sprintf( s, "run/brain/anatomy/brainAnatomy_%ld_birth.txt", fRecentFittest[i]->critterID );
				if( unlink( s ) )
					eprintf( "Error (%d) unlinking \"%s\"\n", errno, s );
				sprintf( s, "run/brain/anatomy/brainAnatomy_%ld_death.txt", fRecentFittest[i]->critterID );
				if( unlink( s ) )
					eprintf( "Error (%d) unlinking \"%s\"\n", errno, s );
				sprintf( s, "run/brain/function/brainFunction_%ld.txt", fRecentFittest[i]->critterID );
				if( unlink( s ) )
					eprintf( "Error (%d) unlinking \"%s\"\n", errno, s );
			}
			
			// Empty the bestRecent list by zeroing out critter IDs and fitnesses
			fRecentFittest[i]->critterID = 0;
			fRecentFittest[i]->fitness = 0.0;
		}
	}
	
	// Handle tracking gene Separation
	if (fMonitorGeneSeparation && fRecordGeneSeparation)
		RecordGeneSeparation();
}


//---------------------------------------------------------------------------
// TSimulation::Init
//
// The order that initializer functions are called is important as values
// may depend on a variable initialized earlier.
//---------------------------------------------------------------------------
void TSimulation::Init()
{
 	// Set up graphical constructs
    "ground.obj" >> fGround;
    
    srand(1);
    
    // Initialize world with default values
    InitWorld();
	
	// Initialize world state from saved file if present
	ReadWorldFile("worldfile");
	     	
	InitNeuralValues();	 // Must be called before genome and brain init
	
	genome::genomeinit();
    brain::braininit();
    critter::critterinit();
	
	 // Following is part of one way to speed up the graphics
	 // Note:  this code must agree with the critter sizing in critter::grow()
	 // and the food sizing in food::initlen().

	float maxcritterlenx = critter::gMaxCritterSize / sqrt(genome::gMinmaxspeed);
	float maxcritterlenz = critter::gMaxCritterSize * sqrt(genome::gMaxmaxspeed);
	float maxcritterradius = 0.5 * sqrt(maxcritterlenx*maxcritterlenx + maxcritterlenz*maxcritterlenz);
	float maxfoodlen = 0.75 * food::gMaxFoodEnergy / food::gSize2Energy;
	float maxfoodradius = 0.5 * sqrt(maxfoodlen * maxfoodlen * 2.0);
	critter::gMaxRadius = maxcritterradius > maxfoodradius ?
						  maxcritterradius : maxfoodradius;
    
    InitMonitoringWindows();
        
    critter* c = NULL;
    
    if (fNumberFit > 0)
    {
        fFittest = new FitStruct*[fNumberFit];
		Q_CHECK_PTR( fFittest );
        
        for (int i = 0; i < fNumberFit; i++)
        {
			fFittest[i] = new FitStruct;
			Q_CHECK_PTR( fFittest[i] );
            fFittest[i]->genes = new genome();
			Q_CHECK_PTR( fFittest[i]->genes );
            fFittest[i]->fitness = 0.0;
			fFittest[i]->critterID = 0;
        }
		
        fRecentFittest = new FitStruct*[fNumberRecentFit];
		Q_CHECK_PTR( fRecentFittest );
        
        for (int i = 0; i < fNumberRecentFit; i++)
        {
			fRecentFittest[i] = new FitStruct;
			Q_CHECK_PTR( fRecentFittest[i] );
            fRecentFittest[i]->genes = NULL;	// new genome();	// we don't save the genes in the bestRecent list
            fRecentFittest[i]->fitness = 0.0;
			fRecentFittest[i]->critterID = 0;
        }
		
        for( int id = 0; id < fNumDomains; id++ )
        {
            fDomains[id].fittest = new FitStruct*[fNumberFit];
			Q_CHECK_PTR( fDomains[id].fittest );
            
            for (int i = 0; i < fNumberFit; i++)
            {
				fDomains[id].fittest[i] = new FitStruct;
				Q_CHECK_PTR( fDomains[id].fittest[i] );
                fDomains[id].fittest[i]->genes = new genome();
				Q_CHECK_PTR( fDomains[id].fittest[i]->genes );
                fDomains[id].fittest[i]->fitness = 0.0;
				fDomains[id].fittest[i]->critterID = 0;
            }
        }
    }

	if( fSmiteFrac > 0.0 )
	{
        for( int id = 0; id < fNumDomains; id++ )
        {
			fDomains[id].fNumLeastFit = 0;
			fDomains[id].fMaxNumLeastFit = lround( fSmiteFrac * fDomains[id].maxnumcritters );
			
			smPrint( "for domain %d fMaxNumLeastFit = %d\n", id, fDomains[id].fMaxNumLeastFit );
			
			if( fDomains[id].fMaxNumLeastFit > 0 )
			{
				fDomains[id].fLeastFit = new critter*[fDomains[id].fMaxNumLeastFit];
				
				for( int i = 0; i < fDomains[id].fMaxNumLeastFit; i++ )
					fDomains[id].fLeastFit[i] = NULL;
			}
        }
	}

	// Set up the run directory and its subsidiaries
	char s[256];
	char t[256];
	// First save the old directory, if it exists
	sprintf( s, "run" );
	sprintf( t, "run_%ld", time(NULL) );
	(void) rename( s, t );
	if( mkdir( "run", PwDirMode ) )
		eprintf( "Error making run directory (%d)\n", errno );
	if( mkdir( "run/stats", PwDirMode ) )
		eprintf( "Error making run/stats directory (%d)\n", errno );
	if( fBestSoFarBrainAnatomyRecordFrequency || fBestSoFarBrainFunctionRecordFrequency ||
		fBestRecentBrainAnatomyRecordFrequency || fBestRecentBrainFunctionRecordFrequency ||
		fBrainAnatomyRecordAll || fBrainFunctionRecordAll ||
		fBrainAnatomyRecordSeeds || fBrainFunctionRecordSeeds )
	{
		// If we're going to be saving info on all these files, must increase the number allowed open
		if( SetMaximumFiles( fMaxCritters * 2 ) )	// 2x is overkill, but let's be safe
			eprintf( "Error setting maximum files to %d (%d)\n", fMaxCritters * 2, errno );

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
		if( fBestSoFarBrainFunctionRecordFrequency || fBestRecentBrainFunctionRecordFrequency || fBrainFunctionRecordAll || fBrainFunctionRecordSeeds )
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
		// If we're going to record the gene means and std devs, we need to allocate a couple of stat arrays
		if( fRecordGeneStats )
		{
			fGeneSum  = (unsigned long*) malloc( sizeof( *fGeneSum  ) * genome::gNumBytes );
			Q_CHECK_PTR( fGeneSum );
			fGeneSum2 = (unsigned long*) malloc( sizeof( *fGeneSum2 ) * genome::gNumBytes );
			Q_CHECK_PTR( fGeneSum2 );
			
			fGeneStatsFile = fopen( "run/genestats.txt", "w" );
			Q_CHECK_PTR( fGeneStatsFile );
			
			fprintf( fGeneStatsFile, "%ld\n", genome::gNumBytes );
		}
	}

	//If we're recording the number of critters in or near various foodbands make some stat arrays.
	if( fRecordFoodBandStats && (food::gNumFoodBands > 0) )
	{
			fNumCrittersInFoodBand = (unsigned long*) malloc( sizeof(unsigned long) * food::gNumFoodBands);
			Q_CHECK_PTR( fNumCrittersInFoodBand );
			fNumCrittersWithin5UnitsOfFoodBand = (unsigned long*) malloc( sizeof(unsigned long) * food::gNumFoodBands);;
			Q_CHECK_PTR( fNumCrittersWithin5UnitsOfFoodBand );
			fNumCrittersWithin10UnitsOfFoodBand = (unsigned long*) malloc( sizeof(unsigned long) * food::gNumFoodBands);;
			Q_CHECK_PTR( fNumCrittersWithin10UnitsOfFoodBand );
	}
	
	system( "cp worldfile run/" );

    // Pass ownership of the cast to the stage [TODO] figure out ownership issues
    fStage.SetCast(&fWorldCast);

	fFoodEnergyIn = 0.0;
	fFoodEnergyOut = 0.0;

	srand48(fGenomeSeed);

	if (!fLoadState)
	{
		long numSeededDomain;
		long numSeededTotal = 0;
		int id;
		
		// first handle the individual domains
		for (id = 0; id < fNumDomains; id++)
		{
			numSeededDomain = 0;	// reset for each domain

			int limit = min((fMaxCritters - (long)critter::gXSortedCritters.count()), fDomains[id].initnumcritters);
			for (int i = 0; i < limit; i++)
			{
				c = critter::getfreecritter(this, &fStage);
				Q_ASSERT(c != NULL);
				
				fNumberCreated++;
				fNumberCreatedRandom++;
				fDomains[id].numcreated++;
				
				if( numSeededDomain < fDomains[id].numberToSeed )
				{
					c->Genes()->SeedGenes();
					if( drand48() < fDomains[id].probabilityOfMutatingSeeds )
						c->Genes()->Mutate();
					numSeededDomain++;
				}
				else
					c->Genes()->Randomize();
				c->grow();
				
				fFoodEnergyIn += c->FoodEnergy();
				fStage.AddObject(c);
				
				float x = drand48() * (fDomains[id].xsize - 0.02) + fDomains[id].xleft + 0.01;
				float z = -0.01 - drand48() * (globals::worldsize - 0.02);
				float y = 0.5 * critter::gCritterHeight;
			#if TestWorld
				// evenly distribute the critters
				x = fDomains[id].xleft  +  0.666 * fDomains[id].xsize;
				z = - globals::worldsize * ((float) (i+1) / (fDomains[id].initnumcritters + 1));
			#endif
				c->settranslation(x, y, z);
				
				float yaw =  360.0 * drand48();
			#if TestWorld
				// point them all the same way
				yaw = 95.0;
			#endif
				c->setyaw(yaw);
				
				critter::gXSortedCritters.add(c);	// stores c->listLink

				c->Domain(id);
				fDomains[id].numcritters++;
				fNeuronGroupCountStats.add( c->Brain()->NumNeuronGroups() );

			#if RecordRandomBrainAnatomies
				c->Brain()->dumpAnatomical( "run/brain/random", "birth", c->Number(), 0.0 );
			#endif
			}
			
			numSeededTotal += numSeededDomain;
			
			long maxNewFood = fMaxFoodCount - (long) food::gXSortedFood.count();
			for (int i = 0; i < min(maxNewFood, fDomains[id].initfoodcount); i++)
			{
				food* f = new food::food;
				Q_CHECK_PTR(f);
			#if TestWorld
				f->setenergy( 0.5 * (food::gMinFoodEnergy + food::gMaxFoodEnergy) );
			#endif
				
				fFoodEnergyIn += f->energy();
				float x = drand48() * (fDomains[id].xsize - 0.02) + fDomains[id].xleft + 0.01;
			#if TestWorld
				x = fDomains[id].xleft  +  0.333 * fDomains[id].xsize;
				float z = - globals::worldsize * ((float) (i+1) / (fDomains[id].initfoodcount + 1));
				f->setz(z);
			#endif
				f->setx(x);
				f->domain(id);
				
				fStage.AddObject(f);
				
				food::gXSortedFood.add(f);
				fDomains[id].foodcount++;
			}
		}
	
		// Handle global initial creations, if necessary
		Q_ASSERT(fInitNumCritters <= fMaxCritters);
		
		while ((int)critter::gXSortedCritters.count() < fInitNumCritters)
		{
			c = critter::getfreecritter(this, &fStage);
			Q_CHECK_PTR(c);

			fNumberCreated++;
			fNumberCreatedRandom++;
			
			if( numSeededTotal < fNumberToSeed )
			{
				c->Genes()->SeedGenes();
				if( drand48() < fProbabilityOfMutatingSeeds )
					c->Genes()->Mutate();
				numSeededTotal++;
			}
			else
				c->Genes()->Randomize();
			c->grow();
			
			fFoodEnergyIn += c->FoodEnergy();
			fStage.AddObject(c);
			
			float x =  0.01 + drand48() * (globals::worldsize - 0.02);
			float z = -0.01 - drand48() * (globals::worldsize - 0.02);
			float y = 0.5 * critter::gCritterHeight;
			c->settranslation(x, y, z);

			float yaw =  360.0 * drand48();
			c->setyaw(yaw);
			
			critter::gXSortedCritters.add(c);	// stores c->listLink
			
			id = WhichDomain(x, z, 0);
			c->Domain(id);
			fDomains[id].numcritters++;
			fNeuronGroupCountStats.add( c->Brain()->NumNeuronGroups() );
		}
			
		while ((int)food::gXSortedFood.count() < fInitialFoodCount)
		{
			food* f = new food::food;
			Q_CHECK_PTR(f);
			
			fFoodEnergyIn += f->energy();
			fStage.AddObject(f);
			food::gXSortedFood.add(f);
			
			id = WhichDomain(f->x(), f->z(), 0);
			f->domain(id);
			fDomains[id].foodcount++;
		}
	}

    fTotalFoodEnergyIn = fFoodEnergyIn;
    fTotalFoodEnergyOut = fFoodEnergyOut;
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

	fCamera.SetPerspective(fCameraFOV,
						   fSceneView->width() / fSceneView->height(),
						   .01,
						   1.5 * globals::worldsize);
						   
    const float cameraRadius = fCameraAngleStart * DEGTORAD;
    fCamera.settranslation((0.5 + fCameraRadius * sin(cameraRadius)) * globals::worldsize,
                            fCameraHeight * globals::worldsize,
						    (-0.5 + fCameraRadius * cos(cameraRadius)) * globals::worldsize);

    if (fCameraRotationRate == 0.0)
		fCamera.SetFixationPoint(0.5 * globals::worldsize, 0.0, -0.5 * globals::worldsize);
    else
        fCamera.SetRotation(0.0, -fCameraFOV / 3.0, 0.0);

    fCamera.setcolor(fCameraColor);
    fStage.AddObject(&fCamera);

	
	// Add scene to scene view
	Q_CHECK_PTR(fSceneView);		
	fSceneView->SetScene(&fScene);
	
	// TODO add overhead view here

#define DebugGenetics 0
#if DebugGenetics
	// This little snippet of code confirms that genetic copying, crossover, and mutation are behaving somewhat reasonably
	critter::gXSortedCritters.reset();
	critter* c1 = NULL;
	critter* c2 = NULL;
	genome* g1 = NULL;
	genome* g2 = NULL;
	genome g1c, g1x2, g1x2m;
	critter::gXSortedCritters.next( c1 );
	critter::gXSortedCritters.next( c2 );
	g1 = c1->Genes();
	g2 = c2->Genes();
	cout << "************** G1 **************" nl;
	g1->Print();
	cout << "************** G2 **************" nl;
	g2->Print();
	g1c.CopyGenes( g1 );
	g1x2.Crossover( g1, g2, false );
	g1x2m.Crossover( g1, g2, true );
	cout << "************** G1c **************" nl;
	g1c.Print();
	cout << "************** G1x2 **************" nl;
	g1x2.Print();
	cout << "************** G1x2m **************" nl;
	g1x2m.Print();
	exit(0);
#endif
	
	// Set up gene Separation monitoring
	if (fMonitorGeneSeparation)
    {
		fGeneSepVals = new float[fMaxCritters * (fMaxCritters - 1) / 2];
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
				
		fMovieFile = fopen( movieFileName, "wb" );
		if( !fMovieFile )
		{
			eprintf( "Error opening movie file: %s\n", movieFileName );
			exit( 1 );
		}
		
		fSceneView->setRecordMovie( fRecordMovie, fMovieFile );
	}
	
	inited = true;
}


//---------------------------------------------------------------------------
// TSimulation::InitNeuralValues
//
// Calculate neural values based either defalt neural values or persistant
// values that were loaded prior to this call.
//---------------------------------------------------------------------------
void TSimulation::InitNeuralValues()
{
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
	globals::edges = true;
	
    fMinNumCritters = 15;
    fInitNumCritters = 15;
	fNumberToSeed = 0;
	fProbabilityOfMutatingSeeds = 0.0;
    fMinFoodCount = 15;
    fMaxFoodCount = 50;
    fMaxFoodGrown = 25;
    fInitialFoodCount = 25;
    fFoodRate = 0.1;
    fMiscCritters = 150;
	fPositionSeed = 42;
    fGenomeSeed = 42;
    fCrittersRfood = true;
    fFitness1Frequency = 100;
    fFitness2Frequency = 2;
	fEat2Consume = 20.0;
    fNumberFit = 10;
	fNumberRecentFit = 10;
    fEatFitnessParameter = 10.0;
    fMateFitnessParameter = 10.0;
    fMoveFitnessParameter = 1.0;
    fEnergyFitnessParameter = 1.0;
    fAgeFitnessParameter = 1.0;
 	fTotalFitness = fEatFitnessParameter
					+ fMateFitnessParameter
					+ fMoveFitnessParameter
					+ fEnergyFitnessParameter
					+ fAgeFitnessParameter;
    fMateWait = 25;
    fMinMateFraction = 0.05;
    fPower2Energy = 2.5;
	fEatThreshold = 0.2;
    fMateThreshold = 0.5;
    fFightThreshold = 0.2;
  	fGroundColor.r = 0.1;
    fGroundColor.g = 0.15;
    fGroundColor.b = 0.05;
    fCameraColor.r = 1.0;
    fCameraColor.g = 1.0;
    fCameraColor.b = 1.0;
    fCameraRadius = 0.6;
    fCameraHeight = 0.35;
    fCameraRotationRate = 0.09;
    fCameraAngleStart = 0.0;
    fCameraFOV = 90.0;
	fMonitorCritterRank = 1;
	fMonitorCritterRankOld = 0;
	fMonitorCritter = NULL;
	fMonitorGeneSeparation = false;
    fRecordGeneSeparation = false;
    
    fNumberCreated = 0;
    fNumberCreatedRandom = 0;
    fNumberCreated1Fit = 0;
    fNumberCreated2Fit = 0;
    fNumberBorn = 0;
    fNumberDied = 0;
    fNumberDiedAge = 0;
    fNumberDiedEnergy = 0;
    fNumberDiedFight = 0;
    fNumberDiedEdge = 0;
	fNumberDiedSmite = 0;
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
    fSmiteFrac = 0.10;
	fSmiteAgeFrac = 0.25;
    fShowVision = true;
	fRecordMovie = false;
	fMovieFile = NULL;
    
    fFitI = 0;
    fFitJ = 1;
		
	fStep = 0;
	globals::worldsize = 100.0;

    brain::gMinWin = 22;
    brain::gDecayRate = 0.9;
    brain::gLogisticsSlope = 0.5;
    brain::gMaxWeight = 1.0;
    brain::gInitMaxWeight = 0.5;
	brain::gNumPrebirthCycles = 25;
 	brain::gNeuralValues.mininternalneurgroups = 1;
    brain::gNeuralValues.maxinternalneurgroups = 5;
    brain::gNeuralValues.mineneurpergroup = 1;
    brain::gNeuralValues.maxeneurpergroup = 8;
    brain::gNeuralValues.minineurpergroup = 1;
    brain::gNeuralValues.maxineurpergroup = 8;
	brain::gNeuralValues.numoutneurgroups = 7;
    brain::gNeuralValues.numinputneurgroups = 5;
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

	fGraphics = true;
    critter::gVision = true;
    critter::gMaxVelocity = 1.0;
    fMaxCritters = 50;	
    critter::gInitMateWait = 25;
    critter::gMinCritterSize = 1.0;
    critter::gMinCritterSize = 4.0;
    critter::gMinMaxEnergy = 500.0;
    critter::gMaxMaxEnergy = 1000.0;
    critter::gSpeed2DPosition = 1.0;
    critter::gYaw2DYaw = 1.0;
    critter::gMinFocus = 20.0;
    critter::gMaxFocus = 140.0;
    critter::gCritterFOV = 10.0;
    critter::gMaxSizeAdvantage = 2.5;
    critter::gEat2Energy = 0.01;
    critter::gMate2Energy = 0.1;
    critter::gFight2Energy = 1.0;
    critter::gMaxSizePenalty = 10.0;
    critter::gSpeed2Energy = 0.1;
    critter::gYaw2Energy = 0.1;
    critter::gLight2Energy = 0.01;
    critter::gFocus2Energy = 0.001;
    critter::gFixedEnergyDrain = 0.1;
    critter::gCritterHeight = 0.2;

	food::gMinFoodEnergy = 20.0;
	fMinFoodEnergyAtDeath = food::gMinFoodEnergy;
    food::gMaxFoodEnergy = 300.0;
    food::gSize2Energy = 100.0;
    food::gFoodHeight = 0.6;
    food::gFoodColor.r = 0.2;
    food::gFoodColor.g = 0.6;
    food::gFoodColor.b = 0.2;
    
    barrier::gBarrierHeight = 5.0;
    barrier::gBarrierColor.r = 0.35;
    barrier::gBarrierColor.g = 0.25;
    barrier::gBarrierColor.b = 0.15;
  
}


//---------------------------------------------------------------------------
// TSimulation::InitMonitoringWindows
//
// Create and display monitoring windows
//---------------------------------------------------------------------------

void TSimulation::InitMonitoringWindows()
{
	// Critter birthrate
	fBirthrateWindow = new TChartWindow( "born / (born + created)", "Birthrate" );
//	fBirthrateWindow->setWindowTitle( QString( "born / (born + created)" ) );
	sprintf( fBirthrateWindow->title, "born / (born + created)" );
	fBirthrateWindow->SetRange(0.0, 1.0);
	fBirthrateWindow->setFixedSize(TChartWindow::kMaxWidth, TChartWindow::kMaxHeight);

	// Critter fitness		
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
		{ 1.0, 0.0, 0.0, 0.0 },
		{ 0.0, 1.0, 0.0, 0.0 },
		{ 0.0, 0.0, 1.0, 0.0 },
		{ 0.0, 1.0, 1.0, 0.0 },
		{ 1.0, 0.0, 1.0, 0.0 },
		{ 1.0, 1.0, 0.0, 0.0 },
		{ 1.0, 1.0, 1.0, 1.0 }
	};
	
	const short numpop = (fNumDomains < 2) ? 1 : (fNumDomains + 1);
    fPopulationWindow = new TChartWindow( "Population", "Population", numpop);
//	fPopulationWindow->setWindowTitle( "population size" );
	sprintf( fPopulationWindow->title, "Population" );
	for (int i = 0; i < numpop; i++)
	{
		int colorIndex = i % 7;
		fPopulationWindow->SetRange(short(i), 0, fMaxCritters);
		fPopulationWindow->SetColor(i, popColor[colorIndex]);
	}
	fPopulationWindow->setFixedSize(TChartWindow::kMaxWidth, TChartWindow::kMaxHeight);

	// Brain monitor
	fBrainMonitorWindow = new TBrainMonitorWindow();
//	fBrainMonitorWindow->setWindowTitle( QString( "Brain Monitor" ) );
	
	// Critter POV
	fCritterPOVWindow = new TCritterPOVWindow( fMaxCritters, this );
	
	// Status window
	fTextStatusWindow = new TTextStatusWindow( this );
				
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

	if (fCritterPOVWindow != NULL)
		fCritterPOVWindow->RestoreFromPrefs( fBirthrateWindow->width() + 1, kMenuBarHeight + mainWindowTitleHeight + titleHeight );
	
	if (fBrainMonitorWindow != NULL)
		fBrainMonitorWindow->RestoreFromPrefs( fBirthrateWindow->width() + 1, kMenuBarHeight + mainWindowTitleHeight + titleHeight + titleHeight );

	if (fTextStatusWindow != NULL)
	{
		QDesktopWidget* desktop = QApplication::desktop();
		const QRect& screenSize = desktop->screenGeometry( desktop->primaryScreen() );
		fTextStatusWindow->RestoreFromPrefs( screenSize.width() - TTextStatusWindow::kDefaultWidth, kMenuBarHeight /*+ mainWindowTitleHeight + titleHeight*/ );
	}
}

//---------------------------------------------------------------------------
// TSimulation::Interact
//---------------------------------------------------------------------------
void TSimulation::Interact()
{       
    critter* c = NULL;
    critter* d = NULL;
	critter* newCritter = NULL;
	critter* oldCritter = NULL;
    food* f = NULL;
    cxsortedlist newCritters;
	long i;
	long j;
    short id = 0;
	short jd;
	short kd;
    short fd;
	bool cDied;
	bool foodMarked = 0;
    float cpower;
    float dpower;
    
	fNewDeaths = 0;

	// first x-sort all the critters
	critter::gXSortedCritters.sort();
	
	// food list and barrier list should be x-sorted already

	fCurrentFittestCount = 0;
	smPrint( "setting fCurrentFittestCount to 0\n" );
	float prevAvgFitness = fAverageFitness; // used in smite code, to limit critters that are smitable
    fAverageFitness = 0.0;
	fNumAverageFitness = 0;	// need this because we'll only count critters that have lived at least a modest portion of their lifespan (fSmiteAgeFrac)
//	fNumLeastFit = 0;
//	fNumSmited = 0;
	for( i = 0; i < fNumDomains; i++ )
	{
		fDomains[i].fNumLeastFit = 0;
		fDomains[i].fNumSmited = 0;
	}

	// If we're saving gene stats, zero them out them here
	if( fRecordGeneStats )
	{
		for( i = 0; i < genome::gNumBytes; i++ )
		{
			fGeneSum[i] = 0;
			fGeneSum2[i] = 0;
		}
	}
	
	// zero'ing out the FoodBandNumCritters stats
	if( fRecordFoodBandStats && (food::gNumFoodBands > 0) )
	{
		fNumCrittersNotInOrNearAnyFoodBand = 0;

		for( i = 0; i < food::gNumFoodBands; i++ )
		{
			fNumCrittersInFoodBand[i] = 0;
			fNumCrittersWithin5UnitsOfFoodBand[i] = 0;
			fNumCrittersWithin10UnitsOfFoodBand[i] = 0;
		}
	}

	// Take care of deaths first, plus least-fit determinations
	// Also use this as a convenient place to compute some stats

	//cout << "before deaths "; critter::gXSortedCritters.list();	//dbg
	fCurrentNeuronGroupCountStats.reset();
	critter::gXSortedCritters.reset();
    while (critter::gXSortedCritters.next(c))
    {
		fCurrentNeuronGroupCountStats.add( c->Brain()->NumNeuronGroups() );

        // Determine the domain in which the critter currently is located

        id = c->Domain();

        if ( (c->Energy() <= 0.0)		||
			 (c->Age() >= c->MaxAge())  ||
             ((!globals::edges) && ((c->x() < 0.0) || (c->x() >  globals::worldsize) ||
									(c->z() > 0.0) || (c->z() < -globals::worldsize))) )
        {
            if (c->Age() >= c->MaxAge())
                fNumberDiedAge++;
            else if (c->Energy() <= 0.0)
                fNumberDiedEnergy++;
            else
                fNumberDiedEdge++;
            Death(c);
            continue; // nothing else to do for this poor schmo
        }

	#ifdef OF1
        if ( (id == 0) && (drand48() < fDeathProb) )
        {
            Death(c);
            continue;
        }
	#endif

	#ifdef DEBUGCHECK
        debugcheck("after a death in interact");
	#endif DEBUGCHECK
	
		// If we're saving gene stats, compute them here
		if( fRecordGeneStats )
		{
			for( i = 0; i < genome::gNumBytes; i++ )
			{
				fGeneSum[i] += c->Genes()->GeneUIntValue(i);
				fGeneSum2[i] += c->Genes()->GeneUIntValue(i) * c->Genes()->GeneUIntValue(i);
			}
		}
		
		// If we're saving critter-occupancy-of-food-band stats, compute them here
		if( fRecordFoodBandStats && (food::gNumFoodBands > 0) )
		{
			// First check and see if a critter is in or near a foodband.
			// WARNING: WE MUST MAKE SURE FOODBANDS OR FOODBAND NEIGHBOORHOODS DO NOT OVERLAP!
			// If Foodbands or neighboorhoods overlap, the critter will still only be counted once, but will be counted as a member of the lowest numbered foodband that it is a member of.
			bool critterAccountedFor = false;
			
			float c_Z = c->z() / globals::worldsize;			// More efficient to divide once now than every time we need to make a comparison
			float fiveUnits = 5.0 / globals::worldsize;
			float tenUnits = 10.0 / globals::worldsize;
			
			for( int i = 0; i < food::gNumFoodBands; i++ )
			{
				// Is it in a band?
				if( c_Z >= food::gFoodBand[i].zMin && c_Z <= food::gFoodBand[i].zMax )
				{
					fNumCrittersInFoodBand[i]++;
					critterAccountedFor = true;
//					cout << "Assigned a Critter and it's value is: " << c->z() << endl;
					break ;
				}
				
				// Is it within 5 units of a band? (inclusive)
				else if( (c_Z < food::gFoodBand[i].zMin && c_Z >= food::gFoodBand[i].zMin - fiveUnits) || (c_Z > food::gFoodBand[i].zMax && c_Z <= food::gFoodBand[i].zMax + fiveUnits) )
				{
					fNumCrittersWithin5UnitsOfFoodBand[i]++;
					critterAccountedFor = true;
					break ;
				}
				
				// How about within 10 units of a band? (inclusive)
				else if( (c_Z < food::gFoodBand[i].zMin && c_Z >= food::gFoodBand[i].zMin - tenUnits) || (c_Z > food::gFoodBand[i].zMax && c_Z <= food::gFoodBand[i].zMax + tenUnits) )
				{
					fNumCrittersWithin10UnitsOfFoodBand[i]++;
					critterAccountedFor = true;
					break ;
				}
			}
			
			// Critter wasn't in or near any band of food. 
			if( critterAccountedFor == false )
			{
				fNumCrittersNotInOrNearAnyFoodBand++;
			}
		}	
				
		// Figure out who is least fit, if we're doing smiting to make room for births
		
		// Do the bookkeeping for the specific domain, if we're using domains
		// Note: I think we must have at least one domain these days - lsy 6/1/05
		
		// The test against average fitness is an attempt to keep fit organisms from being smited, in general,
		// but it also helps protect against the situation when there are so few potential low-fitness candidates,
		// due to the age constraint and/or population size, that critters can end up on both the highest fitness
		// and the lowest fitness lists, which can actually cause a crash (or at least used to).
//		printf( "%ld id=%ld, domain=%d, maxLeast=%d, numLeast=%d, numCrit=%d, maxCrit-
		if( (fNumDomains > 0) && (fDomains[id].fMaxNumLeastFit > 0) )
		{
			if( ((fDomains[id].numcritters > (fDomains[id].maxnumcritters - fDomains[id].fMaxNumLeastFit))) &&	// if there are getting to be too many critters, and
				(c->Age() >= (fSmiteAgeFrac * c->MaxAge())) &&													// the current critter is old enough to consider for smiting, and
				(c->Fitness() < prevAvgFitness) &&																// the current critter has worse than average fitness,
				( (fDomains[id].fNumLeastFit < fDomains[id].fMaxNumLeastFit)	||								// (we haven't filled our quota yet, or
				  (c->Fitness() < fDomains[id].fLeastFit[fDomains[id].fNumLeastFit-1]->Fitness()) ) )			// the critter is bad enough to displace one already in the queue)
			{
				if( fDomains[id].fNumLeastFit == 0 )
				{
					// It's the first one, so just store it
					fDomains[id].fLeastFit[0] = c;
					fDomains[id].fNumLeastFit++;
					smPrint( "critter %ld added to least fit list for domain %d at position 0 with fitness %g\n", c->Number(), id, c->Fitness() );
				}
				else
				{
					int i;
					
					// Find the position to be replaced
					for( i = 0; i < fDomains[id].fNumLeastFit; i++ )
						if( c->Fitness() < fDomains[id].fLeastFit[i]->Fitness() )	// worse than the one in this slot
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
					smPrint( "critter %ld added to least fit list for domain %d at position %d with fitness %g\n", c->Number(), id, i, c->Fitness() );
				}
			}
		}
	}

// Following debug output is accurate only when there is a single domain
//	if( fDomains[0].fNumLeastFit > 0 )
//		printf( "%ld numSmitable = %d out of %d, from %ld critters out of %ld\n", fStep, fDomains[0].fNumLeastFit, fDomains[0].fMaxNumLeastFit, fDomains[0].numcritters, fDomains[0].maxnumcritters );

	// If we're saving gene stats, record them here
	if( fRecordGeneStats )
	{
		fprintf( fGeneStatsFile, "%ld", fStep );
		for( i = 0; i < genome::gNumBytes; i++ )
		{
			float mean, stddev;
			
			mean = (float) fGeneSum[i] / (float) critter::gXSortedCritters.count();
			stddev = sqrt( (float) fGeneSum2[i] / (float) critter::gXSortedCritters.count()  -  mean * mean );
			fprintf( fGeneStatsFile, " %.1f,%.1f", mean, stddev );
		}
		fprintf( fGeneStatsFile, "\n" );
	}
	
#if DebugSmite
	for( id = 0; id < fNumDomains; id++ )
	{
		printf( "At age %ld in domain %d (c,n,c->fit) =", fStep, id );
		for( i = 0; i < fDomains[id].fNumLeastFit; i++ )
			printf( " (%08lx,%ld,%5.2f)", (ulong) fDomains[id].fLeastFit[i], fDomains[id].fLeastFit[i]->Number(), fDomains[id].fLeastFit[i]->Fitness() );
		printf( "\n" );
	}
#endif

	//cout << "after deaths1 "; critter::gXSortedCritters.list();	//dbg

	// Now go through the list, and use the influence radius to determine
	// all possible interactions

#if DebugMaxFitness
	critter::gXSortedCritters.reset();
	critter::gXSortedCritters.next( c );
	critter* lastCritter;
	critter::gXSortedCritters.last( lastCritter );
	printf( "%s: at age %ld about to process %ld critters, %ld pieces of food, starting with critter %08lx (%4ld), ending with critter %08lx (%4ld)\n", __FUNCTION__, fStep, critter::gXSortedCritters.count(), food::gXSortedFood.count(), (ulong) c, c->Number(), (ulong) lastCritter, lastCritter->Number() );
#endif

	food::gXSortedFood.reset();
	critter::gXSortedCritters.reset();
    while (critter::gXSortedCritters.next(c))
    {
        // determine the domain in which the critter currently is located

        id = c->Domain();

		// now see if there's an overlap with any other critters

		//cout << "cr" eql c sp "cr->l" eql c->GetListLink() sp "pre-m" eql critter::gXSortedCritters.marcItem;
        critter::gXSortedCritters.mark(); // so can point back to this critter later
		//cout << " post-m" eql critter::gXSortedCritters.marcItem nlf;
		
        cDied = FALSE;
        jd = id;
        kd = id;

        while (critter::gXSortedCritters.next(d)) // to end of list or...
        {
            if ( (d->x() - d->radius()) >= (c->x() + c->radius()) )
                break;  // this guy (& everybody else in list) is too far away

            // so if we get here, then c & d are close enough in x to interact

            if ( fabs(d->z() - c->z()) < (d->radius() + c->radius()) )
            {
                // and if we get here then they are also close enough in z,
                // so must actually worry about their interaction

				ttPrint( "age %ld: critters # %ld & %ld are close\n", fStep, c->Number(), d->Number() );

                jd = d->Domain();

				// now take care of mating

			#ifdef OF1
                if ( (c->Mate() > fMateThreshold) &&
                     (d->Mate() > fMateThreshold) &&          // it takes two!
                     ((c->Age() - c->LastMate()) >= fMateWait) &&
                     ((d->Age() - d->LastMate()) >= fMateWait) &&  // and some time
                     (c->Energy() > fMinMateFraction * c->MaxEnergy()) &&
                     (d->Energy() > fMinMateFraction * d->MaxEnergy()) && // and energy
                     (kd == 1) && (jd == 1) ) // in the safe domain
			#else
                if ( (c->Mate() > fMateThreshold) &&
                     (d->Mate() > fMateThreshold) &&          // it takes two!
                     ((c->Age() - c->LastMate()) >= fMateWait) &&
                     ((d->Age() - d->LastMate()) >= fMateWait) &&  // and some time
                     (c->Energy() > fMinMateFraction * c->MaxEnergy()) &&
                     (d->Energy() > fMinMateFraction * d->MaxEnergy()) ) // and energy
			#endif
                {
                    kd = WhichDomain(0.5*(c->x()+d->x()),
                                     0.5*(c->z()+d->z()),
                                     kd);

					bool smited = false;
					
					if( (fDomains[kd].numcritters >= fDomains[kd].maxnumcritters) &&	// too many critters to reproduce withing a bit of smiting
						(fDomains[kd].fNumLeastFit > fDomains[kd].fNumSmited) )			// we've still got some left that are suitable for smiting
					{
						while( (fDomains[kd].fNumSmited < fDomains[kd].fNumLeastFit) &&		// there are any left to smite
							   ((fDomains[kd].fLeastFit[fDomains[kd].fNumSmited] == c) ||	// trying to smite mommy
								(fDomains[kd].fLeastFit[fDomains[kd].fNumSmited] == d) ||	// trying to smite daddy
							    ((fCurrentFittestCount > 0) && (fDomains[kd].fLeastFit[fDomains[kd].fNumSmited]->Fitness() > fCurrentMaxFitness[fCurrentFittestCount-1]))) )	// trying to smite one of the fittest
						{
							// We would have smited one of our mating pair, or one of the fittest, which wouldn't be prudent,
							// so just step over them and see if there's someone else to smite
							fDomains[kd].fNumSmited++;
						}
						if( fDomains[kd].fNumSmited < fDomains[kd].fNumLeastFit )	// we've still got someone to smite, so do it
						{
							Death( fDomains[kd].fLeastFit[fDomains[kd].fNumSmited] );
							fDomains[kd].fNumSmited++;
							fNumberDiedSmite++;
							smited = true;
							//cout << "********************* SMITE *******************" nlf;	//dbg
						}
					}

                    if ( (fDomains[kd].numcritters < fDomains[kd].maxnumcritters) &&
                         ((critter::gXSortedCritters.count() + newCritters.count()) < fMaxCritters) )
                    {
						// Still got room for more
                        if( (fMiscCritters < 0) ||									// miscegenation function not being used
							(fDomains[kd].numbornsincecreated < fMiscCritters) ||	// miscegenation function not in use yet
							(drand48() < c->MateProbability(d)) )					// miscegenation function allows the birth
                        {
							ttPrint( "age %ld: critters # %ld & %ld are mating\n", fStep, c->Number(), d->Number() );
                            fNumBornSinceCreated++;
                            fDomains[kd].numbornsincecreated++;
							
                            critter* e = critter::getfreecritter(this, &fStage);
                            Q_CHECK_PTR(e);

                            e->Genes()->Crossover(c->Genes(), d->Genes(), true);
                            e->grow();
                            float eenergy = c->mating(fMateFitnessParameter, fMateWait) + d->mating(fMateFitnessParameter, fMateWait);
                            e->Energy(eenergy);
                            e->FoodEnergy(eenergy);
                            e->settranslation(0.5*(c->x() + d->x()),
                                              0.5*(c->y() + d->y()),
                                              0.5*(c->z() + d->z()));
                            e->setyaw(0.5*(c->yaw() + d->yaw()));   // was (360.0*drand48());
                            e->Domain(kd);
                            fStage.AddObject(e);
                            newCritters.add(e); // add it to the full list later; the e->listLink that gets auto stored here must be replaced with one from full list below
                            fDomains[kd].numcritters++;
                            fNumberBorn++;
                            fDomains[kd].numborn++;
							fNeuronGroupCountStats.add( e->Brain()->NumNeuronGroups() );
							ttPrint( "age %ld: critter # %ld is born\n", fStep, e->Number() );
                        }
                        else	// miscegenation function denied this birth
						{
							fBirthDenials++;
                            fMiscDenials++;
						}
                    }
					else	// Too many critters
						fBirthDenials++;
                }

			#ifdef DEBUGCHECK
                debugcheck("after a birth in interact");
			#endif DEBUGCHECK

				// now take care of fighting

                if (fPower2Energy > 0.0)
                {
                    if ( (c->Fight() > fFightThreshold) )
                        cpower = c->Strength() * c->SizeAdvantage() * c->Fight() * (c->Energy()/c->MaxEnergy());
                    else
                        cpower = 0.0;

                    if ( (d->Fight() > fFightThreshold) )
                        dpower = d->Strength() * d->SizeAdvantage() * d->Fight() * (d->Energy()/d->MaxEnergy());
                    else
                        dpower = 0.0;

                    if ( (cpower > 0.0) || (dpower > 0.0) )
                    {
						ttPrint( "age %ld: critters # %ld & %ld are fighting\n", fStep, c->Number(), d->Number() );
                        // somebody wants to fight
                        fNumberFights++;
                        c->damage(dpower * fPower2Energy);
                        d->damage(cpower * fPower2Energy);
                        if (d->Energy() <= 0.0)
                        {
							//cout << "before deaths2 "; critter::gXSortedCritters.list();	//dbg
                            Death(d);
                            fNumberDiedFight++;
							//cout << "after deaths2 "; critter::gXSortedCritters.list();	//dbg
                        }
                        if (c->Energy() <= 0.0)
                        {
                            critter::gXSortedCritters.tomark(); // point back to c
                            Death(c);
                            fNumberDiedFight++;
                            // note: this leaves list pointing to item before c
                            critter::gXSortedCritters.mark();
							//cout << "after deaths3 "; critter::gXSortedCritters.list();	//dbg
                            cDied = true;
                            break;
                        }
                    }
                }

			#ifdef DEBUGCHECK
                debugcheck("after fighting in interact");
			#endif DEBUGCHECK

            }  // if close enough
        }  // while (critter::gXSortedCritters.next(d))

	#ifdef DEBUGCHECK
        debugcheck("after all critter interactions in interact");
	#endif DEBUGCHECK

        if (cDied)
			continue; // nothing else to do with c, it's gone!
		
        critter::gXSortedCritters.tomark(); // point critter list back to c

		// they finally get to eat (couldn't earlier to keep from conferring
		// a special advantage on critters early in the sorted list)

        fd = id;
        while (food::gXSortedFood.next(f))
        {
            if ( (f->x() + f->radius()) > (c->x() - c->radius()) )
            {
                // end of food comes after beginning of critter
                if (!foodMarked) // first time only
                {
                    foodMarked = true;
                    food::gXSortedFood.markprev();   // mark previous item in list
                }
                if ( (f->x() - f->radius()) > (c->x() + c->radius()) )
                {
                    // beginning of food comes after end of critter,
                    // so there is no overlap, and we can stop searching
                    // for this critter's possible foods

                    // first, set the sorted food list back to the marked
                    // item which immediately preceeds the first possible
                    // food item for this, and therefore the next, critter
                    food::gXSortedFood.tomark();
                    break;  // then get out of the sorted food while loop
                }
                else // we actually have an overlap in x
                {
                    if ( fabs(f->z()-c->z()) < (f->radius()+c->radius()) )
                    {
                        // also overlap in z, so they really interact
						ttPrint( "age %ld: critter # %ld is eating\n", fStep, c->Number() );
                        fd = f->domain();
						fFoodEnergyOut += c->eat(f, fEatFitnessParameter, fEat2Consume, fEatThreshold);

                        if (f->energy() <= 0.0)  // all gone
                        {
                           fDomains[fd].foodcount--;
                           food::gXSortedFood.remove();   // get it out of the list
                           fStage.RemoveObject(f);  // get it out of the world
                           delete f;				// get it out of memory
                        }
                        // whether this food item is completely used up or not,
                        // must set the sorted food list back to the marked
                        // item which immediately preceeded f, so either f,
                        // or the next item in the list, will be available
                        // to the next critter
                        food::gXSortedFood.tomark();

                        // but this guy only gets to eat from one food source
                        break;  // so get out of the sorted food while loop
                    }
                }
            }
        } // while loop on food

	#ifdef DEBUGCHECK
        debugcheck("after eating in interact");
	#endif DEBUGCHECK

		// keep tabs of current and average fitness for surviving organisms

		if( c->Age() >= (fSmiteAgeFrac * c->MaxAge()) )
		{
			fAverageFitness += c->Fitness();
			fNumAverageFitness++;
		}
        if( (fCurrentFittestCount < MAXFITNESSITEMS) || (c->Fitness() > fCurrentMaxFitness[fCurrentFittestCount-1]) )
        {
			if( (fCurrentFittestCount == 0) || ((c->Fitness() <= fCurrentMaxFitness[fCurrentFittestCount-1]) && (fCurrentFittestCount < MAXFITNESSITEMS)) )	// just append
			{
				fCurrentMaxFitness[fCurrentFittestCount] = c->Fitness();
				fCurrentFittestCritter[fCurrentFittestCount] = c;
				fCurrentFittestCount++;
			#if DebugMaxFitness
				printf( "appended critter %08lx (%4ld) to fittest list at position %d with fitness %g, count = %d\n", (ulong) c, c->Number(), fCurrentFittestCount-1, c->Fitness(), fCurrentFittestCount );
			#endif
			}
			else	// must insert
			{
				for( i = 0; i <  fCurrentFittestCount ; i++ )
				{
					if( c->Fitness() > fCurrentMaxFitness[i] )
						break;
				}
				
				for( j = min( fCurrentFittestCount, MAXFITNESSITEMS-1 ); j > i; j-- )
				{
					fCurrentMaxFitness[j] = fCurrentMaxFitness[j-1];
					fCurrentFittestCritter[j] = fCurrentFittestCritter[j-1];
				}
				
				fCurrentMaxFitness[i] = c->Fitness();
				fCurrentFittestCritter[i] = c;
				if( fCurrentFittestCount < MAXFITNESSITEMS )
					fCurrentFittestCount++;
			#if DebugMaxFitness
				printf( "inserted critter %08lx (%4ld) into fittest list at position %ld with fitness %g, count = %d\n", (ulong) c, c->Number(), i, c->Fitness(), fCurrentFittestCount );
			#endif
			}
        }

    } // while loop on critters

//	fAverageFitness /= critter::gXSortedCritters.count();
	if( fNumAverageFitness > 0 )
		fAverageFitness /= fNumAverageFitness;

#if DebugMaxFitness
	printf( "At age %ld (c,n,fit,c->fit) =", fStep );
	for( i = 0; i < fCurrentFittestCount; i++ )
		printf( " (%08lx,%ld,%5.2f,%5.2f)", (ulong) fCurrentFittestCritter[i], fCurrentFittestCritter[i]->Number(), fCurrentMaxFitness[i], fCurrentFittestCritter[i]->Fitness() );
	printf( "\n" );
#endif

    if (fMonitorGeneSeparation && (fNewDeaths > 0))
        CalculateGeneSeparationAll();
        
	// now for a little spontaneous generation!
	
    if (((long)(critter::gXSortedCritters.count() + newCritters.count())) < fMaxCritters)
    {
        // provided there are less than the maximum number of critters already

		// first deal with the individual domains
        for (id = 0; id < fNumDomains; id++)
        {
            while (fDomains[id].numcritters < fDomains[id].minnumcritters)
            {
                fNumberCreated++;
                fDomains[id].numcreated++;
                fNumBornSinceCreated = 0;
                fDomains[id].numbornsincecreated = 0;
                fLastCreated = fStep;
                fDomains[id].lastcreate = fStep;
                critter* newCritter = critter::getfreecritter(this, &fStage);
                Q_CHECK_PTR(newCritter);
                
                if ( fNumberFit && (fDomains[id].numdied >= fNumberFit) )
                {
                    // the list exists and is full
                    if (fFitness1Frequency
                    	&& ((fDomains[id].numcreated / fFitness1Frequency) * fFitness1Frequency == fDomains[id].numcreated) )
                    {
                        // revive 1 fittest
                        newCritter->Genes()->CopyGenes(fDomains[id].fittest[0]->genes);
                        fNumberCreated1Fit++;
                    }
                    else if (fFitness2Frequency
                    		 && ((fDomains[id].numcreated / fFitness2Frequency) * fFitness2Frequency == fDomains[id].numcreated) )
                    {
                        // mate 2 from array of fittest
                        newCritter->Genes()->Crossover(fDomains[id].fittest[fDomains[id].ifit]->genes,
                            				  		   fDomains[id].fittest[fDomains[id].jfit]->genes,
                            				  		   true);
                        fNumberCreated2Fit++;
                        ijfitinc(&(fDomains[id].ifit), &(fDomains[id].jfit));
                    }
                    else
                    {
                        // otherwise, just generate a random, hopeful monster
                        newCritter->Genes()->Randomize();
                        fNumberCreatedRandom++;
                    }
                }
                else
                {
                    // otherwise, just generate a random, hopeful monster
                    newCritter->Genes()->Randomize();
                    fNumberCreatedRandom++;
                }

                newCritter->grow();
                fFoodEnergyIn += newCritter->FoodEnergy();
				float x = drand48() * fDomains[id].xsize + fDomains[id].xleft;
				float y = 0.5 * critter::gCritterHeight;
				float z = drand48() * -globals::worldsize;
				float yaw = drand48() * 360.0;
			#if TestWorld
				x = fDomains[id].xleft  +  0.666 * fDomains[id].xsize;
				z = - globals::worldsize * ((float) (i+1) / (fDomains[id].initnumcritters + 1));
				yaw = 95.0;
			#endif
                newCritter->settranslation( x, y, z );
                newCritter->setyaw( yaw );
                newCritter->Domain(id);
                fStage.AddObject(newCritter);
                fDomains[id].numcritters++;
                newCritters.add(newCritter); // add it to the full list later; the e->listLink that gets auto stored here must be replaced with one from full list below
				fNeuronGroupCountStats.add( newCritter->Brain()->NumNeuronGroups() );
            }
        }

	#ifdef DEBUGCHECK
        debugcheck("after domain creations in interact");
	#endif DEBUGCHECK
		
		// then deal with global creation if necessary

        while (((long)(critter::gXSortedCritters.count() + newCritters.count())) < fMinNumCritters)
        {
            fNumberCreated++;
            numglobalcreated++;
            
            if ((numglobalcreated == 1) && (fNumDomains > 1))
                errorflash(0, "Possible global influence on domains due to minnumcritters");
                
            fNumBornSinceCreated = 0;
            fLastCreated = fStep;

            newCritter = critter::getfreecritter(this, &fStage);
            Q_CHECK_PTR(newCritter);

            if (fNumberFit && fNumberDied >= fNumberFit)
            {
                // the list exists and is full
                if (fFitness1Frequency
                	&& ((numglobalcreated / fFitness1Frequency) * fFitness1Frequency == numglobalcreated))
                {
                    // revive 1 fittest
                    newCritter->Genes()->CopyGenes(fFittest[0]->genes);
                    fNumberCreated1Fit++;
                }
                else if (fFitness2Frequency && ((numglobalcreated / fFitness2Frequency) * fFitness2Frequency == numglobalcreated))
                {
                    // mate 2 from array of fittest
                    newCritter->Genes()->Crossover(fFittest[fFitI]->genes, fFittest[fFitJ]->genes, true);
                    fNumberCreated2Fit++;
                    ijfitinc(&fFitI, &fFitJ);
                }
                else
                {
                    // otherwise, just generate a random, hopeful monster
                    newCritter->Genes()->Randomize();
                    fNumberCreatedRandom++;
                }
            }
            else
            {
                // otherwise, just generate a random, hopeful monster
                newCritter->Genes()->Randomize();
                fNumberCreatedRandom++;
            }

            newCritter->grow();
            fFoodEnergyIn += newCritter->FoodEnergy();
            newCritter->settranslation(drand48() * globals::worldsize, 0.5 * critter::gCritterHeight, drand48() * -globals::worldsize);
            newCritter->setyaw(drand48() * 360.0);
            id = WhichDomain(newCritter->x(), newCritter->z(), 0);
            newCritter->Domain(id);
            fDomains[id].numcreated++;
            fDomains[id].lastcreate = fStep;
            fDomains[id].numcritters++;
            fStage.AddObject(newCritter);
            newCritters.add(newCritter); // add it to the full list later; the e->listLink that gets auto stored here must be replaced with one from full list below
			fNeuronGroupCountStats.add( newCritter->Brain()->NumNeuronGroups() );
        }

	#ifdef DEBUGCHECK
        debugcheck("after global creations in interact");
	#endif DEBUGCHECK
    }
        
	// now add the new critters to the existing list
	
    const int newlifes = newCritters.count();
    if (newlifes > 0)
    {
        bool foundinsertionpt;
        bool oldlistfinished = false;
        critter::gXSortedCritters.reset();
//		newCritters.sort();	// not needed, because new critters are always added wih cxsortedlist::add(), which sorts on add
        newCritters.reset();
        while (newCritters.next(newCritter))
        {
            if (fMonitorGeneSeparation)
                CalculateGeneSeparation(newCritter);

            if (oldlistfinished)
                newCritter->listLink = critter::gXSortedCritters.append(newCritter);
            else
            {
                foundinsertionpt = false;
                while (critter::gXSortedCritters.next(oldCritter))
                {
                    if ( (newCritter->x() - newCritter->radius()) < (oldCritter->x() - oldCritter->radius()) )
                    {
                        newCritter->listLink = critter::gXSortedCritters.inserthere(newCritter);
                        foundinsertionpt = true;
                        break;
                    }
                }
                if (!foundinsertionpt)
                {
                    oldlistfinished = true;
                    newCritter->listLink = critter::gXSortedCritters.append(newCritter);
                }
            }
        }
        newCritters.clear();
    }

#ifdef DEBUGCHECK
    debugcheck("after newcritters added to critter::gXSortedCritters in interact");
#endif DEBUGCHECK

    if ((newlifes || fNewDeaths) && fMonitorGeneSeparation)
    {
        if (fRecordGeneSeparation)
            RecordGeneSeparation();
            
		if (fChartGeneSeparation && fGeneSeparationWindow != NULL)
			fGeneSeparationWindow->AddPoint(fGeneSepVals, fNumGeneSepVals);
    }

	// finally, keep the world's food supply going...

	// first deal with the individual domains
    if ((long)food::gXSortedFood.count() < fMaxFoodCount) // can't create if too many overall
    {
        for (fd = 0; fd < fNumDomains; fd++)
        {
            if (fDomains[fd].foodcount < fDomains[fd].maxfoodgrown)
            {
                float foodprob = (fDomains[fd].maxfoodgrown - fDomains[fd].foodcount) * fFoodRate;
                if (drand48() < foodprob)
                {
                    food* f = new food;
                    fFoodEnergyIn += f->energy();
                    f->setx(drand48()*fDomains[fd].xsize + fDomains[fd].xleft);
                    f->domain(fd);
                    food::gXSortedFood.add(f);
                    fStage.AddObject(f);
                    fDomains[fd].foodcount++;
                }
                
                long newfood = fDomains[fd].minfoodcount - fDomains[fd].foodcount;
                for (i = 0; i < newfood; i++)
                {
                    food* f = new food;
                    fFoodEnergyIn += f->energy();
                    f->setx(drand48()*fDomains[fd].xsize + fDomains[fd].xleft);
                    f->domain(fd);
                    food::gXSortedFood.add(f);
                    fStage.AddObject(f);
                    fDomains[fd].foodcount++;
                }
            }
		#ifdef DEBUGCHECK
            debugcheck("after domain food growth in interact");
		#endif DEBUGCHECK
        }

		// then deal with the global food supply if necessary
        if ((long)food::gXSortedFood.count() < fMaxFoodGrown) // can get higher due to deaths
        {
            const float foodprob = (fMaxFoodGrown - food::gXSortedFood.count()) * fFoodRate;
            if (drand48() < foodprob)
            {
                food* f = new food;
                fFoodEnergyIn += f->energy();
                fd = WhichDomain(f->x(),f->z(),0);
                f->domain(fd);
                food::gXSortedFood.add(f);
                fStage.AddObject(f);
                fDomains[fd].foodcount++;
            }
            
			const long newfood = fMinFoodCount - food::gXSortedFood.count();
            for (i = 0; i < newfood; i++)
            {
                food* f = new food;
                fFoodEnergyIn += f->energy();
                fd = WhichDomain(f->x(),f->z(),0);
                f->domain(fd);
                food::gXSortedFood.add(f);
                fStage.AddObject(f);
                fDomains[fd].foodcount++;
            }
		#ifdef DEBUGCHECK
            debugcheck("after global food growth in interact");
		#endif DEBUGCHECK
        }
    }
}


//---------------------------------------------------------------------------
// TSimulation::RecordGeneSeparation
//---------------------------------------------------------------------------
void TSimulation::RecordGeneSeparation()
{
	fprintf(fGeneSeparationFile, "%d %g %g %g\n",
			fStep,
			fMaxGeneSeparation,
			fMinGeneSeparation,
			fAverageGeneSeparation);
}


//---------------------------------------------------------------------------
// TSimulation::CalculateGeneSeparation
//---------------------------------------------------------------------------
void TSimulation::CalculateGeneSeparation(critter* ci)
{
	// TODO add assert to validate statement below
	
	// NOTE: This version assumes ci is *not* currently in gSortedCritters.
    // It also marks the current position in the list on entry and returns
    // there before exit so it can be invoked during the insertion of the
    // newcritters list into the existing gSortedCritters list.
	
	critter::gXSortedCritters.mark();
	
    critter* cj = NULL;
    float genesep;
    float genesepsum = 0.0;
    long numgsvalsold = fNumGeneSepVals;
        
	critter::gXSortedCritters.reset();
	while (critter::gXSortedCritters.next(cj))
    {
		genesep = ci->Genes()->CalcSeparation(cj->Genes());
        fMaxGeneSeparation = fmax(genesep, fMaxGeneSeparation);
        fMinGeneSeparation = fmin(genesep, fMinGeneSeparation);
        genesepsum += genesep;
        fGeneSepVals[fNumGeneSepVals++] = genesep;
    }
    
    long n = critter::gXSortedCritters.count();
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
	
	critter::gXSortedCritters.tomark();
}



//---------------------------------------------------------------------------
// TSimulation::CalculateGeneSeparationAll
//---------------------------------------------------------------------------
void TSimulation::CalculateGeneSeparationAll()
{
    critter* ci = NULL;
    critter* cj = NULL;
    
    float genesep;
    float genesepsum = 0.0;
    fMinGeneSeparation = 1.e+10;
    fMaxGeneSeparation = 0.0;
    fNumGeneSepVals = 0;
    
	critter::gXSortedCritters.reset();
	while (critter::gXSortedCritters.next(ci))
    {
		critter::gXSortedCritters.mark();

		while (critter::gXSortedCritters.next(cj))
		{	
            genesep = ci->Genes()->CalcSeparation(cj->Genes());
            fMaxGeneSeparation = max(genesep, fMaxGeneSeparation);
            fMinGeneSeparation = min(genesep, fMinGeneSeparation);
            genesepsum += genesep;
            fGeneSepVals[fNumGeneSepVals++] = genesep;
        }
		critter::gXSortedCritters.tomark();
    }
    
    // n * (n - 1) / 2 is how many calculations were made
	long n = critter::gXSortedCritters.count();
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
// TSimulation::SmiteOne
//---------------------------------------------------------------------------
void TSimulation::SmiteOne(short /*id*/, short /*smite*/)
{
/*
    if (id < 0)  // smite one anywhere in the world
    {
        fSortedCritters.next(c);
        smitten = c;
        while (fSortedCritters.next(c))
        {
            if (c->Age() > minsmiteage)
            {
                case (smite)
                1:  // fitness rate
            }
        }
    }
    else  // smite one in domain id
    {
    }
*/
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
            (*i) = min(1, fNumberFit - 1);
    }
}


//---------------------------------------------------------------------------
// TSimulation::Death
//
// Perform all actions needed to critter before list removal and deletion
//---------------------------------------------------------------------------
void TSimulation::Death(critter* c)
{
	Q_CHECK_PTR(c);
	long loserIDBestSoFar = 0;	// the way this is used depends on fCritterNumber being 1-based in critter.cp
	long loserIDBestRecent = 0;	// the way this is used depends on fCritterNumber being 1-based in critter.cp
	bool oneOfTheBestSoFar = false;
	bool oneOfTheBestRecent = false;
	
	const short id = c->Domain();
	
	ttPrint( "age %ld: critter # %ld has died\n", fStep, c->Number() );
	
    fNewDeaths++;
    fNumberDied++;
    fDomains[id].numdied++;
    fDomains[id].numcritters--;
	
	fLifeSpanStats.add( c->Age() );
	fLifeSpanRecentStats.add( c->Age() );
	fLifeFractionRecentStats.add( c->Age() / c->MaxAge() );
	
	// Make any final contributions to the critter's overall, lifetime fitness
    c->lastrewards(fEnergyFitnessParameter, fAgeFitnessParameter); // if any
    
	// If critter's carcass is to become food, make it so here
    if (fCrittersRfood
    	&& ((long)food::gXSortedFood.count() < fMaxFoodCount)
    	&& (fDomains[id].foodcount < fDomains[id].maxfoodcount)
    	&& (globals::edges || ((c->x() >= 0.0) && (c->x() <=  globals::worldsize) &&
    	                       (c->z() <= 0.0) && (c->z() >= -globals::worldsize))) )
    {
        float foodenergy = c->FoodEnergy();
		
        if (foodenergy < fMinFoodEnergyAtDeath)	// it will be bumped up to fMinFoodEnergyAtDeath
        {
            if (fMinFoodEnergyAtDeath >= food::gMinFoodEnergy)			// it will get turned into food
                fFoodEnergyIn += fMinFoodEnergyAtDeath - foodenergy;	// so account for the increase due to the bump (original amount already accounted for)
            else														// else it'll just be disposed of
                fFoodEnergyOut += foodenergy;							// so account for the lost energy
                
            foodenergy = fMinFoodEnergyAtDeath;
        }
		else	// it will retain its original value (no bump due to the min)
		{
			if( foodenergy < food::gMinFoodEnergy )	// it's going to be disposed of
				fFoodEnergyOut += foodenergy;		// so account for the lost energy
		}
        
        if (foodenergy >= food::gMinFoodEnergy)
        {
            food* f = new food(foodenergy, c->x(), c->z());
            Q_CHECK_PTR(f);
			food::gXSortedFood.add(f);  // dead critter becomes food
			fStage.AddObject(f);		// put replacement food into the world
			fDomains[id].foodcount++;
			f->domain(id);
        }
    }
    else
    {
        fFoodEnergyOut += c->FoodEnergy();
    }
	
	// Must call Die() for the critter before any of the uses of Fitness() below, so we get the final, true, post-death fitness
	c->Die();
	
	// Maintain a list of the fittest critters ever, for use in the online/steady-state GA
	// First on a domain-by-domain basis...
    long newfit = 0;
    FitStruct* saveFit;
    if( c->Fitness() > fDomains[id].fittest[fNumberFit-1]->fitness )
    {
        for( short i = 0; i < fNumberFit; i++ )
        {
            if( c->Fitness() > fDomains[id].fittest[i]->fitness )
			{
				newfit = i;
				break;
			}
		}
		
        saveFit = fDomains[id].fittest[fNumberFit-1];				// this is to save the data structure, not its contents
        for( short i = fNumberFit - 1; i > newfit; i-- )
            fDomains[id].fittest[i] = fDomains[id].fittest[i-1];
        fDomains[id].fittest[newfit] = saveFit;						// reuse the old data structure, but replace its contents...
        fDomains[id].fittest[newfit]->fitness = c->Fitness();
        fDomains[id].fittest[newfit]->genes->CopyGenes( c->Genes() );
		fDomains[id].fittest[newfit]->critterID = c->Number();
    }
    
	// Then on a whole-world basis...
    if( c->Fitness() > fFittest[fNumberFit-1]->fitness )
    {
		oneOfTheBestSoFar = true;
		
        for( short i = 0; i < fNumberFit; i++ )
        {
            if( c->Fitness() > fFittest[i]->fitness )
			{
				newfit = i;
				break;
			}
		}
				
		loserIDBestSoFar = fFittest[fNumberFit - 1]->critterID;	// this is the ID of the critter that is being booted from the bestSoFar (fFittest[]) list
        saveFit = fFittest[fNumberFit - 1];		// this is to save the data structure, not its contents
        for( short i = fNumberFit - 1; i > newfit; i-- )
            fFittest[i] = fFittest[i - 1];
        fFittest[newfit] = saveFit;				// reuse the old data structure, but replace its contents...
        fFittest[newfit]->fitness = c->Fitness();
        fFittest[newfit]->genes->CopyGenes( c->Genes() );
		fFittest[newfit]->critterID = c->Number();
    }
    
    if (c->Fitness() > fMaxFitness)
        fMaxFitness = c->Fitness();
	
	// Keep a separate list of the recent fittest, purely for data-gathering purposes
	// "Recent" means since the last archival recording of recent best, as determined by fBestRecentBrainAnatomyRecordFrequency
	// (Don't bother, if we're not gathering that kind of data)
    if( fBestRecentBrainAnatomyRecordFrequency && (c->Fitness() > fRecentFittest[fNumberRecentFit-1]->fitness) )
    {
		oneOfTheBestRecent = true;
		
        for( short i = 0; i < fNumberRecentFit; i++ )
        {
			// If the critter booted off of the bestSoFar list happens to remain on the bestRecent list,
			// then we don't want to let it be unlinked below, so clear loserIDBestSoFar
			// This loop tests the first part of the fRecentFittest[] list, and the last part is tested
			// below, so we don't need a separate loop to make this determination
			if( loserIDBestSoFar == c->Number() )
				loserIDBestSoFar = 0;
			
            if( c->Fitness() > fRecentFittest[i]->fitness )
			{
				newfit = i;
				break;
			}
		}
				
		loserIDBestRecent = fRecentFittest[fNumberRecentFit - 1]->critterID;	// this is the ID of the critter that is being booted from the bestRecent (fRecentFittest[]) list
        saveFit = fRecentFittest[fNumberRecentFit - 1];		// this is to save the data structure, not its contents
        for( short i = fNumberRecentFit - 1; i > newfit; i-- )
		{
			// If the critter booted off of the bestSoFar list happens to remain on the bestRecent list,
			// then we don't want to let it be unlinked below, so clear loserIDBestSoFar
			// This loop tests the last part of the fRecentFittest[] list, and the first part was tested
			// above, so we don't need a separate loop to make this determination
			if( loserIDBestSoFar == c->Number() )
				loserIDBestSoFar = 0;
			
            fRecentFittest[i] = fRecentFittest[i - 1];
		}
        fRecentFittest[newfit] = saveFit;				// reuse the old data structure, but replace its contents...
        fRecentFittest[newfit]->fitness = c->Fitness();
//		fRecentFittest[newfit]->genes->CopyGenes( c->Genes() );	// we don't save the genes in the bestRecent list
		fRecentFittest[newfit]->critterID = c->Number();
    }
    
	// Must also update the leastFit data structures, now that they
	// are used on-demand in the main mate/fight/eat loop in Interact()
	// Update the domain-specific leastFit list (only one, as we know which domain it's in)
	for( int i = 0; i < fDomains[id].fNumLeastFit; i++ )
	{
		if( fDomains[id].fLeastFit[i] != c )	// not one of our least fit, so just loop again to keep searching
			continue;

		// one of our least-fit critters died, so pull in the list over it
		smPrint( "removing critter %ld from the least fit list for domain %d at position %d with fitness %g (because it died)\n", c->Number(), id, i, c->Fitness() );
		for( int j = i; j < fDomains[id].fNumLeastFit-1; j++ )
			fDomains[id].fLeastFit[j] = fDomains[id].fLeastFit[j+1];
		fDomains[id].fNumLeastFit--;
		break;	// c can only appear once in the list, so we're done
	}
	
	// Remove critter from world
    fStage.RemoveObject(c);
    
    // Check and see if we are monitoring this guy
	if (c == fMonitorCritter)
	{
		fMonitorCritter = NULL;

		// Stop monitoring critter brain
		if (fBrainMonitorWindow != NULL)
			fBrainMonitorWindow->StopMonitoring();
	}
	
	// If we're recording all anatomies or recording best anatomies and this was one of the fittest critters,
	// then dump the anatomy to the appropriate location on disk
	if(  fBrainAnatomyRecordAll ||
		(fBestSoFarBrainAnatomyRecordFrequency && oneOfTheBestSoFar) ||
		(fBestRecentBrainAnatomyRecordFrequency && oneOfTheBestRecent) ||
		(fBrainAnatomyRecordSeeds && (c->Number() <= fInitNumCritters))
	  )
		c->Brain()->dumpAnatomical( "run/brain/anatomy", "death", c->Number(), c->Fitness() );
	else	// don't want brain anatomies for this critter, so must eliminate the "incept" and "birth" anatomies that were already recorded
	{
		char s[256];
	
		sprintf( s, "run/brain/anatomy/brainAnatomy_%lu_incept.txt", c->Number() );
		if( unlink( s ) )
			eprintf( "Error (%ld) unlinking \"%s\"\n", errno, s );
		sprintf( s, "run/brain/anatomy/brainAnatomy_%lu_birth.txt", c->Number() );
		if( unlink( s ) )
			eprintf( "Error (%ld) unlinking \"%s\"\n", errno, s );
	}
		
	// If this was one of the seed critters and we're recording their anatomies, then save the data in the appropriate directory
	if( fBrainAnatomyRecordSeeds && (c->Number() <= fInitNumCritters) )
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
	
	// If this critter was so good it displaced another critter from the bestSoFar (fFittest[]) list,
	// then nuke the booted critter's brain anatomy & function recordings
	// Note: A critter may be booted from the bestSoFar list, yet remain on the bestRecent list;
	// if this happens, loserIDBestSoFar will be reset to zero above, so we won't execute this block
	// of code and delete files that are needed for the bestRecent data logging
	// In the rare case that a critter is simultaneously booted from both best lists,
	// don't delete it here (so we don't try to delete it more than once)
	if( loserIDBestSoFar && (loserIDBestSoFar != loserIDBestRecent) )	//  depends on fCritterNumber being 1-based in critter.cp
	{
		char s[256];
		if( !fBrainAnatomyRecordAll )
		{
			sprintf( s, "run/brain/anatomy/brainAnatomy_%lu_incept.txt", loserIDBestSoFar );
			if( unlink( s ) )
				eprintf( "Error (%ld) unlinking \"%s\"\n", errno, s );
			sprintf( s, "run/brain/anatomy/brainAnatomy_%lu_birth.txt", loserIDBestSoFar );
			if( unlink( s ) )
				eprintf( "Error (%ld) unlinking \"%s\"\n", errno, s );
			sprintf( s, "run/brain/anatomy/brainAnatomy_%lu_death.txt", loserIDBestSoFar );
			if( unlink( s ) )
				eprintf( "Error (%ld) unlinking \"%s\"\n", errno, s );
		}
		if( !fBrainFunctionRecordAll )
		{
			sprintf( s, "run/brain/function/brainFunction_%lu.txt", loserIDBestSoFar );
			if( unlink( s ) )
				eprintf( "Error (%ld) unlinking \"%s\"\n", errno, s );
		}
	}

	// If this critter was so good it displaced another critter from the bestRecent (fRecentFittest[]) list,
	// then nuke the booted critter's brain anatomy & function recordings
	// In the rare case that a critter is simultaneously booted from both best lists,
	// we will only delete it here
	if( loserIDBestRecent )	//  depends on fCritterNumber being 1-based in critter.cp
	{
		char s[256];
		if( !fBrainAnatomyRecordAll )
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
		if( !fBrainFunctionRecordAll )
		{
			sprintf( s, "run/brain/function/brainFunction_%lu.txt", loserIDBestRecent );
			if( unlink( s ) )
				eprintf( "Error (%d) unlinking \"%s\"\n", errno, s );
		}
	}

	// If this was one of the seed critters and we're recording their functioning, then save the data in the appropriate directory
	// NOTE:  Must be done after c->Die(), as that is where the brainFunction file is closed
	if( fBrainFunctionRecordSeeds && (c->Number() <= fInitNumCritters) )
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
		!fBrainFunctionRecordAll )
	{
		char s[256];
		sprintf( s, "run/brain/function/brainFunction_%lu.txt", c->Number() );
		if( unlink( s ) )
			eprintf( "Error (%ld) unlinking \"%s\"\n", errno, s );
	}
	
	// following assumes (requires!) list to be currently pointing to c,
    // and will leave the list pointing to the previous critter
	// critter::gXSortedCritters.remove(); // get critter out of the list
	
	// Following assumes (requires!) the critter to have stored c->listLink correctly
	critter::gXSortedCritters.removeFastUnsafe( c->GetListLink() );
	
	// Note: For the sake of computational efficiency, I used to never delete a critter,
	// but "reinit" and reuse them as new critters were born or created.  But Gene made
	// critters be allocated afresh on birth or creation, so we now need to delete the
	// old ones here when they die.  Remove this if I ever get a chance to go back to the
	// more efficient reinit and reuse technique.
	delete c;
}


//-------------------------------------------------------------------------------------------
// TSimulation::ReadWorldFile
//-------------------------------------------------------------------------------------------
void TSimulation::ReadWorldFile(const char* filename)
{
    filebuf fb;
    if (fb.open(filename, ios_base::in) == 0)
    {
        error(0,"unable to open world file \"",filename,"\"");
        return;
    }
	
	istream in(&fb);
    short version;
    char label[64];

#define CurrentWorldfileVersion 15

    in >> version; in >> label;
	cout << "version" ses version nl;
	
	if( version > CurrentWorldfileVersion )

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
        return;
	}
	
	bool ignoreBool;
    in >> ignoreBool; in >> label;
    cout << "fDoCPUWork" ses ignoreBool nl;
    
    in >> fDumpFrequency; in >> label;
    cout << "fDumpFrequency" ses fDumpFrequency nl;
    in >> fStatusFrequency; in >> label;
    cout << "fStatusFrequency" ses fStatusFrequency nl;
    in >> globals::edges; in >> label;
    cout << "edges" ses globals::edges nl;

	// TODO figure out how to config critter using this
    in >> globals::wraparound; in >> label;
    cout << "wraparound" ses globals::wraparound nl;

    in >> critter::gVision; in >> label;    
    if (!fGraphics)
        critter::gVision = false;        
    cout << "vision" ses critter::gVision nl;
    in >> fShowVision; in >> label;
    if (!fGraphics)
        fShowVision = false;
    cout << "showvision" ses fShowVision nl;
    in >> brain::gMinWin; in >> label;
    cout << "minwin" ses brain::gMinWin nl;
    in >> critter::gMaxVelocity; in >> label;
    cout << "maxvel" ses critter::gMaxVelocity nl;
    in >> fMinNumCritters; in >> label;
    cout << "minnumcritters" ses fMinNumCritters nl;
    in >> fMaxCritters; in >> label;
    cout << "maxnumcritters" ses fMaxCritters nl;
    in >> fInitNumCritters; in >> label;
    cout << "initnumcritters" ses fInitNumCritters nl;
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
    in >> fMiscCritters; in >> label;
    cout << "misccritters" ses fMiscCritters nl;
    in >> fPositionSeed; in >> label;
    cout << "fPositionSeed" ses fPositionSeed nl;
    in >> fGenomeSeed; in >> label;
    cout << "genomeseed" ses fGenomeSeed nl;
    in >> fMinFoodCount; in >> label;
    cout << "minfoodcount" ses fMinFoodCount nl;
    in >> fMaxFoodCount; in >> label;
    cout << "maxfoodcount" ses fMaxFoodCount nl;
    in >> fMaxFoodGrown; in >> label;
    cout << "maxfoodgrown" ses fMaxFoodGrown nl;
    in >> fInitialFoodCount; in >> label;
    cout << "initfoodcount" ses fInitialFoodCount nl;
    in >> fFoodRate; in >> label;
    cout << "foodrate" ses fFoodRate nl;
    in >> fCrittersRfood; in >> label;
    cout << "crittersRfood" ses fCrittersRfood nl;
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
  	fTotalFitness = fEatFitnessParameter + fMateFitnessParameter + fMoveFitnessParameter + fEnergyFitnessParameter + fAgeFitnessParameter;
    in >> food::gMinFoodEnergy; in >> label;
    cout << "minfoodenergy" ses food::gMinFoodEnergy nl;
    in >> food::gMaxFoodEnergy; in >> label;
    cout << "maxfoodenergy" ses food::gMaxFoodEnergy nl;
    in >> food::gSize2Energy; in >> label;
    cout << "size2energy" ses food::gSize2Energy nl;
    in >> fEat2Consume; in >> label;
    cout << "eat2consume" ses fEat2Consume nl;
    
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
    cout << "minnumcpts" ses genome::gMinNumCpts nl;
    in >> genome::gMaxNumCpts; in >> label;
    cout << "maxnumcpts" ses genome::gMaxNumCpts nl;
    in >> genome::gMinLifeSpan; in >> label;
    cout << "minlifespan" ses genome::gMinLifeSpan nl;
    in >> genome::gMaxLifeSpan; in >> label;
    cout << "maxlifespan" ses genome::gMaxLifeSpan nl;
    in >> fMateWait; in >> label;
    cout << "matewait" ses fMateWait nl;
    in >> critter::gInitMateWait; in >> label;
    cout << "initmatewait" ses critter::gInitMateWait nl;
    in >> genome::gMinStrength; in >> label;
    cout << "minstrength" ses genome::gMinStrength nl;
    in >> genome::gMaxStrength; in >> label;
    cout << "maxstrength" ses genome::gMaxStrength nl;
    in >> critter::gMinCritterSize; in >> label;
    cout << "mincsize" ses critter::gMinCritterSize nl;
    in >> critter::gMaxCritterSize; in >> label;
    cout << "maxcsize" ses critter::gMaxCritterSize nl;
    in >> critter::gMinMaxEnergy; in >> label;
    cout << "minmaxenergy" ses critter::gMinMaxEnergy nl;
    in >> critter::gMaxMaxEnergy; in >> label;
    cout << "maxmaxenergy" ses critter::gMaxMaxEnergy nl;
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
    in >> critter::gSpeed2DPosition; in >> label;
    cout << "speed2dpos" ses critter::gSpeed2DPosition nl;
    in >> critter::gYaw2DYaw; in >> label;
    cout << "yaw2dyaw" ses critter::gYaw2DYaw nl;
    in >> genome::gMinlrate; in >> label;
    cout << "minlrate" ses genome::gMinlrate nl;
    in >> genome::gMaxlrate; in >> label;
    cout << "maxlrate" ses genome::gMaxlrate nl;
    in >> critter::gMinFocus; in >> label;
    cout << "minfocus" ses critter::gMinFocus nl;
    in >> critter::gMaxFocus; in >> label;
    cout << "maxfocus" ses critter::gMaxFocus nl;
    in >> critter::gCritterFOV; in >> label;
    cout << "critterfovy" ses critter::gCritterFOV nl;
    in >> critter::gMaxSizeAdvantage; in >> label;
    cout << "maxsizeadvantage" ses critter::gMaxSizeAdvantage nl;
    in >> fPower2Energy; in >> label;
    cout << "power2energy" ses fPower2Energy nl;
    in >> critter::gEat2Energy; in >> label;
    cout << "eat2energy" ses critter::gEat2Energy nl;
    in >> critter::gMate2Energy; in >> label;
    cout << "mate2energy" ses critter::gMate2Energy nl;
    in >> critter::gFight2Energy; in >> label;
    cout << "fight2energy" ses critter::gFight2Energy nl;
    in >> critter::gMaxSizePenalty; in >> label;
    cout << "maxsizepenalty" ses critter::gMaxSizePenalty nl;
    in >> critter::gSpeed2Energy; in >> label;
    cout << "speed2energy" ses critter::gSpeed2Energy nl;
    in >> critter::gYaw2Energy; in >> label;
    cout << "yaw2energy" ses critter::gYaw2Energy nl;
    in >> critter::gLight2Energy; in >> label;
    cout << "light2energy" ses critter::gLight2Energy nl;
    in >> critter::gFocus2Energy; in >> label;
    cout << "focus2energy" ses critter::gFocus2Energy nl;
    in >> brain::gNeuralValues.maxsynapse2energy; in >> label;
    cout << "maxsynapse2energy" ses brain::gNeuralValues.maxsynapse2energy nl;
    in >> critter::gFixedEnergyDrain; in >> label;
    cout << "fixedenergydrain" ses critter::gFixedEnergyDrain nl;
    in >> brain::gDecayRate; in >> label;
    cout << "decayrate" ses brain::gDecayRate nl;
    in >> fEatThreshold; in >> label;
    cout << "eatthreshold" ses fEatThreshold nl;
    in >> fMateThreshold; in >> label;
    cout << "matethreshold" ses fMateThreshold nl;
    in >> fFightThreshold; in >> label;
    cout << "fightthreshold" ses fFightThreshold nl;
    in >> genome::gMiscBias; in >> label;
    cout << "miscbias" ses genome::gMiscBias nl;
    in >> genome::gMiscInvisSlope; in >> label;
    cout << "miscinvslope" ses genome::gMiscInvisSlope nl;
    in >> brain::gLogisticsSlope; in >> label;
    cout << "logisticslope" ses brain::gLogisticsSlope nl;
    in >> brain::gMaxWeight; in >> label;
    cout << "maxweight" ses brain::gMaxWeight nl;
    in >> brain::gInitMaxWeight; in >> label;
    cout << "initmaxweight" ses brain::gInitMaxWeight nl;
    in >> genome::gMinBitProb; in >> label;
    cout << "minbitprob" ses genome::gMinBitProb nl;
    in >> genome::gMaxBitProb; in >> label;
    cout << "maxbitprob" ses genome::gMaxBitProb nl;
    in >> critter::gCritterHeight; in >> label;
    cout << "critterheight" ses critter::gCritterHeight nl;
    in >> food::gFoodHeight; in >> label;
    cout << "foodheight" ses food::gFoodHeight nl;
    in >> food::gFoodColor.r >> food::gFoodColor.g >> food::gFoodColor.b >> label;
    cout << "foodcolor = (" << food::gFoodColor.r cms
                               food::gFoodColor.g cms
                               food::gFoodColor.b pnl;
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
    in >> fCameraAngleStart; in >> label;
    cout << "camanglestart" ses fCameraAngleStart nl;
    in >> fCameraFOV; in >> label;
    cout << "camfov" ses fCameraFOV nl;
    in >> fMonitorCritterRank; in >> label;
    if (!fGraphics)
        fMonitorCritterRank = 0; // cannot monitor critter brain without graphics
    cout << "fMonitorCritterRank" ses fMonitorCritterRank nl;
    
    in >> ignoreShort3; in >> label;
    cout << "fMonitorCritWinWidth (ignored)" ses ignoreShort3 nl;
    in >> ignoreShort4; in >> label;
    cout << "fMonitorCritWinHeight (ignored)" ses ignoreShort4 nl;
    
    in >> fBrainMonitorStride; in >> label;
    cout << "fBrainMonitorStride" ses fBrainMonitorStride nl;
    in >> globals::worldsize; in >> label;
    cout << "worldsize" ses globals::worldsize nl;
    long numbarriers;
    float x1; float z1; float x2; float z2;
    in >> numbarriers; in >> label;
    cout << "numbarriers = " << numbarriers nl;
    for (long i = 0; i < numbarriers; i++)
    {
        in >> x1 >> z1 >> x2 >> z2 >> label;
        cout << "barrier #" << i << " position = ("
             << x1 cms z1 << ") to (" << x2 cms z2 pnl;
		barrier::gXSortedBarriers.add(new barrier(x1, z1, x2, z2));
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
   	fDomains[0].xleft = 0.0;
    for (id = 0; id < fNumDomains - 1; id++)
    {
        in >> fDomains[id].xright; in >> label;
        cout << "barrier between domains " << id << " and " << (id+1)
             << " is at x = " << fDomains[id].xright nl;
        fDomains[id].xsize = fDomains[id].xright - fDomains[id].xleft;
        fDomains[id+1].xleft = fDomains[id].xright;
    }
    fDomains[id].xright = globals::worldsize;
    fDomains[id].xsize = fDomains[id].xright - fDomains[id].xleft;

    if (fNumDomains > 1)
    {
        long totmaxnumcritters = 0;
        long totminnumcritters = 0;
        for (id = 0; id < fNumDomains; id++)
        {
            in >> fDomains[id].minnumcritters; in >> label;
            cout << "minnumcritters in domains[" << id << "]" ses fDomains[id].minnumcritters nl;
            in >> fDomains[id].maxnumcritters; in >> label;
            cout << "maxnumcritters in domains[" << id << "]" ses fDomains[id].maxnumcritters nl;
            in >> fDomains[id].initnumcritters; in >> label;
            cout << "initnumcritters in domains[" << id << "]" ses fDomains[id].initnumcritters nl;
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
            in >> fDomains[id].minfoodcount; in >> label;
            cout << "minfoodcount in domains[" << id << "]" ses fDomains[id].minfoodcount nl;
            in >> fDomains[id].maxfoodcount; in >> label;
            cout << "maxfoodcount in domains[" << id << "]" ses fDomains[id].maxfoodcount nl;
            in >> fDomains[id].maxfoodgrown; in >> label;
            cout << "maxfoodgrown in domains[" << id << "]" ses fDomains[id].maxfoodgrown nl;
            in >> fDomains[id].initfoodcount; in >> label;
            cout << "initfoodcount in domains[" << id << "]" ses fDomains[id].initfoodcount nl;
            totmaxnumcritters += fDomains[id].maxnumcritters;
            totminnumcritters += fDomains[id].minnumcritters;
        }
        if (totmaxnumcritters > fMaxCritters)
        {
			char tempstring[256];
            sprintf(tempstring,"%s %ld %s %ld %s",
                "The maximum number of organisms in the world (",
                fMaxCritters,
                ") is < the maximum summed over domains (",
				totmaxnumcritters,
                "), so there may still be some indirect global influences.");
            errorflash(0, tempstring);
        }
        if (totminnumcritters < fMinNumCritters)
        {
			char tempstring[256];
            sprintf(tempstring,"%s %ld %s %ld %s",
                "The minimum number of organisms in the world (",
                fMinNumCritters,
                ") is > the minimum summed over domains (",
                totminnumcritters,
                "), so there may still be some indirect global influences.");
            errorflash(0,tempstring);
        }
    }
    else
    {
        fDomains[0].minnumcritters = fMinNumCritters;
        fDomains[0].maxnumcritters = fMaxCritters;
        fDomains[0].initnumcritters = fInitNumCritters;
        fDomains[0].minfoodcount = fMinFoodCount;
        fDomains[0].maxfoodcount = fMaxFoodCount;
        fDomains[0].maxfoodgrown = fMaxFoodGrown;
        fDomains[0].initfoodcount = fInitialFoodCount;
		fDomains[0].numberToSeed = fNumberToSeed;
		fDomains[0].probabilityOfMutatingSeeds = fProbabilityOfMutatingSeeds;
    }

    for (id = 0; id < fNumDomains; id++)
    {
        fDomains[id].numcritters = 0;
        fDomains[id].numcreated = 0;
        fDomains[id].numborn = 0;
        fDomains[id].numbornsincecreated = 0;
        fDomains[id].numdied = 0;
        fDomains[id].lastcreate = 0;
        fDomains[id].maxgapcreate = 0;
        fDomains[id].foodcount = 0;
        fDomains[id].ifit = 0;
        fDomains[id].jfit = 1;
        fDomains[id].fittest = NULL;
    }
	
	if( version >= 10 )
	{
		in >> food::gNumFoodBands; in >> label;
		cout << "numFoodBands" ses food::gNumFoodBands nl;
		
		food::gFoodBand = (FoodBand*) malloc( sizeof(FoodBand) * food::gNumFoodBands );
		Q_CHECK_PTR( food::gFoodBand );
		
		food::gFoodBandZTotal = 0.0;
		for( int i = 0; i < food::gNumFoodBands; i++ )
		{
			in >> food::gFoodBand[i].zMin; in >> label;
			cout << "FoodBand[" << i << "].zMin" ses food::gFoodBand[i].zMin nl;
			in >> food::gFoodBand[i].zMax; in >> label;
			cout << "FoodBand[" << i << "].zMax" ses food::gFoodBand[i].zMax nl;
			
			food::gFoodBandZTotal += food::gFoodBand[i].zMax - food::gFoodBand[i].zMin;
		}
	}
	else
	{
		food::gNumFoodBands = 1;
		cout << "_numFoodBands" ses food::gNumFoodBands nl;
		
		food::gFoodBand = (FoodBand*) malloc( sizeof(FoodBand) * food::gNumFoodBands );
		Q_CHECK_PTR( food::gFoodBand );
		
		food::gFoodBand[0].zMin = -1.0;
		food::gFoodBand[0].zMax = 0.0;
		cout << "_FoodBand[0].zMin" ses food::gFoodBand[0].zMin;
		cout << "_FoodBand[0].zMax" ses food::gFoodBand[0].zMax;
		
		food::gFoodBandZTotal = 1.0;
	}

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
    in >> brain::gNeuralValues.maxneuron2energy; in >> label;
    cout << "maxneuron2energy" ses brain::gNeuralValues.maxneuron2energy nl;
    
    in >> brain::gNumPrebirthCycles; in >> label;
    cout << "numprebirthcycles" ses brain::gNumPrebirthCycles nl;

    InitNeuralValues();  

    cout << "maxneurgroups" ses brain::gNeuralValues.maxneurgroups nl;
    cout << "maxneurons" ses brain::gNeuralValues.maxneurons nl;
    cout << "maxsynapses" ses brain::gNeuralValues.maxsynapses nl;

    in >> fOverHeadRank; in >> label;
    cout << "fOverHeadRank" ses fOverHeadRank nl;
    in >> fCritterTracking; in >> label;
    cout << "fCritterTracking" ses fCritterTracking nl;
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
	
    in >> fSmiteFrac; in >> label;
    cout << "smiteFrac" ses fSmiteFrac nl;
    in >> fSmiteAgeFrac; in >> label;
    cout << "smiteAgeFrac" ses fSmiteAgeFrac nl;

	if( version < 8 )
	{
		cout nlf;
		fb.close();
		return;
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

	if( version >= 15 )
	{
		in >> fRecordFoodBandStats; in >> label;
		cout << "recordFoodBandStats" ses fRecordFoodBandStats nl;
	}
	
	in >> fRecordMovie; in >> label;
	cout << "recordMovie" ses fRecordMovie nl;

	cout nlf;
    fb.close();
	return;
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
    out << fNumberFights nl;
    out << fMiscDenials nl;
    out << fLastCreated nl;
    out << fMaxGapCreate nl;
    out << fNumBornSinceCreated nl;

    out << fMonitorCritterRank nl;
    out << fMonitorCritterRankOld nl;
    out << fCritterTracking nl;
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

    critter::critterdump(out);

    out << food::gXSortedFood.count() nl;
	food* f = NULL;
	food::gXSortedFood.reset();
	while (food::gXSortedFood.next(f))
		f->dump(out);
					
    out << fNumberFit nl;
    out << fFitI nl;
    out << fFitJ nl;
    for (int index = 0; index < fNumberFit; index++)
    {
		out << fFittest[index]->critterID nl;
        out << fFittest[index]->fitness nl;
        fFittest[index]->genes->Dump(out);
    }
	out << fNumberRecentFit nl;
    for (int index = 0; index < fNumberRecentFit; index++)
    {
		out << fRecentFittest[index]->critterID nl;
		out << fRecentFittest[index]->fitness nl;
//		fRecentFittest[index]->genes->Dump(out);	// we don't save the genes in the bestRecent list
    }

    out << fNumDomains nl;
    for (int id = 0; id < fNumDomains; id++)
    {
        out << fDomains[id].numcritters nl;
        out << fDomains[id].numcreated nl;
        out << fDomains[id].numborn nl;
        out << fDomains[id].numbornsincecreated nl;
        out << fDomains[id].numdied nl;
        out << fDomains[id].lastcreate nl;
        out << fDomains[id].maxgapcreate nl;
        out << fDomains[id].foodcount nl;
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
				out << fDomains[id].fittest[i]->critterID nl;
                out << fDomains[id].fittest[i]->fitness nl;
                fDomains[id].fittest[i]->genes->Dump(out);
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
// TSimulation::DomainError
//---------------------------------------------------------------------------
static int DomainError(float x, float z, short d, short nd)
{
	char errorString[256];
	sprintf(errorString,"%s (%g, %g) %s %d, %d",
			"WhichDomain failed to find any domain for point at (x, z) = ",
			x, z, " & d, nd = ", d, nd);
	error(2, errorString);
	
	return -1; // not really returning, as error(2,...) will abort
}


//---------------------------------------------------------------------------
// TSimulation::WhichDomain
//---------------------------------------------------------------------------
short TSimulation::WhichDomain(float x, float z, short d)
{
	if (((x >= fDomains[d].xleft) && (x <= fDomains[d].xright))
		|| ((d == 0) && (x <= fDomains[d].xright))
		|| ((d == fNumDomains-1) && (x >= fDomains[d].xleft)))
		return d;
		
	short nd = d;
	for (short i = 0; i < fNumDomains - 1; i++)
	{
		nd++;
		
		if (nd > (fNumDomains - 1))
			nd = 0;
			
		if (((x >= fDomains[nd].xleft) && (x <= fDomains[nd].xright))
			|| ((nd == 0) && (x <= fDomains[nd].xright))
			|| ((nd == fNumDomains - 1) && (x >= fDomains[nd].xleft)) )
			return nd;
	}
		
	return DomainError(x, z, d, nd);
}


//---------------------------------------------------------------------------
// TSimulation::SwitchDomain
//
//	No verification is done. Assume caller has verified domains.
//---------------------------------------------------------------------------
void TSimulation::SwitchDomain(short newDomain, short oldDomain)
{
	Q_ASSERT(newDomain != oldDomain);
		
	fDomains[newDomain].numcritters++;
	fDomains[oldDomain].numcritters--;
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
	
	sprintf( t, "critters = %4ld", critter::gXSortedCritters.count() );
	if (fNumDomains > 1)
	{
		sprintf(t2, " (%ld",fDomains[0].numcritters );
		strcat(t, t2 );
		for (id = 1; id < fNumDomains; id++)
		{
			sprintf(t2, ", %ld", fDomains[id].numcritters );
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
	
	sprintf( t, " -edge   = %4ld", fNumberDiedEdge );
	list.push_back( strdup( t ) );

	sprintf( t, " -smite  = %4ld", fNumberDiedSmite );
	list.push_back( strdup( t ) );
	
	sprintf( t, "food = %ld", food::gXSortedFood.count() );
	if (fNumDomains > 1)
	{
	    sprintf( t2, " (%ld",fDomains[0].foodcount );
	    strcat( t, t2 );
	    for (id = 1; id < fNumDomains; id++)
	    {
	        sprintf( t2, ",%ld",fDomains[id].foodcount );
	        strcat( t, t2 );
	    }
	    strcat(t,")" );
	}
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

	sprintf( t, "born/total = %.2f", float(fNumberBorn) / float(fNumberCreated + fNumberBorn) );
	list.push_back( strdup( t ) );

	sprintf( t, "Fitness m=%.2f, c=%.2f, a=%.2f", fMaxFitness, fCurrentMaxFitness[0], fAverageFitness );
	list.push_back( strdup( t ) );
	
	sprintf( t, "NormFit m=%.2f, c=%.2f, a=%.2f", fMaxFitness / fTotalFitness, fCurrentMaxFitness[0] / fTotalFitness, fAverageFitness / fTotalFitness );
	list.push_back( strdup( t ) );
	
	sprintf( t, "Fittest =" );
	int fittestCount = min( 5, min( fNumberFit, (int) fNumberDied ));
	for( int i = 0; i < fittestCount; i++ )
	{
		sprintf( t2, " %lu", fFittest[i]->critterID );
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
		sprintf( t2, " %lu", fCurrentFittestCritter[i]->Number() );
		strcat( t, t2 );
	}
	list.push_back( strdup( t ) );
	
	if( fCurrentFittestCount > 0 )
	{
		sprintf( t, " " );
		for( int i = 0; i < fCurrentFittestCount; i++ )
		{
			sprintf( t2, "  %.2f", fCurrentFittestCritter[i]->Fitness() );
			strcat( t, t2 );
		}
		list.push_back( strdup( t ) );
	}
	
	sprintf( t, "avgFoodEnergy = %.2f", (fAverageFoodEnergyIn - fAverageFoodEnergyOut) / (fAverageFoodEnergyIn + fAverageFoodEnergyOut) );
	list.push_back( strdup( t ) );
	
	sprintf( t, "totFoodEnergy = %.2f", (fTotalFoodEnergyIn - fTotalFoodEnergyOut) / (fTotalFoodEnergyIn + fTotalFoodEnergyOut) );
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

	sprintf( t, "NeurGroups = %.1f ± %.1f [%lu, %lu]", fNeuronGroupCountStats.mean(), fNeuronGroupCountStats.stddev(), (ulong) fNeuronGroupCountStats.min(), (ulong) fNeuronGroupCountStats.max() );
	list.push_back( strdup( t ) );

	sprintf( t, "CurNeurGroups = %.1f ± %.1f [%lu, %lu]", fCurrentNeuronGroupCountStats.mean(), fCurrentNeuronGroupCountStats.stddev(), (ulong) fCurrentNeuronGroupCountStats.min(), (ulong) fCurrentNeuronGroupCountStats.max() );
	list.push_back( strdup( t ) );

	sprintf( t, "Rate %2.1f (%2.1f) %2.1f (%2.1f) %2.1f (%2.1f)",
				fFramesPerSecondInstantaneous, fSecondsPerFrameInstantaneous,
				fFramesPerSecondRecent,        fSecondsPerFrameRecent,
				fFramesPerSecondOverall,       fSecondsPerFrameOverall  );
	list.push_back( strdup( t ) );
	
	if( fRecordFoodBandStats && (food::gNumFoodBands > 0) )
	{
		unsigned long numCrittersInAnyFoodBand = 0;
		unsigned long numCrittersWithin5UnitsOfAnyFoodBand = 0;
		unsigned long numCrittersWithin10UnitsOfAnyFoodBand = 0;
		float makePercent = 100.0 / critter::gXSortedCritters.count();

		for( int i = 0; i < food::gNumFoodBands; i++ )
		{
			sprintf( t, "FB%d %3lu %3lu %3lu  %4.1f %4.1f %4.1f",
						i,
						fNumCrittersInFoodBand[i],
						fNumCrittersInFoodBand[i] + fNumCrittersWithin5UnitsOfFoodBand[i],
						fNumCrittersInFoodBand[i] + fNumCrittersWithin5UnitsOfFoodBand[i] + fNumCrittersWithin10UnitsOfFoodBand[i],
						fNumCrittersInFoodBand[i] * makePercent,
					   (fNumCrittersInFoodBand[i] + fNumCrittersWithin5UnitsOfFoodBand[i]) * makePercent,
					   (fNumCrittersInFoodBand[i] + fNumCrittersWithin5UnitsOfFoodBand[i] + fNumCrittersWithin10UnitsOfFoodBand[i]) * makePercent );
			list.push_back( strdup( t ) );
			
			numCrittersInAnyFoodBand += fNumCrittersInFoodBand[i];
			numCrittersWithin5UnitsOfAnyFoodBand += fNumCrittersWithin5UnitsOfFoodBand[i];
			numCrittersWithin10UnitsOfAnyFoodBand += fNumCrittersWithin10UnitsOfFoodBand[i];
		}

		sprintf( t, "FB* %3lu %3lu %3lu  %4.1f %4.1f %4.1f",
					numCrittersInAnyFoodBand,
					numCrittersInAnyFoodBand + numCrittersWithin5UnitsOfAnyFoodBand,
					numCrittersInAnyFoodBand + numCrittersWithin5UnitsOfAnyFoodBand + numCrittersWithin10UnitsOfAnyFoodBand,
					numCrittersInAnyFoodBand * makePercent,
				   (numCrittersInAnyFoodBand + numCrittersWithin5UnitsOfAnyFoodBand) * makePercent,
				   (numCrittersInAnyFoodBand + numCrittersWithin5UnitsOfAnyFoodBand + numCrittersWithin10UnitsOfAnyFoodBand) * makePercent );
		list.push_back( strdup( t ) );
	}
	
    if( !(fStep % fStatusFrequency) || (fStep == 1) )
    {
		char statusFileName[256];
		
		sprintf( statusFileName, "run/stats/stat.%ld", fStep );
        FILE *statusFile = fopen( statusFileName, "w" );
		Q_CHECK_PTR( statusFile );
		
		TStatusList::const_iterator iter = list.begin();
		for( ; iter != list.end(); ++iter )
			fprintf( statusFile, "%s\n", *iter );

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

	// Update critter POV window
	if (fShowVision && fCritterPOVWindow != NULL && !fCritterPOVWindow->isHidden())
		fCritterPOVWindow->updateGL();
		//QApplication::postEvent(fCritterPOVWindow, new QCustomEvent(kUpdateEventType));	
	
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


