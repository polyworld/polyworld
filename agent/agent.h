#ifndef AGENT_H
#define AGENT_H

// System
#include <algorithm>

// qt
//#include <qrect.h>

// Local
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
#include "objectlist.h"
#include "objectxsortedlist.h"


// Forward declarations
class AbstractFile;
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
class TAgentPOVWindow;
class TSimulation;

#define kPOVCellPad 2

//===========================================================================
// agent
//===========================================================================
class agent : public gpolyobj
{
	friend void operator>>(const char **, agent&);

public:
	static void agentinit();
	static agent* getfreeagent(TSimulation* simulation, gstage* stage);
	static void agentload(std::istream& in);
	static void agentdestruct();
	static void agentdump(std::ostream& out);
	
	enum BodyRedChannel { BRC_FIGHT, BRC_CONST, BRC_GIVE };
	enum BodyGreenChannel { BGC_ID, BGC_LIGHT, BGC_CONST };
	enum BodyBlueChannel { BBC_MATE, BBC_CONST, BBC_ENERGY };
	enum NoseColor { NC_LIGHT, NC_CONST };
	enum YawEncoding { YE_SQUASH, YE_OPPOSE };

	static float	gAgentHeight;	
	static float	gMinAgentSize;
	static float	gMaxAgentSize;
	static float	gEat2Energy;
	static float	gMate2Energy;
	static float	gFight2Energy;
	static float	gGive2Energy;
	static float	gMinSizePenalty;
	static float	gMaxSizePenalty;
	static float	gSpeed2Energy;
	static float	gYaw2Energy;
	static float	gLight2Energy;
	static float	gFocus2Energy;
	static float	gPickup2Energy;
	static float	gDrop2Energy;
	static float	gCarryAgent2Energy;
	static float	gCarryAgentSize2Energy;
	static float	gFixedEnergyDrain;
	static float	gMaxCarries;
	static bool		gVision;
	static long 	gInitMateWait;
	static float	gSpeed2DPosition;
	static float	gMaxRadius;
	static float	gMaxVelocity;
	static float	gMinMaxEnergy;
	static float	gMaxMaxEnergy;
	static float	gYaw2DYaw;
	static YawEncoding gYawEncoding;
	static float	gMinFocus;
	static float	gMaxFocus;
	static float	gAgentFOV;
	static float    gMinVisionPitch;
	static float    gMaxVisionPitch;
	static float    gMinVisionYaw;
	static float    gMaxVisionYaw;
	static float    gEyeHeight;
	static float	gMaxSizeAdvantage;
	static BodyRedChannel gBodyRedChannel;
	static float    gBodyRedChannelConstValue;
	static BodyGreenChannel gBodyGreenChannel;
	static float    gBodyGreenChannelConstValue;
	static BodyBlueChannel gBodyBlueChannel;
	static float    gBodyBlueChannelConstValue;
	static NoseColor gNoseColor;
	static float    gNoseColorConstValue;
	static int		gNumDepletionSteps;
	static double	gMaxPopulationPenaltyFraction;
	static double	gPopulationPenaltyFraction;
	static double	gLowPopulationAdvantageFactor;

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
    void grow( long mateWait,
			   bool recordGenome,
			   bool recordBrainAnatomy,
			   bool recordBrainFunction,
			   bool recordPosition );    
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
    Energy mating( float mateFitnessParam, long mateWait );
    void rewardmovement(float moveFitnessParam, float speed2dpos);
    void lastrewards(float energyFitness, float ageFitness);
    void Die();
	void EndFunctional();
    
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
//	gscene& agent::GetScene();
	gscene& GetScene();
	frustumXZ& GetFrustum();
	static gpolyobj* GetAgentObj();
//	gdlink<agent*>* GetListLink();

	static short povcols;
	static short povrows;
    static short povwidth;
    static short povheight;

    short xleft;
    short xright;
    short ybottom;
    short ytop;
    short ypix;

    //gdlink<agent*>* listLink;

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
	void RecordPosition( void );
	TSimulation* fSimulation;
	
 public:
	struct ExternalData {
		void *position_recorder;
		void *separation_cache;
	} external;

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
	long fIndex;
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
		const Metabolism *metabolism;
	} geneCache;

	NervousSystem *fCns;
	Retina *fRetina;
	RandomSensor *fRandomSensor;
	EnergySensor *fEnergySensor;
	MateWaitSensor *fMateWaitSensor;
	SpeedSensor *fSpeedSensor;
	CarryingSensor *fCarryingSensor;
	BeingCarriedSensor *fBeingCarriedSensor;
	Brain* fBrain;
	struct Nerves
	{
		// Inputs
		Nerve *random;
		Nerve *energy;
		Nerve *mateWaitFeedback;
		Nerve *speedFeedback;
		Nerve *carrying;
		Nerve *beingCarried;
		Nerve *red;
		Nerve *green;
		Nerve *blue;
		// Outputs
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
	} nerves;

    gcamera fCamera;
    gscene fScene;
    frustumXZ fFrustum;
    short fDomain;
	
	AbstractFile *fBrainFuncFile;
	DataLibWriter *fPositionWriter;
	
	float fCarryRadius;
};

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
inline float agent::NormalizedSpeed() { return min( 1.0f, Speed() / agent::gMaxVelocity ); }
inline float agent::Mass() { return fMass; }
inline float agent::SizeAdvantage() { return fSizeAdvantage; }
inline const Metabolism *agent::GetMetabolism() { return geneCache.metabolism; }
inline const Energy &agent::GetEnergy() { return fEnergy; }
inline const Energy &agent::GetFoodEnergy() { return fFoodEnergy; }
inline void agent::SetEnergy( const Energy &e ) { fEnergy = e; }
inline void agent::SetFoodEnergy( const Energy &e ) { fFoodEnergy = e; }
inline const Energy &agent::GetMaxEnergy() { return fMaxEnergy; }
inline float agent::NormalizedEnergy() { return fEnergy.sum() / fMaxEnergy.sum(); }
inline float agent::Eat() { return nerves.eat->get(); }
inline float agent::Fight() { return nerves.fight->get(); }
inline float agent::Give() { return nerves.give->get(); }
inline float agent::Strength() { return geneCache.strength; }
inline float agent::Mate() { return nerves.mate->get(); }
inline float agent::Pickup() { return nerves.pickup->get(); }
inline float agent::Drop() { return nerves.drop->get(); }
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
inline long agent::Index() const { return fIndex; }
inline LifeSpan* agent::GetLifeSpan() { return &fLifeSpan; }
inline bool agent::GetDeathByPatch() { return fDeathByPatch; }
inline void agent::SetDeathByPatch() { fDeathByPatch = true; }
inline Brain* agent::GetBrain() { return fBrain; }
inline Retina* agent::GetRetina() { return fRetina; }
inline gscene& agent::GetScene() { return fScene; }
inline frustumXZ& agent::GetFrustum() { return fFrustum; }
inline gpolyobj* agent::GetAgentObj() { return agentobj; }
//inline gdlink<agent*>* agent::GetListLink() { return listLink; }

inline void agent::SetComplexity( float value ) { fComplexity = value; }
inline float agent::Complexity() { return fComplexity; }

inline float agent::CarryRadius() { return fCarryRadius; }

#endif

