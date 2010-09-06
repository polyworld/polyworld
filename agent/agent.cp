/********************************************************************/
/* PolyWorld:  An Artificial Life Ecological Simulator              */
/* by Larry Yaeger                                                  */
/* Copyright Apple Computer 1990,1991,1992                          */
/********************************************************************/
// Self
#include "agent.h"

// System
#include <gl.h>
#include <string.h>

// stl
#include <bitset>

// qt
#include <qgl.h>

// Local
#include "AgentPOVWindow.h"
#include "barrier.h"
#include "BeingCarriedSensor.h"
#include "CarryingSensor.h"
#include "datalib.h"
#include "debug.h"
#include "food.h"
#include "EnergySensor.h"
#include "globals.h"
#include "GenomeUtil.h"
#include "graphics.h"
#include "graybin.h"
#include "misc.h"
#include "NervousSystem.h"
#include "RandomSensor.h"
#include "Resources.h"
#include "Retina.h"
#include "Simulation.h"
#include "SpeedSensor.h"

using namespace genome;


#define gGive2Energy gFight2Energy

#pragma mark -

// UniformPopulationEnergyPenalty controls whether or not the population energy penalty
// is the same for all agents (1) or based on each agent's own maximum energy capacity (0).
// It is off by default.
#define UniformPopulationEnergyPenalty 0

#define UseLightForOpposingYawHack 0
#define DirectYaw 0

// Agent globals
bool		agent::gClassInited;
unsigned long	agent::agentsEver;
long		agent::agentsliving;
gpolyobj*	agent::agentobj;
short		agent::povcols;
short		agent::povrows;
short		agent::povwidth;
short		agent::povheight;

float		agent::gAgentHeight;
float		agent::gMinAgentSize;
float		agent::gMaxAgentSize;
float		agent::gEat2Energy;
float		agent::gMate2Energy;
float		agent::gFight2Energy;
float		agent::gMaxSizePenalty;
float		agent::gSpeed2Energy;
float		agent::gYaw2Energy;
float		agent::gLight2Energy;
float		agent::gFocus2Energy;
float		agent::gPickup2Energy;
float		agent::gDrop2Energy;
float		agent::gCarryAgent2Energy;
float		agent::gCarryAgentSize2Energy;
float		agent::gFixedEnergyDrain;
float		agent::gMaxCarries;
bool		agent::gVision;
long		agent::gInitMateWait;
float		agent::gSpeed2DPosition;
float		agent::gMaxRadius;
float		agent::gMaxVelocity;
float		agent::gMinMaxEnergy;
float		agent::gMaxMaxEnergy;
float		agent::gYaw2DYaw;
float		agent::gMinFocus;
float		agent::gMaxFocus;
float		agent::gAgentFOV;
float		agent::gMaxSizeAdvantage;

agent::BodyGreenChannel agent::gBodyGreenChannel;
float					agent::gBodyGreenChannelConstValue;

agent::NoseColor agent::gNoseColor;
float            agent::gNoseColorConstValue;

// agent::gNumDepletionSteps and agent:gMaxPopulationFraction must both be initialized to zero
// for backward compatibility, and we depend on that fact in ReadWorldFile(), so don't change it

int			agent::gNumDepletionSteps = 0;
double		agent::gMaxPopulationPenaltyFraction = 0.0;
double		agent::gPopulationPenaltyFraction = 0.0;
double		agent::gLowPopulationAdvantageFactor = 1.0;


// [TODO] figure out a better way to track agent indices
bitset<1000> gAgentIndex;


//---------------------------------------------------------------------------
// agent::agent
//---------------------------------------------------------------------------
agent::agent(TSimulation* sim, gstage* stage)
	:	xleft(-1),  		// to show it hasn't been initialized
		fSimulation(sim),
		fAlive(false), 		// must grow() to be truly alive
		fMass(0.0), 		// mass - not used
		fHeuristicFitness(0.0),  	// crude guess for keeping minimum population early on
		fGenome(NULL),
		fCns(NULL),
		fRetina(NULL),
		fRandomSensor(NULL),
		fEnergySensor(NULL),
		fSpeedSensor(NULL),
		fBrain(NULL),
		fBrainFuncFile(NULL),
		fPositionWriter(NULL)
{
	Q_CHECK_PTR(sim);
	Q_CHECK_PTR(stage);
	
	/* Set object type to be AGENTTYPE */
	setType(AGENTTYPE);
	
	if (!gClassInited)
		agentinit();

	fLastPosition[0] = 0.0;
	fLastPosition[1] = 0.0;
	fLastPosition[2] = 0.0;

	fVelocity[0] = 0.0;
	fVelocity[1] = 0.0;
	fVelocity[2] = 0.0;
	
	fSpeed = 0.0;
	fMaxSpeed = 0.0;
	fLastEat = 0;

	fGenome = GenomeUtil::createGenome();
	Q_CHECK_PTR(fGenome);

	fCns = new NervousSystem();
	Q_CHECK_PTR(fCns);
	
	fBrain = new Brain(fCns);
	Q_CHECK_PTR(fBrain);
	
	// Set up agent POV	
	fScene.SetStage(stage);	
	fScene.SetCamera(&fCamera);
}


//---------------------------------------------------------------------------
// agent::~agent
//---------------------------------------------------------------------------
agent::~agent()
{
//	delete fPolygon;
	delete fGenome;
	delete fCns;
	delete fBrain;
	delete fRandomSensor;
	delete fEnergySensor;
	delete fSpeedSensor;
	delete fRetina;
}


//-------------------------------------------------------------------------------------------
// agent::agentinit
//
// Static global class initializer
//-------------------------------------------------------------------------------------------
void agent::agentinit()
{
    if (agent::gClassInited)
        return;

    agent::gClassInited = true;
    agent::agentsliving = 0;
    agent::agentobj = new gpolyobj();
	Resources::loadPolygons( agent::agentobj, "agent" );
	agent::agentobj->SetName("agentobj");
	
	// If we decide we want the width W (in cells) to be a multiple of N (call it I)
	// and we want the aspect ratio of W to height H (in cells) to be at least A,
	// and we call maxAgents M, then (do the math or trust me):
	// I = floor( (sqrt(M*A) + (N-1)) / N )
	// W = I * N
	// H = floor( (M + W - 1) / W )

	int n = 10;
	int a = 3;
	int i = (int) (sqrt( (float) (TSimulation::fMaxNumAgents * a) ) + n - 1) / n;
	agent::povcols = i * n;   // width in cells
	agent::povrows = (TSimulation::fMaxNumAgents + agent::povcols - 1) / agent:: povcols;
	agent::povwidth = agent::povcols * (brain::retinawidth + kPOVCellPad);
	agent::povheight = agent::povrows * (brain::retinaheight + kPOVCellPad);
//	cout << "numCols" ses agent::povcols cms "numRows" ses agent::povrows cms "w" ses agent::povwidth cms "h" ses agent::povheight nl;
}


//-------------------------------------------------------------------------------------------
// agent::agentdestruct
//
// Static global class destructor
//-------------------------------------------------------------------------------------------
void agent::agentdestruct()
{
	delete agent::agentobj;
}


//-------------------------------------------------------------------------------------------
// agent::getfreeagent
//
// Return a newly allocated agent, but enforce the max number of allowed agents
//-------------------------------------------------------------------------------------------
agent* agent::getfreeagent(TSimulation* simulation, gstage* stage)
{
	Q_CHECK_PTR(simulation);
	Q_CHECK_PTR(stage);
	
	if (agent::agentsliving + 1 > simulation->GetMaxAgents())
	{
		printf("PolyWorld ERROR:: Unable to satisfy request for free agent. Max allocated: %ld\n", simulation->GetMaxAgents());
		return NULL;
	}

	// Create the new agent
	agent* c = new agent(simulation, stage);	
	Q_CHECK_PTR(c);
	
    // Increase current total of creatures alive
    agent::agentsliving++;
    
    // Set number to total creatures that have ever lived (note this is 1-based)
    c->setTypeNumber( ++agent::agentsEver );

	// Set agent index.  Used for POV drawing.
	for (size_t index = 0; index < gAgentIndex.size(); ++index)
	{
		if (!gAgentIndex.test(index))
		{
			c->fIndex = index;
			gAgentIndex.set(index);
//			cout << "getfreeagent: c = " << c << ", agentNumber = " << c->getTypeNumber() << ", fIndex = " << c->fIndex << endl;
			break;
		}
	}
		
    return c;
}


//---------------------------------------------------------------------------
// agent::agentdump
//---------------------------------------------------------------------------    
void agent::agentdump(ostream& out)
{
    out << agent::agentsEver nl;
    out << agent::agentsliving nl;

}


//---------------------------------------------------------------------------
// agent::agentload
//---------------------------------------------------------------------------    
void agent::agentload(istream&)
{
	qWarning("agent::agentload called. Not supported.");
#if 0
    in >> agent::agentsEver;
    in >> agent::agentsliving;

    agent::agentlist->load(in);
	
	long i;
		
    for (i = 0; i < gMaxNumAgents; i++)
        if (!agent::pc[i])
            break;
    if (i)
        error(2,"agent::pc[] array not empty during agentload");
        
    //    if (agent::gXSortedAgents.count())
    //        error(2,"gXSortedAgents list not empty during agentload");
    if (allxsortedlist::gXSortedAll.getCount(AGENTTYPE))
        error(2,"gXSortedAll list not empty during agentload");
        
    long numagentsallocated = 0;
    in >> numagentsallocated;
    for (i = 0; i < numagentsallocated; i++)
    {
        agent::pc[i] = new agent;
        if (agent::pc[i] == NULL)
            error(2,"Insufficient memory for new agent in agentload");
        if (agent::agentlist->isone(i))
        {
            (agent::pc[i])->load(in);
            //agent::pc[i]->listLink = agent::gXSortedAgents.add(agent::pc[i]);
	    	agent::pc[i]->listLink = allxsortedlist::gXSortedAll.add(agent::pc[i]);
            globals::worldstage.addobject(agent::pc[i]);
            if ((agent::pc[i])->fIndex != i)
            {
                char msg[256];
                sprintf(msg,
                    	"pc[i]->fIndex (%ld) does not match actual index (%ld)",
                    	(agent::pc[i])->fIndex,i);
                error(2,msg);
            }
        }
        else
        {
            (agent::pc[i])->fIndex = i;
        }
    }
#endif
}


#pragma mark -


//---------------------------------------------------------------------------
// agent::dump
//---------------------------------------------------------------------------
void agent::dump(ostream& out)
{
    out << getTypeNumber() nl;
    out << fIndex nl;
    out << fAge nl;
    out << fLastMate nl;
    out << fEnergy nl;
    out << fFoodEnergy nl;
    out << fMaxEnergy nl;
    out << fSpeed2Energy nl;
    out << fYaw2Energy nl;
    out << fSizeAdvantage nl;
    out << fMass nl;
    out << fLastPosition[0] sp fLastPosition[1] sp fLastPosition[2] nl;
    out << fVelocity[0] sp fVelocity[1] sp fVelocity[2] nl;
    out << fNoseColor[0] sp fNoseColor[1] sp fNoseColor[2] nl;
    out << fHeuristicFitness nl;

    gobject::dump(out);

    fGenome->dump(out);
    if (fBrain != NULL)
        fBrain->Dump(out);
    else
        error(1, "Attempted to dump a agent with no brain");
}


//---------------------------------------------------------------------------
// agent::load
//---------------------------------------------------------------------------    
void agent::load(istream& in)
{
	qWarning("fix domain issue");
	
	unsigned long agentNumber;
	
    in >> agentNumber;
	setTypeNumber( agentNumber );
    in >> fIndex;
    in >> fAge;
    in >> fLastMate;
    in >> fEnergy;
    in >> fFoodEnergy;
    in >> fMaxEnergy;
    in >> fSpeed2Energy;
    in >> fYaw2Energy;
    in >> fSizeAdvantage;
    in >> fMass;
    in >> fLastPosition[0] >> fLastPosition[1] >> fLastPosition[2];
    in >> fVelocity[0] >> fVelocity[1] >> fVelocity[2];
    in >> fNoseColor[0] >> fNoseColor[1] >> fNoseColor[2];
    in >> fHeuristicFitness;

    gobject::load(in);

    fGenome->load(in);
    if (fBrain == NULL)
    {
        fBrain = new Brain(fCns);
        Q_CHECK_PTR(fBrain);
    }
    fBrain->Load(in);

    // done loading in raw information, now setup some derived quantities
    SetGeometry();
    SetGraphics();
    fDomain = fSimulation->WhichDomain(fPosition[0], fPosition[2], 0);
}


//---------------------------------------------------------------------------
// agent::grow
//---------------------------------------------------------------------------
void agent::grow( bool recordGenome,
				  bool recordBrainAnatomy,
				  bool recordBrainFunction,
				  bool recordPosition )
{    
	Q_CHECK_PTR(fBrain);
	Q_CHECK_PTR(fGenome);
	Q_CHECK_PTR(fCns);

	if( recordGenome )
	{
		char path[256];
		sprintf( path, "run/genome/genome_%ld.txt", getTypeNumber() );
		ofstream out( path );
		fGenome->dump( out );
	}

	InitGeneCache();

	fCns->setBrain( fBrain );

#define INPUT(NAME) nerves.NAME = fCns->add( Nerve::INPUT, #NAME )
	INPUT(random);
	INPUT(energy);
	if( genome::gEnableSpeedFeedback )
		INPUT(speedFeedback);
	if( genome::gEnableCarry )
	{
		INPUT(carrying);
		INPUT(beingCarried);
	}
	INPUT(red);
	INPUT(green);
	INPUT(blue);
#undef INPUT

#define OUTPUT(NAME) nerves.NAME = fCns->add( Nerve::OUTPUT, #NAME )
	OUTPUT(eat);
	OUTPUT(mate);
	OUTPUT(fight);
	OUTPUT(speed);
	OUTPUT(yaw);
	OUTPUT(light);
	OUTPUT(focus);
	if( genome::gEnableGive )
		OUTPUT(give);
	if( genome::gEnableCarry )
	{
		OUTPUT(pickup);
		OUTPUT(drop);
	}
#undef OUTPUT

	fCns->add( fRetina = new Retina(brain::retinawidth) );
	fCns->add( fEnergySensor = new EnergySensor(this) );
	fCns->add( fRandomSensor = new RandomSensor(fBrain->rng) );
	if( genome::gEnableSpeedFeedback )
		fCns->add( fSpeedSensor = new SpeedSensor(this) );
	if( genome::gEnableCarry )
	{
		fCns->add( fCarryingSensor = new CarryingSensor(this) );
		fCns->add( fBeingCarriedSensor = new BeingCarriedSensor(this) );
	}

	fCns->grow( fGenome, getTypeNumber(), recordBrainAnatomy );

	// If we're recording brain function,
	// open the file to be used to write out neural activity
	if( recordBrainFunction )
		fBrainFuncFile = fBrain->startFunctional( getTypeNumber() );

	if( recordPosition )
	{
		char path[512];
		sprintf( path,
				 "run/motion/position/position_%ld.txt",
				 getTypeNumber() );

		fPositionWriter = new DataLibWriter( path, true, false );

		const char *colnames[] = {"Timestep", "x", "y", "z", NULL};
		const datalib::Type coltypes[] = {datalib::INT, datalib::FLOAT, datalib::FLOAT, datalib::FLOAT};
		fPositionWriter->beginTable( "Positions",
									 colnames,
									 coltypes );		
	}

    // setup the agent's geometry
    SetGeometry();

	// initially set red & blue to 0
    fColor[0] = fColor[2] = 0.0;
    
	// set body green channel
	switch(gBodyGreenChannel)
	{
	case BGC_ID:
		fColor[1] = fGenome->get( "ID" );
		break;
	case BGC_CONST:
		fColor[1] = gBodyGreenChannelConstValue;
		break;
	case BGC_LIGHT:
		// no-op
		break;
	default:
		assert(false);
		break;
	}
    
	// set the initial nose color
	float noseColor;
	switch(gNoseColor)
	{
	case NC_CONST:
		noseColor = gNoseColorConstValue;
		break;
	case NC_LIGHT:
		// start neutral gray
		noseColor = 0.5;
		break;
	default:
		assert(false);
		break;
	}
    fNoseColor[0] = fNoseColor[1] = fNoseColor[2] = noseColor;
    
    fAge = 0;
    fLastMate = gInitMateWait;
    
	float size_rel = geneCache.size - gMinAgentSize;

    fMaxEnergy = gMinMaxEnergy + (size_rel
    			 * (gMaxMaxEnergy - gMinMaxEnergy) / (gMaxAgentSize - gMinAgentSize) );
	
    fEnergy = fMaxEnergy;
	
	fFoodEnergy = fMaxEnergy;
	
//	printf( "%s: energy initialized to %g\n", __func__, fEnergy );
    
    fSpeed2Energy = gSpeed2Energy * geneCache.maxSpeed
				     * size_rel * (gMaxSizePenalty - 1.0)
					/ (gMaxAgentSize - gMinAgentSize);
    
    fYaw2Energy = gYaw2Energy * geneCache.maxSpeed
              	   * size_rel * (gMaxSizePenalty - 1.0)
              	   / (gMaxAgentSize - gMinAgentSize);
    
    fSizeAdvantage = 1.0 + ( size_rel *
                (gMaxSizeAdvantage - 1.0) / (gMaxAgentSize - gMinAgentSize) );

    // now setup the camera & window for our agent to see the world in
    SetGraphics();

    fAlive = true;
}


//---------------------------------------------------------------------------
// agent::setradius
//---------------------------------------------------------------------------
void agent::setradius()
{
	// only set radius anew if not set manually
	if( !fRadiusFixed ) 
		fRadius = sqrt( fLength[0]*fLength[0] + fLength[2]*fLength[2] ) * fRadiusScale * fScale * 0.5;
	srPrint( "agent::%s(): r=%g%s lx=%g lz=%g rs=%g s=%g\n", __FUNCTION__, fRadius, fRadiusFixed ? " (fixed)" : "", fLength[0], fLength[2], fRadiusScale, fScale );
}


//---------------------------------------------------------------------------
// agent::eat
//
//	Return the amount of food energy actually lost to the world (if any)
//---------------------------------------------------------------------------
float agent::eat(food* f, float eatFitnessParameter, float eat2consume, float eatthreshold, long step)
{
	Q_CHECK_PTR(f);
	
	float result = 0;
	
	if (nerves.eat->get() > eatthreshold)
	{
		float trytoeat = nerves.eat->get() * eat2consume;
		
		if ((fEnergy+trytoeat) > fMaxEnergy)
			trytoeat = fMaxEnergy - fEnergy;
		
		float actuallyeat = f->eat(trytoeat);
		fEnergy += actuallyeat;
		fFoodEnergy += actuallyeat;

	#ifdef OF1
		mytotein += actuallyeat;
	#endif

		if (fFoodEnergy > fMaxEnergy)
		{
			result = fFoodEnergy - fMaxEnergy;
			fFoodEnergy = fMaxEnergy;
		}
				
		fHeuristicFitness += eatFitnessParameter * actuallyeat / (eat2consume * MaxAge());
		
		if( actuallyeat > 0.0 )
			fLastEat = step;
	}
	
	return result;
}

//---------------------------------------------------------------------------
// agent::receive
//---------------------------------------------------------------------------    
float agent::receive( agent *giver, float *e )
{
	float actual = min( *e, fMaxEnergy - fEnergy );
	float result = 0;

	if( actual > 0 )
	{
		fEnergy += actual;
#if GIVE_TODO
		fFoodEnergy += actual;
#endif
		giver->damage( actual );

#if GIVE_TODO
		if( fFoodEnergy > fMaxEnergy )
		{
			result = fFoodEnergy - fMaxEnergy;
			fFoodEnergy = fMaxEnergy;
		}
#endif
	}
	else
	{
		actual = 0;
	}

	*e = actual;

	return result;
}

//---------------------------------------------------------------------------
// agent::damage
//---------------------------------------------------------------------------    
void agent::damage(float e)
{
	e *= gLowPopulationAdvantageFactor;
	fEnergy -= (e<fEnergy) ? e : fEnergy;
}


//---------------------------------------------------------------------------
// agent::MateProbability
//---------------------------------------------------------------------------    
float agent::MateProbability(agent* c)
{
	return fGenome->mateProbability(c->fGenome);
}


//---------------------------------------------------------------------------
// agent::mating
//
// Note:  This function must remain valid whether we are doing a virtual
// or a real birth
//---------------------------------------------------------------------------    
float agent::mating( float mateFitnessParam, long mateWait )
{
	fLastMate = fAge;
	
	if( mateWait <= 0 )
		mateWait = 1;
	fHeuristicFitness += mateFitnessParam * mateWait / MaxAge();
	
	float mymateenergy = fGenome->get( "MateEnergyFraction" ) * fEnergy;	
	fEnergy -= mymateenergy;
	fFoodEnergy -= mymateenergy;
	
	return mymateenergy;
}
    

//---------------------------------------------------------------------------
// agent::rewardmovement
//---------------------------------------------------------------------------        
void agent::rewardmovement(float moveFitnessParam, float speed2dpos)
{
	fHeuristicFitness += moveFitnessParam
				* (fabs(fPosition[0] - fLastPosition[0]) + fabs(fPosition[2] - fLastPosition[2]))
		/ (geneCache.maxSpeed * speed2dpos * MaxAge());
}
    

//---------------------------------------------------------------------------
// agent::lastrewards
//---------------------------------------------------------------------------        
void agent::lastrewards(float energyFitness, float ageFitness)
{
    fHeuristicFitness += energyFitness * fEnergy / fMaxEnergy
              + ageFitness * fAge / MaxAge();
}
    
 
//---------------------------------------------------------------------------
// agent::ProjectedHeuristicFitness
//---------------------------------------------------------------------------        
float agent::ProjectedHeuristicFitness()
{
	if( fSimulation->LifeFractionSamples() >= 50 )
		return( fHeuristicFitness * fSimulation->LifeFractionRecent() * MaxAge() / fAge +
				fSimulation->EnergyFitnessParameter() * fEnergy / fMaxEnergy +
				fSimulation->AgeFitnessParameter() * fSimulation->LifeFractionRecent() );
	else
		return( fHeuristicFitness );
}

//---------------------------------------------------------------------------
// agent::Die
//---------------------------------------------------------------------------    
void agent::Die()
{
	if( fPositionWriter )
	{
		delete fPositionWriter;
		fPositionWriter = NULL;
	}

	if( fLifeSpan.death.reason == LifeSpan::DR_SIMEND )
	{
		return;
	}
	
    itfor( gObjectList, fCarries, it )
    {
        gobject* o = *it;
        o->Dropped();
    }
    fCarries.clear();
	
	if( BeingCarried() )
	{
		agent* a = (agent*)fCarriedBy;
		a->DropObject( this );
	}

	// Decrement total number of agents
	agent::agentsliving--;	
	Q_ASSERT(agent::agentsliving >= 0);
	
	// Clear index in bitset
	//cout << "agent::Die: this = " << this << ", agentNumber = " << getTypeNumber() << ", fIndex = " << fIndex << "----------" << endl;
	gAgentIndex.set(fIndex, false);
	
	// Used to clear this agent's pane in the POV window/region, and call endbrainmonitoring()
	
	// No longer alive. :(		
	fAlive = false;

	// If we're recording brain function, end it here
	if( fBrainFuncFile )
		fBrain->endFunctional( fBrainFuncFile, fHeuristicFitness );
	fBrainFuncFile = NULL;
}



//---------------------------------------------------------------------------
// agent::SetGeometry
//---------------------------------------------------------------------------    
void agent::SetGeometry()
{
	// obtain a fresh copy of the basic agent geometry
    clonegeom(*agentobj);
        
    // then adjust the geometry to fit size, speed, & agentheight
    fLengthX = Size() / sqrt(geneCache.maxSpeed);
    fLengthZ = Size() * sqrt(geneCache.maxSpeed);
    srPrint( "agent::%s(): min=%g, max=%g, speed=%g, size=%g, lx=%g, lz=%g\n", __FUNCTION__, gMinAgentSize, gMaxAgentSize, geneCache.maxSpeed, Size(), fLengthX, fLengthZ );
    for (long i = 0; i < fNumPolygons; i++)
    {
        for (long j = 0; j < fPolygon[i].fNumPoints; j++)
        {
            fPolygon[i].fVertices[j * 3  ] *= fLengthX;
            fPolygon[i].fVertices[j * 3 + 1] *= gAgentHeight;
            fPolygon[i].fVertices[j * 3 + 2] *= fLengthZ;
        }
    }
    setlen();
	fCarryRadius = fRadius;
}


//---------------------------------------------------------------------------
// agent::SetGraphics
//---------------------------------------------------------------------------    
void agent::SetGraphics()
{
    // setup the camera & window for our agent to see the world in
	float fovx = FieldOfView();

	fCamera.SetAspect(fovx * brain::retinaheight / (gAgentFOV * brain::retinawidth));
    fCamera.settranslation(0.0, 0.0, -0.5 * fLengthZ);
    
    if (xleft < 0)  // not initialized yet
    {
        short irow = short(fIndex / povcols);            
        short icol = short(fIndex) - (povcols * irow);

        xleft = icol * (brain::retinawidth + kPOVCellPad)  +  kPOVCellPad;
        xright = xleft + brain::retinawidth - 1;
		ytop = agent::povheight  -  (irow) * (brain::retinaheight + kPOVCellPad)  -  kPOVCellPad  -  1;
		ybottom = ytop  -  brain::retinaheight  +  1;
		ypix = ybottom  +  brain::retinaheight / 2;		// +  1;

//		cout << "****povheight" ses povheight cms "retinaheight" ses brain::retinaheight cms "povrows" ses povrows cms "irow" ses irow nl;
//		cout << "    povwidth " ses povwidth  cms "retinawidth " ses brain::retinawidth  cms "povcols" ses povcols cms "icol" ses icol nl;
//		cout << "    index" ses fIndex cms "xleft" ses xleft cms "xright" ses xright cms "ytop" ses ytop cms "ybottom" ses ybottom cms "ypix" ses ypix nl;

        fCamera.SetNear(.01);
        fCamera.SetFar(1.5 * globals::worldsize);
        fCamera.SetFOV(gAgentFOV);

		if( fSimulation->glFogFunction() != 'O' )
			fCamera.SetFog(true, fSimulation->glFogFunction(), fSimulation->glExpFogDensity(), fSimulation->glLinearFogEnd() );
		
        fCamera.AttachTo(this);
    }
}


//---------------------------------------------------------------------------
// agent::InitGeneCache
//---------------------------------------------------------------------------    
void agent::InitGeneCache()
{
	geneCache.maxSpeed = fGenome->get("MaxSpeed");
	geneCache.strength = fGenome->get("Strength");
	geneCache.size = fGenome->get("Size");
	geneCache.lifespan = fGenome->get("LifeSpan");
}

//---------------------------------------------------------------------------
// agent::SaveLastPosition
//---------------------------------------------------------------------------    
void agent::SaveLastPosition()
{
	fLastPosition[0] = fPosition[0];
	fLastPosition[1] = fPosition[1];
	fLastPosition[2] = fPosition[2];
}


//---------------------------------------------------------------------------
// agent::UpdateVision
//---------------------------------------------------------------------------
void agent::UpdateVision()
{
    if (gVision)
    {
		// create retinal pixmap, based on values of focus & numvisneurons
        const float fovx = nerves.focus->get() * (gMaxFocus - gMinFocus) + gMinFocus;
        		
		fFrustum.Set(fPosition[0], fPosition[2], fAngle[0], fovx, gMaxRadius);
		fCamera.SetAspect(fovx * brain::retinaheight / (gAgentFOV * brain::retinawidth));
		
		fSimulation->GetAgentPOVWindow()->DrawAgentPOV( this );

		debugcheck( "after DrawAgentPOV" );

		fRetina->updateBuffer( xleft, ypix );
	}
}


//---------------------------------------------------------------------------
// agent::UpdateBrain
//---------------------------------------------------------------------------
void agent::UpdateBrain()
{
	fCns->update( this == TSimulation::fMonitorAgent && TSimulation::fOverHeadRank );

	// If we're recording brain function, do it here
	if( fBrainFuncFile )
		fBrain->writeFunctional( fBrainFuncFile );
}


//---------------------------------------------------------------------------
// agent::UpdateBody
//
// Return energy consumed
//---------------------------------------------------------------------------
const float FF = 1.01;

float agent::UpdateBody( float moveFitnessParam,
						 float speed2dpos,
						 int solidObjects,
						 agent* carrier )
{
    debugcheck( "%lu", Number() );
	Q_ASSERT( lxor( !BeingCarried(), carrier ) );
	
	float dx;
	float dz;
	float energyUsed = 0;
	
	// just do x & z dimensions in this version
    SaveLastPosition();
	
	if( BeingCarried() )  // the agent carrying this agent initiated the update
	{
		setx( carrier->x() );
		setz( carrier->z() );
		dx = x() - LastX();
		dz = z() - LastX();
	}
	else  // this is a normal update (the agent is not being carried)
	{
	#if TestWorld
		dx = dz = 0.0;
	#else
		float dpos = nerves.speed->get() * geneCache.maxSpeed * gSpeed2DPosition;
		if( dpos > gMaxVelocity )
			dpos = gMaxVelocity;
		dx = -dpos * sin( yaw() * DEGTORAD );
		dz = -dpos * cos( yaw() * DEGTORAD );
		addx( dx );
		addz( dz );
	#endif
	}

#if ! TestWorld
  #if DirectYaw
	setyaw( nerves.yaw->get() * 360.0 );
  #else
   #if UseLightForOpposingYawHack
	float dyaw = (nerves.yaw->get() - nerves.light->get()) * geneCache.maxSpeed * gYaw2DYaw;
   #else
	float dyaw = (2.0 * nerves.yaw->get() - 1.0) * geneCache.maxSpeed * gYaw2DYaw;
   #endif
	addyaw(dyaw);
  #endif
#endif

	// Whether being carried or not, behaviors cost energy
    float energyused = nerves.eat->get()   * gEat2Energy
                     + nerves.mate->get()  * gMate2Energy
                     + nerves.fight->get() * gFight2Energy
                     + nerves.speed->get() * fSpeed2Energy
                     + fabs(2.0*nerves.yaw->get() - 1.0) * fYaw2Energy
                     + nerves.light->get() * gLight2Energy
                     + fBrain->BrainEnergy()
                     + gFixedEnergyDrain;

	if( genome::gEnableGive )
	{
		energyused += nerves.give->get() * gGive2Energy;
	}
	
	if( genome::gEnableCarry )
	{
		energyused += CarryEnergy();	// depends on number and size of items being carried
	}

    double denergy = energyused * Strength();

	// Apply large-population energy penalty
	double populationEnergyPenalty;
#if UniformPopulationEnergyPenalty
	populationEnergyPenalty = gPopulationPenaltyFraction * 0.5 * (gMaxMaxEnergy + gMinMaxEnergy);
#else
	populationEnergyPenalty = gPopulationPenaltyFraction * fMaxEnergy;
#endif
	denergy += populationEnergyPenalty;
	
	// Apply low-population energy advantage
	denergy *= gLowPopulationAdvantageFactor;	// if population is getting too low, reduce energy consumption

    fEnergy -= denergy;
    fFoodEnergy -= denergy;

	energyUsed = denergy;

	SetRed( nerves.fight->get() );	// set red color according to desire to fight
	SetBlue( nerves.mate->get() ); 	// set blue color according to desire to mate
  	if( gBodyGreenChannel == BGC_LIGHT )
	{
		SetGreen(nerves.light->get());
	}

	if( gNoseColor == NC_LIGHT )
	{
		fNoseColor[0] = fNoseColor[1] = fNoseColor[2] = nerves.light->get();
	}

    fAge++;

	// Do collision detection for barriers, edges, and solid objects, if not being carried
	if( ! BeingCarried() )
	{
		// Do barrier overrun testing here...
		// apply a multiplicative Fudge Factor (FF) to keep agents from mating
		// *across* the barriers
		//
		// For barrier collisions only, use the "CarryRadius" instead of the radius.
		// The CarryRadius takes into account all agents this agent is carrying, so
		// it can't push one of the agents it is carrying through a barrier and drop
		// it on the other side or let the carried agent mate with an agent on the
		// other side.
		
		barrier* b = NULL;
		barrier::gXSortedBarriers.reset();
		while( barrier::gXSortedBarriers.next(b) )
		{
			if( (b->xmax() > (    x() - FF * CarryRadius())) ||
				(b->xmax() > (LastX() - FF * CarryRadius())) )
			{
				// end of barrier comes after beginning of agent
				// in either its new or old position
				if( (b->xmin() > (    x() + FF * CarryRadius())) &&
					(b->xmin() > (LastX() + FF * CarryRadius())) )
				{
					// beginning of barrier comes after end of agent,
					// in both new and old positions,
					// so there is no overlap, and we can stop searching
					// for this agent's possible barrier overlaps
					break;  // get out of the sorted barriers while loop
				}
				else // we have an overlap in x
				{
					if( ((b->zmin() < ( z() + FF * CarryRadius())) || (b->zmin() < (LastZ() + FF * CarryRadius()))) &&
						((b->zmax() > ( z() - FF * CarryRadius())) || (b->zmax() > (LastZ() - FF * CarryRadius()))) )
					{
						// also overlap in z, so there may be an intersection
						float dist  = b->dist(     x(),     z() );
						float disto = b->dist( LastX(), LastZ() );
						float p;
						
						if( fabs( dist ) < FF * CarryRadius() )
						{
							// they actually overlap/intersect
							if( (disto*dist) < 0.0 )
							{   // sign change, so crossed the barrier already
								p = fabs( dist ) + FF * CarryRadius();
								if( disto < 0.0 ) p = -p;
							}
							else
							{
								p = FF * CarryRadius() - fabs( dist );
								if( dist < 0. ) p = -p;
							}

							addz(  p * b->sina() );
							addx( -p * b->cosa() );

						} // actual intersection
						else if( (disto * dist) < 0.0 )
						{
							// the agent completely passed through the barrier
							p = fabs( dist ) + FF * CarryRadius();
							
							if( disto < 0.0 )
								p = -p;
															
							addz(  p * b->sina() );
							addx( -p * b->cosa() );
						}

						fSimulation->UpdateCollisionsLog( this, OT_BARRIER );
					} // overlap in z
				} // beginning of barrier comes after end of agent
			} // end of barrier comes after beginning of agent
		} // while( barrier::gXSortedBarriers.next(b) )

		// If there are solid objects besides bricks, or
		// if only bricks are solid and bricks are present in the simulation...
		if( ( (solidObjects != BRICKTYPE) && (solidObjects > 0) ) ||
			( (solidObjects == BRICKTYPE) && (brick::GetNumBricks() > 0) ) )
		{
			// If the agent moves, then we want to do collision avoidance
			if( dx != 0.0 || dz != 0.0 )
			{
				AvoidCollisions( solidObjects );
			}
		}

	#if 0
		TODO
		// Do a check
		barrier::gXSortedBarriers.reset();
		while( barrier::gXSortedBarriers.next( b ) )
		{
			float dist  = b->dist(     x(),     z() );
			float disto = b->dist( LastX(), LastZ() );

			if( (disto * dist) < 0.0 )
			{
				cout << "****Got one! moving from (" << LastX() cm LastZ() << ") to (" << x() cm z() pnlf;
				cout << "fRadius = " << radius() nlf;
			}
		}
	#endif

		// Hardwire knowledge of world boundaries for now
		// Need to do something special with the agent list,
		// when the agents do a wraparound (for the sake of efficiency
		// and possibly correctness in the sort)
		if( globals::edges )
		{
			bool collision = false;

			if( fPosition[0] > globals::worldsize )
			{
				collision = true;
				if( globals::wraparound )
					fPosition[0] -= globals::worldsize;
				else
					fPosition[0] = globals::worldsize;
			}
			else if( fPosition[0] < 0.0 )
			{
				collision = true;
				if( globals::wraparound )
					fPosition[0] += globals::worldsize;
				else
					fPosition[0] = 0.0;
			}
			
			if( fPosition[2] < -globals::worldsize )
			{
				collision = true;
				if( globals::wraparound )
					fPosition[2] += globals::worldsize;
				else
					fPosition[2] = -globals::worldsize;
			}
			else if( fPosition[2] > 0.0 )
			{
				collision = true;
				if( globals::wraparound )
					fPosition[2] -= globals::worldsize;
				else
					fPosition[2] = 0.0;
			}

			if( collision )
			{
				fSimulation->UpdateCollisionsLog( this, OT_EDGE );
			}
		}
	} // if( ! BeingCarried() )

	// Keep track of the domain in which the agent resides
    short newDomain = fSimulation->WhichDomain( fPosition[0], fPosition[2], fDomain );
    if( newDomain != fDomain )
    {
        fSimulation->SwitchDomain( newDomain, fDomain, AGENTTYPE );
        fDomain = newDomain;
    }
        
#ifdef OF1
    if( fDomain == 0 )
        myt0++;
#endif

    fVelocity[0] = x() - LastX();
    fVelocity[2] = z() - LastZ();

	fSpeed = sqrt( fVelocity[0]*fVelocity[0] + fVelocity[2]*fVelocity[2] );
	
	if( fSpeed > fMaxSpeed )
		fMaxSpeed = fSpeed;

	rewardmovement( moveFitnessParam, speed2dpos );

	RecordPosition();
	
	// Now update any objects we are carrying
	// (They will not be updated in TSimulation::UpdateAgents*().)
	itfor( gObjectList, fCarries, it )
	{
		gobject* carried = *it;
		switch( carried->getType() )
		{
			case AGENTTYPE:
				energyUsed += ((agent*)carried)->UpdateBody( moveFitnessParam,
															speed2dpos,
															solidObjects,
															this );
				// carried agent's domain will be taken care of in its UpdateBody() call
				break;
			
			case FOODTYPE:
				carried->setx( x() );
				carried->setz( z() );
				fSimulation->SwitchDomain( Domain(), ((food*)carried)->domain(), FOODTYPE );
				((food*)carried)->domain( Domain() );
				break;
			
			case BRICKTYPE:
				carried->setx( x() );
				carried->setz( z() );
				// bricks do not currently identify their domain, nor are they counted in domains
				break;
			
			default:
				Q_ASSERT_X( false, "updating carried objects", "encountered unknown object type" );
				break;
		}
	}
    
    return energyUsed;
}


void agent::AvoidCollisions( int solidObjects )
{
	// Save the current agent pointer in the master x-sorted list before we mess with it, so we can restore it later
	objectxsortedlist::gXSortedObjects.setMark( AGENTTYPE );
	
	// First look backwards in the list (to the left, decreasing x)
	AvoidCollisionDirectional( PREV, solidObjects );

	// Restore the current agent pointer in the master x-sorted list
	objectxsortedlist::gXSortedObjects.toMark( AGENTTYPE );

	// Then look forwards in the list (to the right, increasing x)
	AvoidCollisionDirectional( NEXT, solidObjects );
	
	// Restore the current agent pointer in the master x-sorted list
	objectxsortedlist::gXSortedObjects.toMark( AGENTTYPE );
}


// WARNING:  AvoidCollisionDirectional assumes it will not be called
// with both dx == 0.0 and dz == 0.0.  This is normally taken care of
// in Update() before calling AvoidCollisions().
void agent::AvoidCollisionDirectional( int direction, int solidObjects )
{
	#define CollisionRadiusReductionFactor 0.90
	
	gobject* obj;
	
	float dx = x() - LastX();
	float dz = z() - LastZ();
	float agtRadius = radius() * CollisionRadiusReductionFactor;

	// Look in the specified direction
	while( objectxsortedlist::gXSortedObjects.anotherObj( direction, solidObjects, &obj ) )
	{
		float objRadius = obj->radius() * CollisionRadiusReductionFactor;
		
		// Test to see if we're close enough in x; if not, get out, we're done,
		// because all objects after this one are even farther away
		// Note: anotherObj() will complain and exit if direction is neither NEXT nor PREV
		if( direction == NEXT )
		{
			if( obj->x() - objRadius > max( x(), LastX() ) + agtRadius )
				break;
		}
		else	// direction == PREV
		{
			if( obj->x() + objRadius < min( x(), LastX() ) - agtRadius)
				break;
		}
		
		// Test to see if we're too far away in z; if so, we're done with this object
		if( obj->z() - objRadius > max( z(), LastZ() ) + agtRadius  ||
			obj->z() + objRadius < min( z(), LastZ() ) - agtRadius )
			continue;
		
		// If we're carrying the object, then there's nothing to be done
		if( Carrying( obj ) )
			continue;
		
		// If we reach here, then the two objects appear to have had contact this time step
		// and we're not carrying the other object
		
		// We only want to adjust the position of our agent if it was traveling in the
		// direction of the object it is touching, so take a small step from the start
		// position towards the end position and see whether the distance to the potential
		// collision object decreases.  ("Small" because we want to avoid the case where
		// the agent's velocity is great enough to step past the collision object and end
		// up farther away than it started, after going completely through the collision
		// object.  Dividing by worldsize should take care of that in any situation.)
		float xs, zs;
		float dosquared = (obj->x()-LastX())*(obj->x()-LastX()) + (obj->z()-LastZ())*(obj->z()-LastZ());
		if( fabs( dx ) > fabs( dz ) )
		{
			float s = dz / dx;
			xs = LastX()  +  dx / globals::worldsize;
			zs = LastZ()  +  s * (xs - LastX());
		}
		else
		{
			float s = dx / dz;
			zs = LastZ()  +  dz / globals::worldsize;
			xs = LastX()  +  s * (zs - LastZ());
		}
		float dssquared = (obj->x()-xs)*(obj->x()-xs) + (obj->z()-zs)*(obj->z()-zs);
		
		// Test to see if the agent is approaching the potential collision object
		if( dssquared < dosquared )
		{
			// If we reach here, then there was a collision
			// So calculate where along our path we had to stop in order to avoid it
			float xf, zf;	// the "fixed" coordinates so as to avoid penetrating the brick
			GetCollisionFixedCoordinates( LastX(), LastZ(), x(), z(), obj->x(), obj->z(), agtRadius, objRadius, &xf, &zf );
			setx( xf );
			setz( zf );

			ObjectType ot;
			switch(obj->getType())
			{
			case AGENTTYPE:
				ot = OT_AGENT;
				break;
			case FOODTYPE:
				ot = OT_FOOD;
				break;
			case BRICKTYPE:
				ot = OT_BRICK;
				break;
			default:
				assert(false);
				break;
			}

			fSimulation->UpdateCollisionsLog( this, ot );
			//break;	// can only hit one
		}
	}
}


// Cheap and dirty algorithm for finding a "fixed" position for the agent, so it avoids a collision with a brick
// Treats both the agent and the brick as circles and calculates the point along the agent's trajectory
// that will place it adjacent to, but not penetrating the brick.  Math is in my notebook.
// x and z are coordinates
// o and n are old and new agent positions (beginning and end of time step, before being fixed)
// r is for radius
// b is for object
// f is for fixed agent position (to avoid collision)
// d is for distance (or delta)
void agent::GetCollisionFixedCoordinates( float xo, float zo, float xn, float zn, float xb, float zb, float rc, float rb, float *xf, float *zf )
{
	float xf1, zf1, xf2, zf2;
	float dx = xn - xo;
	float dz = zn - zo;
	float dsquared = dx*dx + dz*dz;
	
	if( dx == 0.0 && dz == 0.0 )
	{
		*xf = xn;
		*zf = zn;
		return;
	}
	
	if( fabs( dx ) > fabs( dz ) )
	{
		float s = dz / dx;
		float a = 1 + s*s;
		float b = 2.0 * (s * (zo - zb - s*xo) - xb);
		float c = xb*xb + zb*zb + s*s*xo*xo + zo*zo + 2.0 * (s * xo * (zb-zo) - zb*zo) - (rc+rb)*(rc+rb);
		float discriminant = b*b - 4.0*a*c;
		if( discriminant < 0.0 )
		{
			// roots are not real; shouldn't be possible, but protect against it
			*xf = xn;
			*zf = zn;
			return;
		}
		float d = sqrt(discriminant);
		float e = 0.5 / a;		
		xf1 = (-b + d) * e;
		xf2 = (-b - d) * e;
		zf1 = zo  +  s * (xf1 - xo);
		zf2 = zo  +  s * (xf2 - xo);
	}
	else
	{
		float s = dx / dz;
		float a = 1 + s*s;
		float b = 2.0 * (s * (xo - xb - s*zo) - zb);
		float c = xb*xb + zb*zb + s*s*zo*zo + xo*xo + 2.0 * (s * zo * (xb-xo) - xb*xo) - (rc+rb)*(rc+rb);
		float discriminant = b*b - 4.0*a*c;
		if( discriminant < 0.0 )
		{
			// roots are not real; shouldn't be possible, but protect against it
			*xf = xn;
			*zf = zn;
			return;
		}
		float d = sqrt(discriminant);
		float e = 0.5 / a;		
		zf1 = (-b + d) * e;
		zf2 = (-b - d) * e;
		xf1 = xo  +  s * (zf1 - zo);
		xf2 = xo  +  s * (zf2 - zo);
	}

	float d1squared = (xf1-xo)*(xf1-xo) + (zf1-zo)*(zf1-zo);
	float d2squared = (xf2-xo)*(xf2-xo) + (zf2-zo)*(zf2-zo);
	if( d1squared < d2squared )
	{
		if( d1squared < dsquared )
		{
			*xf = xf1;
			*zf = zf1;
		}
		else
		{
			*xf = xn;
			*zf = zn;
		}
	}
	else
	{
		if( d2squared < dsquared )
		{
			*xf = xf2;
			*zf = zf2;
		}
		else
		{
			*xf = xn;
			*zf = zn;
		}	
	}
}


//---------------------------------------------------------------------------
// agent::Pickup
//---------------------------------------------------------------------------
void agent::PickupObject( gobject* o )
{
    debugcheck( "%lu", Number() );
	
	o->PickedUp( (gobject*)this, ly() );
	fCarries.push_back( o );
	if( o->radius() > fCarryRadius )
		fCarryRadius = o->radius();

	fSimulation->UpdateCarryLog( this,
								 o,
								 TSimulation::CA__PICKUP );
}


//---------------------------------------------------------------------------
// agent::DropMostRecent
//---------------------------------------------------------------------------
void agent::DropMostRecent( void )
{
    debugcheck( "%lu", Number() );
	
	gobject* o = fCarries.back();
	o->Dropped();
	fCarries.pop_back();
	
	if( o->radius() == fCarryRadius )
	{
		RecalculateCarryRadius();
	}

	fSimulation->UpdateCarryLog( this,
								 o,
								 TSimulation::CA__DROP_RECENT );
}


//---------------------------------------------------------------------------
// agent::DropObject
//---------------------------------------------------------------------------
void agent::DropObject( gobject* o )
{
    debugcheck( "agent # %lu (carrying %d) dropping %s # %lu (carrying %d)", Number(), NumCarries(), OBJECTTYPE( o ), o->getTypeNumber(), o->NumCarries() );
	
	o->Dropped();
	fCarries.remove( o );
	if( o->radius() == fCarryRadius )
	{
		RecalculateCarryRadius();
	}

	fSimulation->UpdateCarryLog( this,
								 o,
								 TSimulation::CA__DROP_OBJECT );
}


//---------------------------------------------------------------------------
// agent::RecalculateCarryRadius
//---------------------------------------------------------------------------
void agent::RecalculateCarryRadius( void )
{
	float newCarryRadius = fRadius;
	itfor( gObjectList, fCarries, it )
	{
		if( (*it)->radius() > newCarryRadius )
			newCarryRadius = (*it)->radius();
	}
	fCarryRadius = newCarryRadius;
}


//---------------------------------------------------------------------------
// agent::draw
//---------------------------------------------------------------------------
void agent::draw()
{
	glPushMatrix();
		position();
		glScalef(fScale, fScale, fScale);
		gpolyobj::drawcolpolyrange(0, 4, fNoseColor);
		gpolyobj::drawcolpolyrange(5, 9, fColor);
//		fCamera.draw();
	glPopMatrix();
}


void agent::print()
{
    cout << "Printing agent #" << getTypeNumber() nl;
    gobject::print();
    cout << "  fAge = " << fAge nl;
    cout << "  fLastMate = " << fLastMate nl;
    cout << "  fEnergy = " << fEnergy nl;
    cout << "  fFoodEnergy = " << fFoodEnergy nl;
    cout << "  fMaxEnergy = " << fMaxEnergy nl;
    cout << "  myspeed2energy = " << fSpeed2Energy nl;
    cout << "  fYaw2Energy = " << fYaw2Energy nl;
    cout << "  fSizeAdvantage = " << fSizeAdvantage nl;
    cout << "  fLengthX = " << fLengthX nl;
    cout << "  fLengthZ = " << fLengthZ nl;
    cout << "  fVelocity[0], fVelocity[2] = " << fVelocity[0] cms fVelocity[2] nl;
    cout << "  fHeuristicFitness = " << fHeuristicFitness nl;
    
    if (fGenome != NULL)
    {
        cout << "  fGenome->Lifespan() = " << MaxAge() nl;
        cout << "  fGenome->MutationRate() = " << fGenome->get( "MutationRate" ) nl;
        cout << "  fGenome->Strength() = " << Strength() nl;
        cout << "  fGenome->Size() = " << Size() nl;
        cout << "  fGenome->MaxSpeed() = " << geneCache.maxSpeed nl;
    }
    else
    {
        cout << "  genome is not yet defined" nl;
	}
	      
    if (fBrain != NULL)
    {
        cout << "  fBrain->brainenergy() = " << fBrain->BrainEnergy() nl;
        cout << "  fBrain->eat() = " << nerves.eat->get() nl;
        cout << "  fBrain->mate() = " << nerves.mate->get() nl;
        cout << "  fBrain->fight() = " << nerves.fight->get() nl;
        cout << "  fBrain->speed() = " << nerves.speed->get() nl;
        cout << "  fBrain->yaw() = " << nerves.yaw->get() nl;
        cout << "  fBrain->light() = " << nerves.light->get() nl;
    }
    else
        cout << "  brain is not yet defined" nl;
    cout.flush();
}


//---------------------------------------------------------------------------
// agent::FieldOfView
//---------------------------------------------------------------------------
float agent::FieldOfView()
{
	return nerves.focus->get() * (gMaxFocus - gMinFocus) + gMinFocus;
}


//---------------------------------------------------------------------------
// agent::NumberToName
//---------------------------------------------------------------------------
void agent::NumberToName()
{
	char tempstring[256];
	sprintf(tempstring, "agent#%ld", agentsEver);
	this->SetName(tempstring);
}


//---------------------------------------------------------------------------
// agent::Heal
//---------------------------------------------------------------------------
void agent::Heal( float HealingRate, float minFoodEnergy)
{
	// if agent has some FoodEnergy to spare, and agent can receive some Energy.
	if( ( fFoodEnergy > minFoodEnergy) && (fMaxEnergy > fEnergy) )		
	{
		// which is the smallest: healing rate, amount agent can give, or amount agent can receive?
		float cangive = fminf( HealingRate, fFoodEnergy - minFoodEnergy );
		float delta = fminf( cangive, fMaxEnergy - fEnergy );

		fFoodEnergy -= delta;					// take delta away from FoodEnergy
		fEnergy     += delta;					// and add it to Energy
	}
}


//---------------------------------------------------------------------------
// agent::Carrying
//---------------------------------------------------------------------------
bool agent::Carrying( gobject* o )
{
	return (o->CarriedBy() == (gobject*)this);
}


//---------------------------------------------------------------------------
// agent::CarryEnergy
//---------------------------------------------------------------------------
float agent::CarryEnergy( void )
{
	float energy = 0.0;
	
	itfor( gObjectList, fCarries, it )
	{
		gobject* o = *it;	// the object being carried
		
		switch( o->getType() )
		{
			case AGENTTYPE:
				energy += agent::gCarryAgent2Energy +
						  agent::gCarryAgentSize2Energy * (((agent*)o)->Size() - agent::gMinAgentSize) / (agent::gMaxAgentSize - agent::gMinAgentSize);
				break;
			
			case FOODTYPE:
				energy += food::gCarryFood2Energy * o->radius() / food::gMaxFoodRadius;
				break;
			
			case BRICKTYPE:
				energy += brick::gCarryBrick2Energy;	// all bricks are the same size currently
				break;
			
			default:
				error( 2, "unknown object type in CarryEnergy()" );
				break;
		}
	}
	
	return energy;
}


//---------------------------------------------------------------------------
// agent::RecordPosition
//---------------------------------------------------------------------------
void agent::RecordPosition( void )
{
	if( fPositionWriter )
	{
		//printf( "%3lu %3lu  %6.2f  %6.2f\n", fSimulation->fStep, getTypeNumber(), LastX(), x() );
		if( LastX() > 5.0  &&  x() == 0.0 )
			printf( "Got one: %3lu %3lu  %6.2f  %6.2f\n", fSimulation->fStep, getTypeNumber(), LastX(), x() );
		fPositionWriter->addRow( fSimulation->fStep,
								 x(),
								 y(),
								 z() );
	}
}


//---------------------------------------------------------------------------
// agent::PrintCarries
//---------------------------------------------------------------------------
void agent::PrintCarries( FILE* f )
{
	fprintf( f, "%ld: %s # %lu is carrying", fSimulation->fStep, OBJECTTYPE( this ), getTypeNumber() );
	if( fCarries.size() > 0 )
	{
		itfor( gObjectList, fCarries, it )
		{
			gobject* o = *it;
			fprintf( f, " %s # %lu", OBJECTTYPE( o ), o->getTypeNumber() );
		}
		fprintf( f, "\n" );
	}
	else
	{
		fprintf( f, " nothing\n" );
	}
}
