#ifndef SIMULATION_H
#define SIMULATION_H

#ifdef linux
#include <errno.h>
#endif

// qt
#include <qobject.h>
#include <qapplication.h>
#include <qmainwindow.h>
#include <qmenu.h>

// Local
#include "barrier.h"
#include "critter.h"
#include "food.h"
#include "gmisc.h"
#include "graphics.h"
#include "gstage.h"
#include "TextStatusWindow.h"
#include "OverheadView.h"

// Forward declarations
class TBinChartWindow;
class TBrainMonitorWindow;
class TChartWindow;
class TSceneView;
class TOverheadView;
class TCritterPOVWindow;
//class QMenuBar;
//class QTimer;
class TSimulation;

static const int MAXDOMAINS = 10;
static const int MAXFITNESSITEMS = 5;

// Define file mode mask so that users can read+write, group and others can read (but not write)
#define	PwFileMode ( S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH )
// Define directory mode mask the same, except you need execute privileges to use as a directory (go fig)
#define	PwDirMode ( S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH )

#define CurrentWorldfileVersion 22


struct FitStruct
{
	ulong	critterID;
	float	fitness;
	float   Complexity;
	genome*	genes;
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
	FoodPatch* fFoodPatches;
	BrickPatch* fBrickPatches;
    long minnumcritters;
    long maxnumcritters;
    long initnumcritters;
	long numberToSeed;
    long numcritters;
    long numcreated;
    long numborn;
    long numbornsincecreated;
    long numdied;
    long lastcreate;
    long maxgapcreate;
	float probabilityOfMutatingSeeds;
    short ifit;
    short jfit;
    FitStruct** fittest;
	int fNumLeastFit;
	int fMaxNumLeastFit;
	int fNumSmited;
	critter** fLeastFit;
	
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
	TSceneWindow();
	virtual ~TSceneWindow();
	
	void Update();

protected:
    void closeEvent(QCloseEvent* event);    
    virtual void keyReleaseEvent( QKeyEvent* event ); //Moved here from SceneView (CMB 3/26/06)
    
	void AddFileMenu();
	void AddEditMenu();
	void AddWindowMenu();
	void AddHelpMenu();
	
	void SaveWindowState();
	void SaveDimensions();
	void SaveVisibility();
	void RestoreFromPrefs();
	
private slots:

	// File menu
    void choose() {}
    void save() {}
    void saveAs() {}
    void about() {}    
    void windowsMenuAboutToShow();    
    void timeStep();
    
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
// TSimulation
//===========================================================================
class TSimulation : public QObject
{
	Q_OBJECT

public:
	TSimulation( TSceneView* sceneView, TSceneWindow* sceneWindow );
	virtual ~TSimulation();
	
	void Start();
	void Stop();
	void Pause();
	
	short WhichDomain( float x, float z, short d );
	void SwitchDomain( short newDomain, short oldDomain );
	
	gcamera& GetCamera();
	gcamera& GetOverheadCamera();

	TChartWindow* GetBirthrateWindow() const;
	TChartWindow* GetFitnessWindow() const;
	TChartWindow* GetEnergyWindow() const;
	TChartWindow* GetPopulationWindow() const;	
	TBrainMonitorWindow* GetBrainMonitorWindow() const;	
	TBinChartWindow* GetGeneSeparationWindow() const;
	TCritterPOVWindow* GetCritterPOVWindow() const;
	TTextStatusWindow* GetStatusWindow() const;
	TOverheadView*   GetOverheadWindow() const;
	
//	bool GetShowBrain() const;
//	void SetShowBrain(bool showBrain);
	long GetMaxCritters() const;
	static long fMaxCritters;
	long GetInitNumCritters() const;
	
//	short OverHeadRank( void );
	
	void PopulateStatusList(TStatusList& list);
	
	void Step();
	void Update();
	
	static long fStep;
	bool fCritterTracking;		//Moving to public for access to sceneview and keypress (CMB 3/19/06)
	long fMonitorCritterRank;	//Moving to public for access to sceneview and keypress (CMB 3/19/06)
	long fMonitorCritterRankOld;
	bool fRotateWorld;		//Turn world rotation on or off (CMB 3/19/06)
	static short fOverHeadRank;    	
	static critter* fMonitorCritter;	
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
	bool fBrainAnatomyRecordAll;
	bool fBrainFunctionRecordAll;
	bool fBrainAnatomyRecordSeeds;
	bool fBrainFunctionRecordSeeds;
	
	bool fRecordComplexity;

	bool fRecordAdamiComplexity;
	int fAdamiComplexityRecordFrequency;
	
	float EnergyFitnessParameter() const;
	float AgeFitnessParameter() const;
	float LifeFractionRecent();
	unsigned long LifeFractionSamples();  // the number of samples upon which LifeFractionRecent() is based

	char TSimulation::glFogFunction();
	float TSimulation::glExpFogDensity();
	int TSimulation::glLinearFogEnd();


	float TSimulation::GetCritterHealingRate();				// Virgil Healing

	int getRandomPatch(int domainNumber);
	Domain fDomains[MAXDOMAINS];	

private slots:
	
private:
	void Init();
	void InitNeuralValues();
	void InitWorld();
	void InitMonitoringWindows();
	
	void Interact();
	
	void RecordGeneSeparation();
	void CalculateGeneSeparation(critter* ci);
	void CalculateGeneSeparationAll();

	// Following two functions only determine whether or not we should create the relevant files.
	// Linking, renaming, and unlinking are handled according to the specific recording options.
	bool RecordBrainAnatomy( long critterNumber );
	bool RecordBrainFunction( long critterNumber );
	
	void SmiteOne(short id, short smite);
	void ijfitinc(short* i, short* j);
		
	void Death(critter* inCritter);
	
	void ReadWorldFile(const char* filename);	
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
	TCritterPOVWindow* fCritterPOVWindow;
	TTextStatusWindow* fTextStatusWindow;
	
	long fMaxSteps;
	bool fPaused;
	int fDelay;
	int fDumpFrequency;
	int fStatusFrequency;
	bool fLoadState;
	bool inited;
	
	gstage fStage;	
	TCastList fWorldCast;

	long fNumDomains;
	
	float fCritterHealingRate;	// Virgil Healing
	bool fHealing;				// Virgil Healing
	
	//long fMonitorCritterRank;
	//long fMonitorCritterRankOld;
//	critter* fMonitorCritter;
	//bool fCritterTracking;
	float fGroundClearance;
//	short fOverHeadRank;
	short fOverHeadRankOld;
	critter* fOverheadCritter;
	bool fChartBorn;
	bool fChartFitness;
	bool fChartFoodEnergy;
	bool fChartPopulation;
//	bool fShowBrain;
	bool fShowTextStatus;
	bool fRecordGeneStats;
	bool fRecordFoodPatchStats;
	
	bool fUseComplexityAsFitnessFunc;	//Virgil	

	long fNewDeaths;
	
	Color fGroundColor;
	Color fCameraColor;
	float fCameraRadius;
	float fCameraHeight;
	float fCameraRotationRate;
	float fCameraAngleStart;
	float fCameraAngle;
	float fCameraFOV;

	critter* fCurrentFittestCritter[MAXFITNESSITEMS];
	float fCurrentMaxFitness[MAXFITNESSITEMS];
	int fCurrentFittestCount;
	int fNumberFit;
	FitStruct** fFittest;
	int fNumberRecentFit;
	FitStruct** fRecentFittest;
	long fFitness1Frequency;
	long fFitness2Frequency;
	short fFitI;
	short fFitJ;
	long fPositionSeed;
	long fGenomeSeed;

	float fEatFitnessParameter;
	float fEatThreshold;
	
	float fMaxFitness;
	ulong fNumAverageFitness;
	float fAverageFitness;
	float fTotalFitness;
	float fMateFitnessParameter;
	float fMoveFitnessParameter;
	float fEnergyFitnessParameter;
	float fAgeFitnessParameter;

	long fMinNumCritters; 
	long fInitNumCritters;
	long fNumberToSeed;
	float fProbabilityOfMutatingSeeds;
	float fMinMateFraction;
	long fMateWait;
	long fMiscCritters; // number of critters born without intervening creation before miscegenation function kicks in
	float fMateThreshold;
	float fFightThreshold;
	
	long fNumberBorn;
	long fNumberDied;
	long fNumberDiedAge;
	long fNumberDiedEnergy;
	long fNumberDiedFight;
	long fNumberDiedEdge;
	long fNumberDiedSmite;
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
	int fNumFoodPatches;
	FoodPatch* fFoodPatches;

	int fNumBrickPatches;
	BrickPatch* fBrickPatches;


	long fMinFoodCount;
	long fMaxFoodCount;
	long fMaxFoodGrown;
	long fInitialFoodCount;
	bool fCrittersRfood;
	float fFoodRate;
	float fFoodEnergyIn;
	float fFoodEnergyOut;
	float fTotalFoodEnergyIn;
	float fTotalFoodEnergyOut;
	float fAverageFoodEnergyIn;
	float fAverageFoodEnergyOut;
	float fEat2Consume; // (converts eat neuron value to energy consumed)
	int fFoodPatchOuterRange;
	float fMinFoodEnergyAtDeath;
	float fPower2Energy; // controls amount of damage to other critter
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
	StatRecent fLifeSpanRecentStats;
	StatRecent fLifeFractionRecentStats;

	char  fSmiteMode;		// 'R' = Random; 'L' = Leastfit
	float fSmiteFrac;
	float fSmiteAgeFrac;
	int fNumLeastFit;
	int fMaxNumLeastFit;
	int fNumSmited;
	critter** fLeastFit;
	bool fShowVision;
	bool fGraphics;
	long fBrainMonitorStride;
	
	bool fRecordMovie;
	FILE* fMovieFile;
	
	unsigned long* fGeneSum;	// sum, for computing mean
	unsigned long* fGeneSum2;	// sum of squares, for computing std. dev.
	FILE* fGeneStatsFile;
	
	FILE* fFoodPatchStatsFile;

	unsigned long fNumCrittersNotInOrNearAnyFoodPatch;
	unsigned long* fNumCrittersInFoodPatch;
	
    gcamera fCamera;
    gcamera fOverheadCamera;
    gpolyobj fGround;
    TSetList fWorldSet;	
    gscene fScene;
    gscene fOverheadScene;
};

inline gcamera& TSimulation::GetCamera() { return fCamera; }
inline gcamera& TSimulation::GetOverheadCamera() { return fOverheadCamera; } //CMB 3/17/06
inline TChartWindow* TSimulation::GetBirthrateWindow() const { return fBirthrateWindow; }
inline TChartWindow* TSimulation::GetFitnessWindow() const { return fFitnessWindow; }
inline TChartWindow* TSimulation::GetEnergyWindow() const { return fFoodEnergyWindow; }
inline TChartWindow* TSimulation::GetPopulationWindow() const { return fPopulationWindow; }
inline TBrainMonitorWindow* TSimulation::GetBrainMonitorWindow() const { return fBrainMonitorWindow; }
inline TBinChartWindow* TSimulation::GetGeneSeparationWindow() const { return fGeneSeparationWindow; }
inline TCritterPOVWindow* TSimulation::GetCritterPOVWindow() const { return fCritterPOVWindow; }
inline TOverheadView* TSimulation::GetOverheadWindow() const { return fOverheadWindow; }
inline TTextStatusWindow* TSimulation::GetStatusWindow() const { return fTextStatusWindow; }
//inline bool TSimulation::GetShowBrain() const { return fBrainMonitorWindow->visible; }
//inline void TSimulation::SetShowBrain(bool showBrain) { fBrainMonitorWindow->visible = showBrain; }
inline long TSimulation::GetMaxCritters() const { return fMaxCritters; }
//inline short TSimulation::OverHeadRank( void ) { return fOverHeadRank; }
inline long TSimulation::GetInitNumCritters() const { return fInitNumCritters; }
inline float TSimulation::EnergyFitnessParameter() const { return fEnergyFitnessParameter; }
inline float TSimulation::AgeFitnessParameter() const { return fAgeFitnessParameter; }
inline float TSimulation::LifeFractionRecent() { return fLifeFractionRecentStats.mean(); }
inline unsigned long TSimulation::LifeFractionSamples() { return fLifeFractionRecentStats.samples(); }

//GL Fog
inline char TSimulation::glFogFunction()  { return fFogFunction;   }
inline float TSimulation::glExpFogDensity() { return fExpFogDensity; }
inline int TSimulation::glLinearFogEnd()    { return fLinearFogEnd;  }



inline float TSimulation::GetCritterHealingRate() { return fCritterHealingRate;	} 


// Following two functions only determine whether or not we should create the relevant files.
// Linking, renaming, and unlinking are handled according to the specific recording options.
inline bool TSimulation::RecordBrainAnatomy( long critterNumber )
{
	return( fBestSoFarBrainAnatomyRecordFrequency ||
			fBestRecentBrainAnatomyRecordFrequency ||
			fBrainAnatomyRecordAll ||
			(fBrainAnatomyRecordSeeds && (critterNumber <= fInitNumCritters))
		  );
}
inline bool TSimulation::RecordBrainFunction( long critterNumber )
{
	return( fBestSoFarBrainFunctionRecordFrequency ||
			fBestRecentBrainFunctionRecordFrequency ||
			fBrainFunctionRecordAll ||
			(fBrainFunctionRecordSeeds && (critterNumber <= fInitNumCritters))
		  );
}


#endif
