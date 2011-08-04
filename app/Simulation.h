#ifndef SIMULATION_H
#define SIMULATION_H

#ifdef linux
	#include <errno.h>
#endif

#include <string>

// qt
#include <qobject.h>
#include <qapplication.h>
#include <qmainwindow.h>
#include <qmenu.h>

// Local
#include "agent.h"
#include "barrier.h"
#include "datalib.h"
#include "EatStatistics.h"
#include "food.h"
#include "Scheduler.h"
#include "SeparationCache.h"
#include "gmisc.h"
#include "graphics.h"
#include "gstage.h"
#include "TextStatusWindow.h"
#include "OverheadView.h"

// Forward declarations
namespace condprop
{
	class PropertyList;
}
namespace proplib
{
	class Document;
}
namespace genome
{
	class Genome;
}
class ContactEntry;
class TBinChartWindow;
class TBrainMonitorWindow;
class TChartWindow;
class TSceneView;
class TOverheadView;
class TAgentPOVWindow;
//class QMenuBar;
//class QTimer;
class TSimulation;

static const int MAXDOMAINS = 10; // if you change this, you MUST change the schema file.
static const int MAXFITNESSITEMS = 5;

// Define file mode mask so that users can read+write, group and others can read (but not write)
#define	PwFileMode ( S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH )
// Define directory mode mask the same, except you need execute privileges to use as a directory (go fig)
#define	PwDirMode ( S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH )


// Used in logic related to Mating as well as ContactEntry
#define MATE__NIL						0
#define MATE__DESIRED					(1 << 0)
#define MATE__PREVENTED__PARTNER		(1 << 1)
#define MATE__PREVENTED__CARRY			(1 << 2)
#define MATE__PREVENTED__MATE_WAIT		(1 << 3)
#define MATE__PREVENTED__ENERGY			(1 << 4)
#define MATE__PREVENTED__EAT_MATE_SPAN	(1 << 5)
#define MATE__PREVENTED__EAT_MATE_MIN_DISTANCE	(1 << 6)
#define MATE__PREVENTED__OF1			(1 << 7)
#define MATE__PREVENTED__MAX_DOMAIN		(1 << 8)
#define MATE__PREVENTED__MAX_WORLD		(1 << 9)
#define MATE__PREVENTED__MISC			(1 << 10)
#define MATE__PREVENTED__MAX_VELOCITY	(1 << 11)


// Used in logic related to Fighting as well as ContactEntry
#define FIGHT__NIL						0
#define FIGHT__DESIRED					(1 << 0)
#define FIGHT__PREVENTED__CARRY			(1 << 1)
#define FIGHT__PREVENTED__SHIELD		(1 << 2)
#define FIGHT__PREVENTED__POWER			(1 << 3)


// Used in logic related to Giving as well as ContactEntry
#define GIVE__NIL						0
#define GIVE__DESIRED					(1 << 0)
#define GIVE__PREVENTED__CARRY			(1 << 1)
#define GIVE__PREVENTED__ENERGY			(1 << 2)

struct Position
{
	float x;
	float y;
	float z;
};

struct FitStruct
{
	ulong	agentID;
	float	fitness;
	float   complexity;
	genome::Genome*genes;
};
typedef struct FitStruct FitStruct;


//===========================================================================
// Domain
//===========================================================================
class Domain
{
 public:
	Domain();
	~Domain();
	
    float centerX;
    float centerZ;
    float absoluteSizeX;
    float absoluteSizeZ;
    float sizeX;
    float sizeZ;
    float startX;
    float startZ;
    float endX;
    float endZ;
    int numFoodPatches;
    int numBrickPatches;
    float fraction;
    float foodRate;
    int foodCount;
    int initFoodCount;
    int minFoodCount;
    int maxFoodCount;
    int maxFoodGrownCount;
	int numFoodPatchesGrown;
	FoodPatch* fFoodPatches;
	BrickPatch* fBrickPatches;
    long minNumAgents;
    long maxNumAgents;
    long initNumAgents;
	long numberToSeed;
    long numAgents;
    long numcreated;
    long numborn;
    long numbornsincecreated;
    long numdied;
    long lastcreate;
    long maxgapcreate;
	long numToCreate;
	float probabilityOfMutatingSeeds;
    short ifit;
    short jfit;
    FitStruct** fittest;	// based on complete fitness, however it is being calculated in AgentFitness(c)
	int fNumLeastFit;
	int fMaxNumLeastFit;
	int fNumSmited;
	agent** fLeastFit;	// based on heuristic fitness

	FoodPatch* whichFoodPatch( float x, float z );
};

inline Domain::Domain()
{
	foodCount = 0;
}

inline Domain::~Domain()
{
}

inline FoodPatch* Domain::whichFoodPatch( float x, float z )
{
	FoodPatch* fp = NULL;
	
	for( int i = 0; i < numFoodPatches; i++ )
	{
		if( fFoodPatches[i].pointIsInside( x, z, 0.0 ) )
		{
			fp = &(fFoodPatches[i]);
			break;
		}
	}
	
	return( fp );
}


//===========================================================================
// Stat
//===========================================================================
class Stat
{
public:
	Stat()					{ reset(); }
	~Stat()					{}
	
	float	mean()			{ if( !count ) return( 0.0 ); return( sum / count ); }
	float	stddev()		{ if( !count ) return( 0.0 ); register double m = sum / count;  return( sqrt( sum2 / count  -  m * m ) ); }
	float	min()			{ if( !count ) return( 0.0 ); return( mn ); }
	float	max()			{ if( !count ) return( 0.0 ); return( mx ); }
	void	add( float v )	{ sum += v; sum2 += v*v; count++; mn = v < mn ? v : mn; mx = v > mx ? v : mx; }
	void	reset()			{ mn = FLT_MAX; mx = FLT_MIN; sum = sum2 = count = 0; }
	unsigned long samples() { return( count ); }

private:
	float	mn;		// minimum
	float	mx;		// maximum
	double	sum;	// sum
	double	sum2;	// sum of squares
	unsigned long	count;	// count
};

//===========================================================================
// StatRecent
//===========================================================================
class StatRecent
{
public:
	StatRecent( unsigned int width = 1000 )	{ w = width; history = (float*) malloc( w * sizeof(*history) ); Q_CHECK_PTR( history); reset(); }
	~StatRecent()							{ if( history ) free( history ); }
	
	float	mean()			{ if( !count ) return( 0.0 ); return( sum / count ); }
	float	stddev()		{ if( !count ) return( 0.0 ); register double m = sum / count;  return( sqrt( sum2 / count  -  m * m ) ); }
	float	min()			{ if( !count ) return( 0.0 ); if( needMin ) recomputeMin(); return( mn ); }
	float	max()			{ if( !count ) return( 0.0 ); if( needMax ) recomputeMax(); return( mx ); }
	void	add( float v )	{ if( count < w ) { sum += v; sum2 += v*v; mn = v < mn ? v : mn; mx = v > mx ? v : mx; history[index++] = v; count++; } else { if( index >= w ) index = 0; sum += v - history[index]; sum2 += v*v - history[index]*history[index]; if( v >= mx ) mx = v; else if( history[index] == mx ) needMax = true; if( v <= mn ) mn = v; else if( history[index] == mn ) needMin = true; history[index++] = v; } }
	void	reset()			{ mn = FLT_MAX; mx = FLT_MIN; sum = sum2 = count = index = 0; needMin = needMax = false; }
	unsigned long samples() { return( count ); }

private:
	float	mn;		// minimum
	float	mx;		// maximum
	double	sum;	// sum
	double	sum2;	// sum of squares
	unsigned long	count;	// count
	unsigned int	w;		// width of data window considered current (controls roll-off)
	float*	history;	// w recent samples
	unsigned int	index;	// point in history[] at which next sample is to be inserted
	bool	needMin;
	bool	needMax;
	
	void	recomputeMin()	{ mn = FLT_MAX; for( unsigned int i = 0; i < w; i++ ) mn = history[i] < mn ? history[i] : mn; needMin = false; }
	void	recomputeMax()	{ mx = FLT_MIN; for( unsigned int i = 0; i < w; i++ ) mx = history[i] > mx ? history[i] : mx; needMax = false; }
};

//===========================================================================
// TSceneWindow
//===========================================================================
class TSceneWindow: public QMainWindow
{
	Q_OBJECT

public:
	TSceneWindow( const char *worldfilePath );
	virtual ~TSceneWindow();
	
	void CreateSimulationScheduler();
	void Update();
	TSimulation *GetSimulation();

protected:
    void closeEvent(QCloseEvent* event);    
    virtual void keyReleaseEvent( QKeyEvent* event ); //Moved here from SceneView (CMB 3/26/06)
    
	void AddFileMenu();
	void AddEditMenu();
	void AddRunMenu();
	void AddWindowMenu();
	void AddHelpMenu();
	
	void SaveWindowState();
	void SaveDimensions();
	void SaveVisibility();
	void RestoreFromPrefs();

	bool exitOnUserConfirm();

public slots:
    void timeStep();

private slots:

	// File menu
    void choose() {}
    void save() {}
    void saveAs() {}
    void about() {}    
    void windowsMenuAboutToShow();

	// Run menu
	void endAtTimestep();
	void endNow();
    
    // Window menu
    void ToggleEnergyWindow();
    void ToggleFitnessWindow();
    void TogglePopulationWindow();
    void ToggleBirthrateWindow();
    void ToggleOverheadWindow();
	void ToggleBrainWindow();
	void TogglePOVWindow();
	void ToggleTextStatus();
	void TileAllWindows();
    
private:
	QMenuBar* fMenuBar;
	QMenu* fWindowsMenu;
	
	TSceneView* fSceneView;
	TOverheadView* fOverheadWindow; //CMB 3/17/06
	TSimulation* fSimulation;
	
	QAction* toggleEnergyWindowAct;
	QAction* toggleFitnessWindowAct;
	QAction* togglePopulationWindowAct;
	QAction* toggleBirthrateWindowAct;
	QAction* toggleBrainWindowAct;
	QAction* togglePOVWindowAct;
	QAction* toggleTextStatusAct;
	QAction* toggleOverheadWindowAct;
	QAction* tileAllWindowsAct;

	QString windowSettingsName;
};

//===========================================================================
// ObjectType
//===========================================================================
enum ObjectType
{
	OT_AGENT = 0,
	OT_FOOD,
	OT_BRICK,
	OT_BARRIER,
	OT_EDGE
};

//===========================================================================
// TSimulation
//===========================================================================
class TSimulation : public QObject
{
	Q_OBJECT

public:
	TSimulation( TSceneView* sceneView, TSceneWindow* sceneWindow, const char *worldfilePath );
	virtual ~TSimulation();
	
	void Start();
	void Stop();
	void Pause();
	
	short WhichDomain( float x, float z, short d );
	void SwitchDomain( short newDomain, short oldDomain, int objectType );
	
	gcamera& GetCamera();
	gcamera& GetOverheadCamera();

	TChartWindow* GetBirthrateWindow() const;
	TChartWindow* GetFitnessWindow() const;
	TChartWindow* GetEnergyWindow() const;
	TChartWindow* GetPopulationWindow() const;	
	TBrainMonitorWindow* GetBrainMonitorWindow() const;	
	TBinChartWindow* GetGeneSeparationWindow() const;
	TAgentPOVWindow* GetAgentPOVWindow() const;
	TTextStatusWindow* GetStatusWindow() const;
	TOverheadView*   GetOverheadWindow() const;
	
//	bool GetShowBrain() const;
//	void SetShowBrain(bool showBrain);
	long GetMaxAgents() const;
	static long fMaxNumAgents;
	long GetInitNumAgents() const;
	
//	short OverHeadRank( void );
	
	void PopulateStatusList(TStatusList& list);

	long GetMaxSteps() const;
	
	void Step();
	void End( const std::string &reason );
	std::string EndAt( long timestep );
	void Update();

	bool fLockStepWithBirthsDeathsLog;	// Are we running in lockstep mode?
	FILE * fLockstepFile;				// Define a file pointer to our LOCKSTEP-BirthsDeaths.log
	int fLockstepTimestep;				// Timestep at which the next event in LOCKSTEP-BirthDeaths.log occurs
	int fLockstepNumDeathsAtTimestep;	// How many agents died at this LockstepTimestep?
	int fLockstepNumBirthsAtTimestep;	// how many agents were born at LockstepTimestep?
	void SetNextLockstepEvent();		// function to read the next event from LOCKSTEP-BirthsDeaths.log


	static long fStep;
	bool fAgentTracking;		//Moving to public for access to sceneview and keypress (CMB 3/19/06)
	long fMonitorAgentRank;	//Moving to public for access to sceneview and keypress (CMB 3/19/06)
	long fMonitorAgentRankOld;
	bool fRotateWorld;		//Turn world rotation on or off (CMB 3/19/06)
	static short fOverHeadRank;    	
	static agent* fMonitorAgent;	
	static double fFramesPerSecondOverall;
	static double fSecondsPerFrameOverall;
	static double fFramesPerSecondRecent;
	static double fSecondsPerFrameRecent;
	static double fFramesPerSecondInstantaneous;
	static double fSecondsPerFrameInstantaneous;
	static double fTimeStart;

	int fBestSoFarBrainAnatomyRecordFrequency;
	int fBestSoFarBrainFunctionRecordFrequency;
	int fBestRecentBrainAnatomyRecordFrequency;
	int fBestRecentBrainFunctionRecordFrequency;
	
	bool fRecordBirthsDeaths;

	DataLibWriter *fLifeSpanLog;

	bool fRecordPosition;
	bool fRecordBarrierPosition;
	bool fRecordContacts;
	DataLibWriter *fContactsLog;
	bool fRecordCollisions;
	DataLibWriter *fCollisionsLog;
	bool fRecordCarry;
	DataLibWriter *fCarryLog;
	bool fRecordEnergy;
	DataLibWriter *fEnergyLog;

	bool fBrainAnatomyRecordAll;
	bool fBrainFunctionRecordAll;
	bool fBrainAnatomyRecordSeeds;
	bool fBrainFunctionRecordSeeds;
	
	bool fApplyLowPopulationAdvantage;
	
	bool fRecordComplexity;				// record the Olaf Functional Complexity (neural)

	bool fRecordGenomes;
	bool fRecordSeparations;
	DataLibWriter *fSeparationsLog;
	bool fRecordAdamiComplexity;		// record the Adami Physical Complexity  (genetic)
	int fAdamiComplexityRecordFrequency;
	
	float EnergyFitnessParameter() const;
	float AgeFitnessParameter() const;
	float LifeFractionRecent();
	unsigned long LifeFractionSamples();  // the number of samples upon which LifeFractionRecent() is based

	char glFogFunction();
	float glExpFogDensity();
	int glLinearFogEnd();


	float GetAgentHealingRate();				// Virgil Healing

	int getRandomPatch(int domainNumber);
	Domain fDomains[MAXDOMAINS];	

private slots:
	
private:
	void Init( const char *worldfilePath );
	void InitAgents();
	void InitNeuralValues();
	void InitWorld();
	void InitMonitoringWindows();

	void SeedGenome( long agentNumber,
					 genome::Genome *genes,
					 float probabilityOfMutatingSeeds,
					 long numSeeded );
	void SeedGenomeFromFile( long agentNumber,
							 genome::Genome *genes,
							 long numSeeded );
	void ReadSeedFilePaths();

	void SetSeedPosition( agent *a,
						  long numSeeded,
						  float x, float y, float z );
	void ReadSeedPositionsFromFile();

	void InitLifeSpanLog();
	void UpdateLifeSpanLog( agent *a );
	void EndLifeSpanLog();

	void InitContactsLog();
	void EndContactsLog();

	void InitCollisionsLog();
 public:
	void UpdateCollisionsLog( agent *c, ObjectType ot );
 private:
	void EndCollisionsLog();

	void InitCarryLog();
 public:
	enum CarryAction { CA__PICKUP = 0, CA__DROP_RECENT, CA__DROP_OBJECT };
	void UpdateCarryLog( agent *c, gobject *obj, CarryAction action );
 private:
	void EndCarryLog();

	void InitEnergyLog();
	enum EnergyLogEventType { ELET__GIVE = 0, ELET__FIGHT };
	void UpdateEnergyLog( agent *c,
						  gobject *obj,
						  float neuralActivation,
						  float energy,
						  EnergyLogEventType elet );
	void EndEnergyLog();

	void InitSeparationsLog();
	void EndSeparationsLog();
	
	void PickParentsUsingTournament(int numInPool, int* iParent, int* jParent);
	void UpdateAgents();
	void UpdateAgents_StaticTimestepGeometry();

	void Interact();
	void DeathAndStats();
	void MateLockstep();
	int GetMatePotential( agent *x );
	int GetMateStatus( int xPotential, int yPotential );
	int GetMateDenialStatus( agent *x, int *xStatus,
							 agent *y, int *yStatus,
							 short domainID );
	void Mate( agent *c,
			   agent *d,
			   ContactEntry *contactEntry );
	void Smite( short kd,
				agent *c,
				agent *d );
	int GetFightStatus( agent *x,
						agent *y,
						float *out_power );
	void Fight( agent *c,
				agent *d,
				ContactEntry *contactEntry,
				bool *cDied,
				bool *dDied );
	int GetGiveStatus( agent *x,
					   float *out_energy );
	void Give( agent *x,
			   agent *y,
			   ContactEntry *contactEntry,
			   bool *xDied,
			   bool toMarkOnDeath );
	void Eat( agent *c );
	void Carry( agent *c );
	void Pickup( agent *c );
	void Drop( agent *c );
	void Fitness( agent *c );
	void CreateAgents();
	void MaintainBricks();
	void MaintainFood();
	
	void RecordGeneSeparation();
	void CalculateGeneSeparation(agent* ci);
	void CalculateGeneSeparationAll();

	// Following two functions only determine whether or not we should create the relevant files.
	// Linking, renaming, and unlinking are handled according to the specific recording options.
	bool RecordBrainAnatomy( long agentNumber );
	bool RecordBrainFunction( long agentNumber );
	
	void ijfitinc(short* i, short* j);

	void Birth( agent* a,
				LifeSpan::BirthReason reason,
				agent* a_parent1 = NULL,
				agent* a_parent2 = NULL );
	void Kill( agent* inAgent,
			   LifeSpan::DeathReason reason );
	void Kill_UpdateBrainData( agent *c );
	void Kill_UpdateFittest( agent *c );
	void RemoveFood( food *f );

	float AgentFitness( agent* c );
	
	void ProcessWorldFile( proplib::Document *docWorldFile );

	void Dump();
	

	TSceneView* fSceneView;
	TSceneWindow* fSceneWindow;
	TOverheadView* fOverheadWindow;      //CMB 3/17/06
	TChartWindow* fBirthrateWindow;
	TChartWindow* fFitnessWindow;
	TChartWindow* fFoodEnergyWindow;
	TChartWindow* fPopulationWindow;
	TBrainMonitorWindow* fBrainMonitorWindow;
	TBinChartWindow* fGeneSeparationWindow;
	TAgentPOVWindow* fAgentPOVWindow;
	TTextStatusWindow* fTextStatusWindow;

	Scheduler fScheduler;
	BusyFetchQueue<agent *> fUpdateBrainQueue;
	
	long fMaxSteps;
	bool fEndOnPopulationCrash;
	bool fPaused;
	int fDelay;
	int fDumpFrequency;
	int fStatusFrequency;
	bool fLoadState;
	bool inited;
	
	gstage fStage;	
	TCastList fWorldCast;

	long fNumDomains;
	
	int fSolidObjects;	// agents cannot pass through solid objects (collisions are avoided)
	int fCarryObjects;  // specifies which types of objects can be picked up.
	int fShieldObjects;  // specifies which types of objects act as shields.

	float fCarryPreventsEat;
	float fCarryPreventsFight;
	float fCarryPreventsGive;
	float fCarryPreventsMate;
	
	float fAgentHealingRate;	// Virgil Healing
	bool fHealing;				// Virgil Healing
	
	//long fMonitorAgentRank;
	//long fMonitorAgentRankOld;
//	agent* fMonitorAgent;
	//bool fAgentTracking;
	float fGroundClearance;
//	short fOverHeadRank;
	short fOverHeadRankOld;
	agent* fOverheadAgent;
	bool fChartBorn;
	bool fChartFitness;
	bool fChartFoodEnergy;
	bool fChartPopulation;
//	bool fShowBrain;
	bool fShowTextStatus;
	bool fRecordGeneStats;
	bool fRecordPerformanceStats;
	bool fRecordFoodPatchStats;
	bool fCalcFoodPatchAgentCounts;
	
	std::string fComplexityType;
	float fComplexityFitnessWeight;
	float fHeuristicFitnessWeight;

	long fNewLifes;
	long fNewDeaths;
	
	Color fGroundColor;
	Color fCameraColor;
	float fCameraRadius;
	float fCameraHeight;
	float fCameraRotationRate;
	float fCameraAngleStart;
	float fCameraAngle;
	float fCameraFOV;

	agent* fCurrentFittestAgent[MAXFITNESSITEMS];	// based on heuristic fitness
	float fCurrentMaxFitness[MAXFITNESSITEMS];	// based on heuristic fitness
	int fCurrentFittestCount;
	int fNumberFit;
	FitStruct** fFittest;	// based on the complete fitness, however it is being calculated in AgentFitness(c)
	int fNumberRecentFit;
	FitStruct** fRecentFittest;	// based on the complete fitness, however it is being calculated in AgentFitness(c)
	long fFitness1Frequency;
	long fFitness2Frequency;
	short fFitI;
	short fFitJ;
	long fPositionSeed;
	long fGenomeSeed;
	long fSimulationSeed;

	float fEatFitnessParameter;
	float fEatThreshold;
	
	float fMaxFitness;
	ulong fNumAverageFitness;
	float fAverageFitness;
	float fPrevAvgFitness;
	float fTotalHeuristicFitness;
	float fMateFitnessParameter;
	float fMoveFitnessParameter;
	float fEnergyFitnessParameter;
	float fAgeFitnessParameter;

	long fMinNumAgents; 
	long fInitNumAgents;
	long fNumberToSeed;
	float fProbabilityOfMutatingSeeds;
	bool fSeedFromFile;
	std::vector<std::string> fSeedFilePaths;
	bool fPositionSeedsFromFile;
	std::vector<Position> fSeedPositions;
	float fMinMateFraction;
	long fMateWait;
	long fMiscAgents; // number of agents born without intervening creation before miscegenation function kicks in
	float fMateThreshold;
	float fMaxMateVelocity;
	float fMinEatVelocity;
	float fMaxEatVelocity;
	float fMaxEatYaw;
	EatStatistics fEatStatistics;
	long fEatMateSpan;
	float fEatMateMinDistance;
	float fFightThreshold;
	float fFightFraction;
	enum
	{
		FM_NORMAL,
		FM_NULL
	} fFightMode;
	float fGiveThreshold;
	float fGiveFraction;
	float fPickupThreshold;
	float fDropThreshold;
	
	long fNumberBorn;
	long fNumberBornVirtual;
	long fNumberDied;
	long fNumberDiedAge;
	long fNumberDiedEnergy;
	long fNumberDiedFight;
	long fNumberDiedEdge;
	long fNumberDiedSmite;
	long fNumberDiedPatch;
	long fNumberCreated;
	long fNumberCreatedRandom;
	long fNumberCreated1Fit;
	long fNumberCreated2Fit;
	long fNumberFights;
	long fBirthDenials;
	long fMiscDenials;
	long fLastCreated;
	long fMaxGapCreate;
	long fNumBornSinceCreated;
	
	bool fUseProbabilisticFoodPatches;
	bool fFoodRemovalNeeded;
	int fNumFoodPatches;
	FoodPatch* fFoodPatches;
	FoodPatch** fFoodPatchesNeedingRemoval;

	int fNumBrickPatches;
	BrickPatch* fBrickPatches;


	long fMinFoodCount;
	long fMaxFoodCount;
	long fMaxFoodGrownCount;
	long fInitFoodCount;
	enum
	{
		RFOOD_FALSE,
		RFOOD_TRUE,
		RFOOD_TRUE__FIGHT_ONLY
	} fAgentsRfood;
	float fFoodRate;
	float fFoodRemoveEnergy;
	float fFoodEnergyIn;
	float fFoodEnergyOut;
	float fTotalFoodEnergyIn;
	float fTotalFoodEnergyOut;
	float fAverageFoodEnergyIn;
	float fAverageFoodEnergyOut;
	float fEat2Consume; // (converts eat neuron value to energy consumed)
	int fFoodPatchOuterRange;
	float fMinFoodEnergyAtDeath;
	float fPower2Energy; // controls amount of damage to other agent
	float fDeathProbability;
	
	char   fFogFunction;
	float fExpFogDensity;
	int   fLinearFogEnd;

	bool fMonitorGeneSeparation; 	// whether gene-separation will be monitored or not
	bool fRecordGeneSeparation; 	// whether gene-separation will be recorded or not
	float fMaxGeneSeparation;
	float fMinGeneSeparation;
	float fAverageGeneSeparation;
	FILE* fGeneSeparationFile;
	bool fChartGeneSeparation;
	float* fGeneSepVals;
	long fNumGeneSepVals;
	Stat fLifeSpanStats;
	Stat fNeuronGroupCountStats;
	Stat fCurrentNeuronGroupCountStats;
	Stat fCurrentNeuronCountStats;
	Stat fCurrentSynapseCountStats;
	StatRecent fLifeSpanRecentStats;
	StatRecent fLifeFractionRecentStats;

	bool fRandomBirthLocation;
	float fRandomBirthLocationRadius;

	char  fSmiteMode;		// 'R' = Random; 'L' = Leastfit
	float fSmiteFrac;
	float fSmiteAgeFrac;
	int fNumLeastFit;
	int fMaxNumLeastFit;
	int fNumSmited;
	agent** fLeastFit;	// based on heuristic fitness
	bool fShowVision;
	bool fStaticTimestepGeometry;
	bool fParallelInitAgents;
	bool fParallelInteract;
	bool fParallelBrains;
	bool fGraphics;
	long fBrainMonitorStride;
	
	bool fRecordMovie;
	class PwMovieWriter *fMovieWriter;
	
	unsigned long* fGeneSum;	// sum, for computing mean
	unsigned long* fGeneSum2;	// sum of squares, for computing std. dev.
	agent** fGeneStatsAgents;   // list of agents to be used for computing stats.
	FILE* fGeneStatsFile;

	SeparationCache fSeparationCache;
	
	FILE* fFoodPatchStatsFile;

	unsigned long fNumAgentsNotInOrNearAnyFoodPatch;
	unsigned long* fNumAgentsInFoodPatch;
	
    gcamera fCamera;
    gcamera fOverheadCamera;
    gpolyobj fGround;
    TSetList fWorldSet;	
    gscene fScene;
    gscene fOverheadScene;

	condprop::PropertyList *fConditionalProps;
};

inline gcamera& TSimulation::GetCamera() { return fCamera; }
inline gcamera& TSimulation::GetOverheadCamera() { return fOverheadCamera; } //CMB 3/17/06
inline TChartWindow* TSimulation::GetBirthrateWindow() const { return fBirthrateWindow; }
inline TChartWindow* TSimulation::GetFitnessWindow() const { return fFitnessWindow; }
inline TChartWindow* TSimulation::GetEnergyWindow() const { return fFoodEnergyWindow; }
inline TChartWindow* TSimulation::GetPopulationWindow() const { return fPopulationWindow; }
inline TBrainMonitorWindow* TSimulation::GetBrainMonitorWindow() const { return fBrainMonitorWindow; }
inline TBinChartWindow* TSimulation::GetGeneSeparationWindow() const { return fGeneSeparationWindow; }
inline TAgentPOVWindow* TSimulation::GetAgentPOVWindow() const { return fAgentPOVWindow; }
inline TOverheadView* TSimulation::GetOverheadWindow() const { return fOverheadWindow; }
inline TTextStatusWindow* TSimulation::GetStatusWindow() const { return fTextStatusWindow; }
//inline bool TSimulation::GetShowBrain() const { return fBrainMonitorWindow->visible; }
//inline void TSimulation::SetShowBrain(bool showBrain) { fBrainMonitorWindow->visible = showBrain; }
inline long TSimulation::GetMaxAgents() const { return fMaxNumAgents; }
//inline short TSimulation::OverHeadRank( void ) { return fOverHeadRank; }
inline long TSimulation::GetInitNumAgents() const { return fInitNumAgents; }
inline long TSimulation::GetMaxSteps() const { return fMaxSteps; }
inline float TSimulation::EnergyFitnessParameter() const { return fEnergyFitnessParameter; }
inline float TSimulation::AgeFitnessParameter() const { return fAgeFitnessParameter; }
inline float TSimulation::LifeFractionRecent() { return fLifeFractionRecentStats.mean(); }
inline unsigned long TSimulation::LifeFractionSamples() { return fLifeFractionRecentStats.samples(); }


inline float TSimulation::GetAgentHealingRate() { return fAgentHealingRate;	} 


// Following two functions only determine whether or not we should create the relevant files.
// Linking, renaming, and unlinking are handled according to the specific recording options.
inline bool TSimulation::RecordBrainAnatomy( long agentNumber )
{
	return( fBestSoFarBrainAnatomyRecordFrequency ||
			fBestRecentBrainAnatomyRecordFrequency ||
			fBrainAnatomyRecordAll ||
			(fBrainAnatomyRecordSeeds && (agentNumber <= fInitNumAgents))
		  );
}
inline bool TSimulation::RecordBrainFunction( long agentNumber )
{
	return( fBestSoFarBrainFunctionRecordFrequency ||
			fBestRecentBrainFunctionRecordFrequency ||
			fBrainFunctionRecordAll ||
			(fBrainFunctionRecordSeeds && (agentNumber <= fInitNumAgents))
		  );
}


#endif
