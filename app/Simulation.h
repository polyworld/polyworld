#ifndef SIMULATION_H
#define SIMULATION_H

// qt
#include <qobject.h>

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

static const int MAXDOMAINS = 10;
static const int MAXFITNESSITEMS = 5;

//extern long TSimulation::fAge;

//===========================================================================
// TSimulation
//===========================================================================
class TSimulation : public QObject
{
	Q_OBJECT

public:
	TSimulation(TSceneView* sceneView);
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
	
	static long fAge;
	static short fOverHeadRank;
	static critter* fMonitorCritter;
	static double fFramesPerSecondOverall;
	static double fSecondsPerFrameOverall;
	static double fFramesPerSecondRecent;
	static double fSecondsPerFrameRecent;
	static double fFramesPerSecondInstantaneous;
	static double fSecondsPerFrameInstantaneous;
	static double fTimeStart;

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
	TChartWindow* fBirthrateWindow;
	TChartWindow* fFitnessWindow;
	TChartWindow* fFoodEnergyWindow;
	TChartWindow* fPopulationWindow;
	TBrainMonitorWindow* fBrainMonitorWindow;
	TBinChartWindow* fGeneSeparationWindow;
	TCritterPOVWindow* fCritterPOVWindow;
	TTextStatusWindow* fTextStatusWindow;
	
	bool fPaused;
	long fDelay;
	long fDumpFrequency;
	long fStatusFrequency;
	bool fLoadState;
	
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
	int fNumberFit;
	genome** fFittest;
	float* fFitness;
	long fFitness1Frequency;
	long fFitness2Frequency;
	short fFitI;
	short fFitJ;
	long fPositionSeed;
	long fGenomeSeed;

	float fEatFitnessParameter;
	float fEatThreshold;
	
	float fMaxFitness;
	float fAverageFitness;
	float fTotalFitness;
	float fMateFitnessParameter;
	float fMoveFitnessParameter;
	float fEnergyFitnessParameter;
	float fAgeFitnessParameter;

	long fMinNumCritters; 
	long fInitNumCritters;
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
	long fGapFromLastCreate;
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

	float fSmiteFrac;
	float fSmiteAgeFrac;
	int fNumLeastFit;
	int fMaxNumLeastFit;
	int fNumSmited;
	critter** fLeastFit;
	long fBrainMonitorStride;
	bool fShowVision;
	bool fGraphics;
	
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


