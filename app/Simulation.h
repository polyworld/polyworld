#ifndef SIMULATION_H
#define SIMULATION_H

// qt
#include <qobject.h>
#include <qapplication.h>
#include <qmainwindow.h>
#include <QMenu>

// Local
#include "barrier.h"
#include "critter.h"
#include "food.h"
#include "gmisc.h"
#include "graphics.h"
#include "gstage.h"
#include "PWTypes.h"
#include "TextStatusWindow.h"

// Forward declarations
class TBinChartWindow;
class TBrainMonitorWindow;
class TChartWindow;
class TSceneView;
class TCritterPOVWindow;
//class QMenuBar;
//class QTimer;
class TSimulation;

static const int MAXDOMAINS = 10;
static const int MAXFITNESSITEMS = 5;

struct FitStruct
{
	ulong	critterID;
	float	fitness;
	genome*	genes;
};
typedef struct FitStruct FitStruct;

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
    TSceneWindow(QMenuBar* menuBar);
    virtual ~TSceneWindow();
    
    void Update();

protected:
    void closeEvent(QCloseEvent* event);
    
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
	void ToggleBrainWindow();
	void TogglePOVWindow();
	void ToggleTextStatus();
	void TileAllWindows();
    
private:
	QMenuBar* fMenuBar;
	QMenu* fWindowsMenu;
	
	TSceneView* fSceneView;
	TSimulation* fSimulation;
	
	QAction* toggleEnergyWindowAct;
	QAction* toggleFitnessWindowAct;
	QAction* togglePopulationWindowAct;
	QAction* toggleBirthrateWindowAct;
	QAction* toggleBrainWindowAct;
	QAction* togglePOVWindowAct;
	QAction* toggleTextStatusAct;
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
	
	short WhichDomain(float x, float z, short d);
	void SwitchDomain(short newDomain, short oldDomain);
	
	gcamera& GetCamera();

	TChartWindow* GetBirthrateWindow() const;
	TChartWindow* GetFitnessWindow() const;
	TChartWindow* GetEnergyWindow() const;
	TChartWindow* GetPopulationWindow() const;	
	TBrainMonitorWindow* GetBrainMonitorWindow() const;	
	TBinChartWindow* GetGeneSeparationWindow() const;
	TCritterPOVWindow* GetCritterPOVWindow() const;
	TTextStatusWindow* GetStatusWindow() const;
	
//	bool GetShowBrain() const;
//	void SetShowBrain(bool showBrain);
	long GetMaxCritters() const;
	static long fMaxCritters;
	
//	short OverHeadRank( void );
	
	void PopulateStatusList(TStatusList& list);
	
	void Step();
	void Update();
	
	static long fStep;
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
	bool fBrainAnatomyRecordAll;
	bool fBrainFunctionRecordAll;

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
	void SmiteOne(short id, short smite);
	void ijfitinc(short* i, short* j);
		
	void Death(critter* inCritter);
	
	void ReadWorldFile(const char* filename);	
	void Dump();

	TSceneView* fSceneView;
	TSceneWindow* fSceneWindow;
	TChartWindow* fBirthrateWindow;
	TChartWindow* fFitnessWindow;
	TChartWindow* fFoodEnergyWindow;
	TChartWindow* fPopulationWindow;
	TBrainMonitorWindow* fBrainMonitorWindow;
	TBinChartWindow* fGeneSeparationWindow;
	TCritterPOVWindow* fCritterPOVWindow;
	TTextStatusWindow* fTextStatusWindow;
	
	bool fPaused;
	int fDelay;
	int fDumpFrequency;
	int fStatusFrequency;
	bool fLoadState;
	bool inited;
	
	gstage fStage;	
	TCastList fWorldCast;

	domainstruct fDomains[MAXDOMAINS];	
	long fNumDomains;
	
	long fMonitorCritterRank;
	long fMonitorCritterRankOld;
//	critter* fMonitorCritter;
	bool fCritterTracking;
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
	float fMinFoodEnergyAtDeath;
	float fPower2Energy; // controls amount of damage to other critter
	float fDeathProbability;


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
	
    gcamera fCamera;
    gpolyobj fGround;
    TSetList fWorldSet;	
	gscene fScene;
};

inline gcamera& TSimulation::GetCamera() { return fCamera; }
inline TChartWindow* TSimulation::GetBirthrateWindow() const { return fBirthrateWindow; }
inline TChartWindow* TSimulation::GetFitnessWindow() const { return fFitnessWindow; }
inline TChartWindow* TSimulation::GetEnergyWindow() const { return fFoodEnergyWindow; }
inline TChartWindow* TSimulation::GetPopulationWindow() const { return fPopulationWindow; }
inline TBrainMonitorWindow* TSimulation::GetBrainMonitorWindow() const { return fBrainMonitorWindow; }
inline TBinChartWindow* TSimulation::GetGeneSeparationWindow() const { return fGeneSeparationWindow; }
inline TCritterPOVWindow* TSimulation::GetCritterPOVWindow() const { return fCritterPOVWindow; }
inline TTextStatusWindow* TSimulation::GetStatusWindow() const { return fTextStatusWindow; }
//inline bool TSimulation::GetShowBrain() const { return fBrainMonitorWindow->visible; }
//inline void TSimulation::SetShowBrain(bool showBrain) { fBrainMonitorWindow->visible = showBrain; }
inline long TSimulation::GetMaxCritters() const { return fMaxCritters; }
//inline short TSimulation::OverHeadRank( void ) { return fOverHeadRank; }


#endif


