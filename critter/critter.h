#ifndef CRITTER_H
#define CRITTER_H

// System
#include <algobase.h>

// qt
#include <qrect.h>

// Local
#include "brain.h"
#include "debug.h"
#include "gcamera.h"
#include "gdlink.h"
#include "genome.h"
#include "gmisc.h"
#include "gpolygon.h"
#include "gscene.h"
#include "gstage.h"
#include "misc.h"

// Forward declarations
class critter;
class food;
class TCritterPOVWindow;
class TSimulation;

#define kPOVCellPad 2

//===========================================================================
// TSortedCritterList
//===========================================================================

class cxsortedlist : public gdlist<critter*>
{
public:
    cxsortedlist() { }
    ~cxsortedlist() { }
    void add(critter* a);
    void sort();
    void list();
};

//===========================================================================
// critter
//===========================================================================
class critter : public gpolyobj
{
	friend void operator>>(const char **, critter&);

public:
	static void critterinit();
	static critter* getfreecritter(TSimulation* simulation, gstage* stage);
	static void critterload(std::istream& in);
	static void critterdestruct();
	static void critterdump(std::ostream& out);
	
	static float gCritterHeight;	
	static float gMinCritterSize;
	static float gMaxCritterSize;
	static float gEat2Energy;
	static float gMate2Energy;
	static float gFight2Energy;
	static float gMaxSizePenalty;
	static float gSpeed2Energy;
	static float gYaw2Energy;
	static float gLight2Energy;
	static float gFocus2Energy;
	static float gFixedEnergyDrain;
	static bool	gVision;
	static long gInitMateWait;
	static float gSpeed2DPosition;
	static float gMaxRadius;
	static float gMaxVelocity;
	static float gMinMaxEnergy;
	static float gMaxMaxEnergy;
	static float gYaw2DYaw;
	static float gMinFocus;
	static float gMaxFocus;
	static float gCritterFOV;
	static float gMaxSizeAdvantage;
	static cxsortedlist gXSortedCritters;
	    
    critter(TSimulation* simulation, gstage* stage);
    virtual ~critter();
    
    void dump(std::ostream& out);
    void load(std::istream& in);
    float Update(float moveFitnessParam, float speed2dpos);
    
    void SetVelocity(float x, float y, float z);
    void SetVelocity(short k, float f);
    void SetVelocityX(float f);
    void SetVelocityY(float f);
    void SetVelocityZ(float f);
    void SetMass(float f);
    
    virtual void draw();
    void grow();    
    virtual void setradius();    
    float eat(food* f, float eatFitnessParameter, float eat2consume, float eatthreshold);
    void damage(float e);
    float MateProbability(critter* c);
    float mating(float fitness, long wait);
    void rewardmovement(float moveFitnessParam, float speed2dpos);
    void lastrewards(float energyFitness, float ageFitness);
    void Die();
    
    void SetLastX(float x);
    void SetLastY(float y);
    void SetLastZ(float z);
    void SaveLastPosition();
    float X();
    float Y();
    float Z();
    float LastX();
    float LastY();
    float LastZ();
    float Velocity(short i);
    float VelocityX();
    float VelocityY();
    float VelocityZ();
    float Mass();
    float SizeAdvantage();
    float Energy();
    void Energy(float e);
    float FoodEnergy();
    void FoodEnergy(float e);
	float Fight();
    float Strength();
    float Mate();
    float Size();
    long Age();
    long MaxAge();
    float MaxEnergy();
    long LastMate();
    genome* Genes();
    long Number();
    float Fitness();
    virtual void print();
    float FieldOfView();
    short Domain();
    void Domain(short id);
    bool Alive() const;
    long Index() const;
	brain* Brain();
//	gscene& critter::GetScene();
	gscene& GetScene();
	frustumXZ& GetFrustum();
	static gpolyobj* GetCritterObj();
	static short povcols;
	static short povrows;
    static short povwidth;
    static short povheight;
    short xleft;
    short xright;
    short ybottom;
    short ytop;
    short ypix;
	
	static critter* currentCritter;	// during brain updates

protected:
    void Behave();
    void NumberToName();
    void SetGeometry();
    void SetGraphics();
    
	static bool gClassInited;
    static long crittersever;
    static long crittersliving;
    static gpolyobj* critterobj;
    static critter** pc;

	TSimulation* fSimulation;
    bool fAlive;
	long fIndex;
    long fCritterNumber;
    long fAge;
    long fLastMate;
    
    float fEnergy;
    float fFoodEnergy;
    float fMaxEnergy;    
    float fSpeed2Energy;
    float fYaw2Energy;
    float fSizeAdvantage;
    
    float fLengthX;
    float fLengthZ;
    
    float fMass; // mass (not used)
    
    float fLastPosition[3];
    float fVelocity[3];
    float fNoseColor[3];
    
    float fFitness;  // crude guess for keeping minimum population early on
    genome* fGenome;
    brain* fBrain;
    gcamera fCamera;
    gscene fScene;
    frustumXZ fFrustum;
    short fDomain;

	FILE* fBrainFuncFile;
};

inline void critter::SetVelocity(float x, float y, float z) { fVelocity[0] = x; fVelocity[1] = y; fVelocity[2] = z; }
inline void critter::SetVelocity(short k, float f) { fVelocity[k] = f; }
inline void critter::SetVelocityX(float f) { fVelocity[0] = f; }
inline void critter::SetVelocityY(float f) { fVelocity[1] = f; }
inline void critter::SetVelocityZ(float f) { fVelocity[2] = f; }
inline void critter::SetMass(float f) { fMass = f; }
inline void critter::SetLastX(float x) { fLastPosition[0] = x; }
inline void critter::SetLastY(float y) { fLastPosition[1] = y; }
inline void critter::SetLastZ(float z) { fLastPosition[2] = z; }
inline float critter::X() { return fPosition[0]; }
inline float critter::Y() { return fPosition[1]; }
inline float critter::Z() { return fPosition[2]; }
inline float critter::LastX() { return fLastPosition[0]; }
inline float critter::LastY() { return fLastPosition[1]; }
inline float critter::LastZ() { return fLastPosition[2]; }
inline float critter::Velocity(short i) { return fVelocity[i]; }
inline float critter::VelocityX() { return fVelocity[0]; }
inline float critter::VelocityY() { return fVelocity[1]; }
inline float critter::VelocityZ() { return fVelocity[2]; }
inline float critter::Mass() { return fMass; }
inline float critter::SizeAdvantage() { return fSizeAdvantage; }
inline float critter::Energy() { return fEnergy; }
inline void critter::Energy(float e) { fEnergy = e; }
inline float critter::FoodEnergy() { return fFoodEnergy; }
inline void critter::FoodEnergy(float e) { fFoodEnergy = e; }
inline float critter::Fight() { return fBrain->Fight(); }
inline float critter::Strength() { return fGenome->Strength(); }
inline float critter::Mate() { return fBrain->Mate(); }
inline float critter::Size() { return fGenome->Size(gMinCritterSize, gMaxCritterSize); }
inline long critter::Age() { return fAge; }
inline long critter::MaxAge() { return fGenome->Lifespan(); }
inline float critter::MaxEnergy() { return fMaxEnergy; }
inline long critter::LastMate() { return fLastMate; }
inline genome* critter::Genes() { return fGenome; }
inline long critter::Number() { return fCritterNumber; }
inline float critter::Fitness() { return fFitness; }
inline short critter::Domain() { return fDomain; }
inline void critter::Domain(short id) { fDomain = id; }
inline bool critter::Alive() const { return fAlive; }
inline long critter::Index() const { return fIndex; }
inline brain* critter::Brain() { return fBrain; }
inline gscene& critter::GetScene() { return fScene; }
inline frustumXZ& critter::GetFrustum() { return fFrustum; }
inline gpolyobj* critter::GetCritterObj() { return critterobj; }

#endif

