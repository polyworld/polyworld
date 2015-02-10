#ifndef AGENT_H
#define AGENT_H

// System
#include <algorithm>

// qt
//#include <qrect.h>

// Local
#include "AgentAttachedData.h"
#include "AgentListener.h"
#include "Brain.h"
#include "debug.h"
#include "Energy.h"
#include "gcamera.h"
#include "gdlink.h"
#include "Genome.h"
#include "globals.h"
#include "gmisc.h"
#include "gpolygon.h"
#include "gscene.h"
#include "gstage.h"
#include "LifeSpan.h"
#include "misc.h"
#include "Nerve.h"
#include "NervousSystem.h"
#include "objectlist.h"
#include "objectxsortedlist.h"
#include "proplib.h"


// Forward declarations
class agent;
class BeingCarriedSensor;
class CarryingSensor;
class DataLibWriter;
class EnergySensor;
class food;
class MateWaitSensor;
class Metabolism;
class NervousSystem;
class RandomSensor;
class Retina;
class SpeedSensor;
class TSimulation;


//===========================================================================
// agent
//===========================================================================
class agent : public gpolyobj
{
	friend void operator>>(const char **, agent&);

public:	
	enum BodyRedChannel { BRC_FIGHT, BRC_CONST, BRC_GIVE };
	enum BodyGreenChannel { BGC_ID, BGC_LIGHT, BGC_CONST };
	enum BodyBlueChannel { BBC_MATE, BBC_CONST, BBC_ENERGY };
	enum NoseColor { NC_LIGHT, NC_CONST };
	enum YawEncoding { YE_SQUASH, YE_OPPOSE };

	static struct Configuration
	{
		float	agentHeight;	
		float	minAgentSize;
		float	maxAgentSize;
		long	minLifeSpan;
		long	maxLifeSpan;
		float	minStrength;
		float	maxStrength;
		float	minmaxspeed;
		float	maxmaxspeed;
		float	minmateenergy;
		float	maxmateenergy;
		float	eat2Energy;
		float	mate2Energy;
		float	fight2Energy;
		float	give2Energy;
		float	minSizePenalty;
		float	maxSizePenalty;
		float	speed2Energy;
		float	yaw2Energy;
		float	light2Energy;
		float	focus2Energy;
		float	pickup2Energy;
		float	drop2Energy;
		float	carryAgent2Energy;
		float	carryAgentSize2Energy;
		float	fixedEnergyDrain;
		float	maxCarries;
		bool	vision;
		long 	initMateWait;
		float	speed2DPosition;
		float	maxRadius;
		float	maxVelocity;
		float	minMaxEnergy;
		float	maxMaxEnergy;
		float	yaw2DYaw;
		YawEncoding yawEncoding;
		float	minFocus;
		float	maxFocus;
		float	agentFOV;
		float   minVisionPitch;
		float   maxVisionPitch;
		float   minVisionYaw;
		float   maxVisionYaw;
		float   eyeHeight;
		float	maxSizeAdvantage;
		BodyRedChannel bodyRedChannel;
		float   bodyRedChannelConstValue;
		BodyGreenChannel bodyGreenChannel;
		float   bodyGreenChannelConstValue;
		BodyBlueChannel bodyBlueChannel;
		float   bodyBlueChannelConstValue;
		NoseColor noseColor;
		float   noseColorConstValue;
		double	energyUseMultiplier;

		bool	enableMateWaitFeedback;
		bool	enableSpeedFeedback;
		bool	enableGive;
		bool	enableCarry;
		bool	enableVisionPitch;
		bool	enableVisionYaw;
		
	} config;

	static void processWorldfile( proplib::Document &doc );
	static void agentinit();
	static agent* getfreeagent(TSimulation* simulation, gstage* stage);
	static void agentload(std::istream& in);
	static void agentdestruct();
	static void agentdump(std::ostream& out);

    agent(TSimulation* simulation, gstage* stage);
    ~agent();
    
    void dump(std::ostream& out);
    void load(std::istream& in);
	void UpdateVision();
	void UpdateBrain();
    float UpdateBody( float moveFitnessParam,
					  float speed2dpos,
					  int solidObjects,
					  agent* carrier );
	void AvoidCollisions( int solidObjects );
	void AvoidCollisionDirectional( int direction, int solidObjects );
	void GetCollisionFixedCoordinates( float xo, float zo, float xn, float zn, float xb, float zb, float rc, float rb, float *xf, float *zf );
    
    void SetVelocity(float x, float y, float z);
    void SetVelocity(short k, float f);
    void SetVelocityX(float f);
    void SetVelocityY(float f);
    void SetVelocityZ(float f);
    void SetMass(float f);
    
    virtual void draw();
	void setGenomeReady();
    void grow( long mateWait );    
    virtual void setradius();    
	void eat( food* f,
			  float eatFitnessParameter,
			  float eat2consume,
			  float eatthreshold,
			  long step,
			  Energy &return_lost,
			  Energy &return_actuallyEat );
	Energy receive( agent *giver, const Energy &e );
    Energy damage(const Energy &e, bool nullMode);
    float MateProbability(agent* c);
    Energy mating( float mateFitnessParam, long mateWait, bool lockstep );
    void rewardmovement(float moveFitnessParam, float speed2dpos);
    void lastrewards(float energyFitness, float ageFitness);
    void Die();

	void addListener( AgentListener *listener );
	void removeListener( AgentListener *listener );
    
    void SetLastX(float x);
    void SetLastY(float y);
    void SetLastZ(float z);
    void SaveLastPosition();
    float LastX();
    float LastY();
    float LastZ();
    float Velocity(short i);
    float VelocityX();
    float VelocityY();
    float VelocityZ();
	float Speed();
	float MaxSpeed();
	float NormalizedSpeed();
	float NormalizedYaw();
    float Mass();
    float SizeAdvantage();
	const Metabolism *GetMetabolism();
	const Energy &GetEnergy();
	const Energy &GetFoodEnergy();
	void SetEnergy( const Energy &e );
	void SetFoodEnergy( const Energy &e );
	const Energy &GetMaxEnergy();
	float NormalizedEnergy();
	float Eat();
	float Fight();
	float Give();
    float Strength();
    float Mate();
	float Pickup();
	float Drop();
    float Size();
    long Age();
    long MaxAge();
    long LastMate();
	long LastEat();
	float LastEatDistance();
	genome::Genome* Genes();
	NervousSystem* GetNervousSystem();
    long Number();
	float CurrentHeuristicFitness();
	float ProjectedHeuristicFitness();
	float HeuristicFitness();
    float Complexity();
    virtual void print();
    float FieldOfView();
    short Domain();
    void Domain(short id);
    bool Alive() const;
    long Index() const;
	LifeSpan* GetLifeSpan();
	bool GetDeathByPatch();
	void SetDeathByPatch();
	Brain* GetBrain();
	Retina* GetRetina();
	gscene& GetScene();
	gcamera &getCamera();
	frustumXZ& GetFrustum();
	static gpolyobj* GetAgentObj();

	void SetComplexity( float value );
	
	void Heal( float HealingRate, float minFoodEnergy );	//Virgil
	
	void PickupObject( gobject* o );
	void DropMostRecent( void );
	void DropObject( gobject* o );
	float CarryRadius( void );
	void RecalculateCarryRadius( void );
	bool Carrying( gobject* );
	float CarryEnergy( void );
	void PrintCarries( FILE* );
	TSimulation* fSimulation;

	struct BrainAnalysisParms
	{
		std::string functionPath;
	} brainAnalysisParms;
	
protected:
    void NumberToName();
    void SetGeometry();
    void SetGraphics();
	void InitGeneCache();
    
	static bool gClassInited;
    static unsigned long agentsEver;
    static long agentsliving;
    static gpolyobj* agentobj;
    static agent** pc;

    bool fAlive;
    long fAge;
    long fLastMate;
	long fLastEat;
	float fLastEatPosition[3];
	LifeSpan fLifeSpan;
	bool fDeathByPatch;

	Energy fEnergy;
	Energy fFoodEnergy;
	Energy fMaxEnergy;
	const Metabolism *fMetabolism;
    
    float fSpeed2Energy;
    float fYaw2Energy;
    float fSizeAdvantage;
    
    float fLengthX;
    float fLengthZ;
    
    float fMass; // mass (not used)
    
    float fLastPosition[3];
    float fVelocity[3];
    float fNoseColor[3];

	float fSpeed;
	float fMaxSpeed;
	
    float fHeuristicFitness;	// rough estimate along evolutionary biology lines
	float fComplexity;
	
	genome::Genome* fGenome;
	struct GeneCache
	{
		float maxSpeed;
		float strength;
		float size;
		long lifespan;
	} geneCache;

	NervousSystem *fCns;
	Retina *fRetina;
	RandomSensor *fRandomSensor;
	EnergySensor *fEnergySensor;
	MateWaitSensor *fMateWaitSensor;
	SpeedSensor *fSpeedSensor;
	CarryingSensor *fCarryingSensor;
	BeingCarriedSensor *fBeingCarriedSensor;
	struct OutputNerves
	{
		Nerve *eat;
		Nerve *mate;
		Nerve *fight;
		Nerve *speed;
		Nerve *yaw;
		Nerve *yawOppose;
		Nerve *light;
		Nerve *focus;
		Nerve *visionPitch;
		Nerve *visionYaw;
		Nerve *give;
		Nerve *pickup;
		Nerve *drop;
	} outputNerves;

    gcamera fCamera;
    gscene fScene;
    frustumXZ fFrustum;
    short fDomain;
	
	float fCarryRadius;

	friend class AgentAttachedData;
	AgentAttachedData::SlotData *attachedData;

	AgentListeners listeners;
};

inline void agent::addListener( AgentListener *listener ) { listeners.push_back(listener); }
inline void agent::removeListener( AgentListener *listener ) { if( fAlive ) listeners.remove(listener); }
inline void agent::SetVelocity(float x, float y, float z) { fVelocity[0] = x; fVelocity[1] = y; fVelocity[2] = z; }
inline void agent::SetVelocity(short k, float f) { fVelocity[k] = f; }
inline void agent::SetVelocityX(float f) { fVelocity[0] = f; }
inline void agent::SetVelocityY(float f) { fVelocity[1] = f; }
inline void agent::SetVelocityZ(float f) { fVelocity[2] = f; }
inline void agent::SetMass(float f) { fMass = f; }
inline void agent::SetLastX(float x) { fLastPosition[0] = x; }
inline void agent::SetLastY(float y) { fLastPosition[1] = y; }
inline void agent::SetLastZ(float z) { fLastPosition[2] = z; }
inline float agent::LastX() { return fLastPosition[0]; }
inline float agent::LastY() { return fLastPosition[1]; }
inline float agent::LastZ() { return fLastPosition[2]; }
inline float agent::Velocity(short i) { return fVelocity[i]; }
inline float agent::VelocityX() { return fVelocity[0]; }
inline float agent::VelocityY() { return fVelocity[1]; }
inline float agent::VelocityZ() { return fVelocity[2]; }
inline float agent::Speed() { return fSpeed; }
inline float agent::MaxSpeed() { return fMaxSpeed; }
inline float agent::NormalizedSpeed() { return min( 1.0f, Speed() / agent::config.maxVelocity ); }
inline float agent::Mass() { return fMass; }
inline float agent::SizeAdvantage() { return fSizeAdvantage; }
inline const Metabolism *agent::GetMetabolism() { return fMetabolism; }
inline const Energy &agent::GetEnergy() { return fEnergy; }
inline const Energy &agent::GetFoodEnergy() { return fFoodEnergy; }
inline void agent::SetEnergy( const Energy &e ) { fEnergy = e; }
inline void agent::SetFoodEnergy( const Energy &e ) { fFoodEnergy = e; }
inline const Energy &agent::GetMaxEnergy() { return fMaxEnergy; }
inline float agent::NormalizedEnergy() { return fEnergy.sum() / fMaxEnergy.sum(); }
inline float agent::Eat() { return outputNerves.eat->get(); }
inline float agent::Fight() { return outputNerves.fight->get(); }
inline float agent::Give() { return outputNerves.give->get(); }
inline float agent::Strength() { return geneCache.strength; }
inline float agent::Mate() { return outputNerves.mate->get(); }
inline float agent::Pickup() { return outputNerves.pickup->get(); }
inline float agent::Drop() { return outputNerves.drop->get(); }
inline float agent::Size() { return geneCache.size; }
inline long agent::Age() { return fAge; }
inline long agent::MaxAge() { return geneCache.lifespan; }
inline long agent::LastMate() { return fLastMate; }
inline long agent::LastEat() { return fLastEat; }
inline float agent::LastEatDistance() { return dist( fPosition[0], fPosition[2], fLastEatPosition[0], fLastEatPosition[2] ); }
inline genome::Genome* agent::Genes() { return fGenome; }
inline NervousSystem* agent::GetNervousSystem() { return fCns; }
inline long agent::Number() { return getTypeNumber(); }
// replace both occurences of 0.8 with actual estimate of fraction of lifespan agent will live
inline float agent::CurrentHeuristicFitness() { return fHeuristicFitness; }
#define UseProjectedHeuristicFitness 1
#if UseProjectedHeuristicFitness
	inline float agent::HeuristicFitness() { return fAlive ? ProjectedHeuristicFitness() : CurrentHeuristicFitness(); }
#else
	inline float agent::HeuristicFitness() { return CurrentHeuristicFitness(); }
#endif
inline short agent::Domain() { return fDomain; }
inline void agent::Domain(short id) { fDomain = id; }
inline bool agent::Alive() const { return fAlive; }
inline LifeSpan* agent::GetLifeSpan() { return &fLifeSpan; }
inline bool agent::GetDeathByPatch() { return fDeathByPatch; }
inline void agent::SetDeathByPatch() { fDeathByPatch = true; }
inline Brain* agent::GetBrain() { return fCns->getBrain(); }
inline Retina* agent::GetRetina() { return fRetina; }
inline gscene& agent::GetScene() { return fScene; }
inline gcamera &agent::getCamera() { return fCamera; }
inline frustumXZ& agent::GetFrustum() { return fFrustum; }
inline gpolyobj* agent::GetAgentObj() { return agentobj; }
//inline gdlink<agent*>* agent::GetListLink() { return listLink; }

inline void agent::SetComplexity( float value ) { fComplexity = value; }
inline float agent::Complexity() { return fComplexity; }

inline float agent::CarryRadius() { return fCarryRadius; }

#endif

