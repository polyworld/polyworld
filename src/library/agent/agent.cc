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

// Local
#include "AgentPovRenderer.h"
#include "BeingCarriedSensor.h"
#include "CarryingSensor.h"
#include "EnergySensor.h"
#include "MateWaitSensor.h"
#include "Metabolism.h"
#include "RandomSensor.h"
#include "Retina.h"
#include "SpeedSensor.h"

#include "brain/NervousSystem.h"
#include "brain/groups/GroupsBrain.h"
#include "genome/GenomeUtil.h"
#include "graphics/graphics.h"
#include "environment/barrier.h"
#include "environment/food.h"
#include "logs/Logs.h"
#include "sim/debug.h"
#include "sim/globals.h"
#include "sim/Simulation.h"
#include "utils/AbstractFile.h"
#include "utils/datalib.h"
#include "utils/graybin.h"
#include "utils/misc.h"
#include "utils/RandomNumberGenerator.h"
#include "utils/Resources.h"

using namespace genome;

// UniformPopulationEnergyPenalty controls whether or not the population energy penalty
// is the same for all agents (1) or based on each agent's own maximum energy capacity (0).
// It is off by default.
#define UniformPopulationEnergyPenalty 0

#define DirectYaw 0

// Agent globals
bool		agent::gClassInited;
unsigned long	agent::agentsEver;
long		agent::agentsliving;
gpolyobj*	agent::agentobj;

agent::Configuration agent::config;

//---------------------------------------------------------------------------
// agent::processWorldfile
//---------------------------------------------------------------------------
void agent::processWorldfile( proplib::Document &doc )
{
    agent::config.agentHeight = doc.get( "AgentHeight" );
	agent::config.vision = doc.get( "Vision" );
	agent::config.maxVelocity = doc.get( "MaxVelocity" );
	agent::config.maxCarries = doc.get( "MaxCarries" );
	agent::config.minVisionPitch = doc.get( "MinVisionPitch" );
	agent::config.maxVisionPitch = doc.get( "MaxVisionPitch" );
	agent::config.minVisionYaw = doc.get( "MinVisionYaw" );
	agent::config.maxVisionYaw = doc.get( "MaxVisionYaw" );
	agent::config.eyeHeight = doc.get( "EyeHeight" );
    agent::config.initMateWait = doc.get( "InitMateWait" );
    agent::config.randomSeedMateWait = doc.get( "RandomSeedMateWait" );
    agent::config.minAgentSize = doc.get( "MinAgentSize" );
    agent::config.maxAgentSize = doc.get( "MaxAgentSize" );
    agent::config.minLifeSpan = doc.get( "MinLifeSpan" );
    agent::config.maxLifeSpan = doc.get( "MaxLifeSpan" );
    agent::config.minStrength = doc.get( "MinAgentStrength" );
    agent::config.maxStrength = doc.get( "MaxAgentStrength" );
    agent::config.minmaxspeed = doc.get( "MinAgentMaxSpeed" );
    agent::config.maxmaxspeed = doc.get( "MaxAgentMaxSpeed" );
    agent::config.minmateenergy = doc.get( "MinEnergyFractionToOffspring" );
    agent::config.maxmateenergy = doc.get( "MaxEnergyFractionToOffspring" );
    agent::config.minMaxEnergy = doc.get( "MinAgentMaxEnergy" );
    agent::config.maxMaxEnergy = doc.get( "MaxAgentMaxEnergy" );
    agent::config.speed2DPosition = doc.get( "MotionRate" );
    agent::config.yaw2DYaw = doc.get( "YawRate" );
	{
		string encoding = doc.get( "YawEncoding" );
		if( encoding == "Oppose" )
			agent::config.yawEncoding = agent::YE_OPPOSE;
		else if( encoding == "Squash" )
			agent::config.yawEncoding = agent::YE_SQUASH;
		else
			assert( false );
	}
    agent::config.minFocus = doc.get( "MinHorizontalFieldOfView" );
    agent::config.maxFocus = doc.get( "MaxHorizontalFieldOfView" );
    agent::config.agentFOV = doc.get( "VerticalFieldOfView" );
    agent::config.maxSizeAdvantage = doc.get( "MaxSizeFightAdvantage" );
	{
		proplib::Property &prop = doc.get( "BodyRedChannel" );
		if( (string)prop == "Fight" )
			agent::config.bodyRedChannel = agent::BRC_FIGHT;
		else if( (string)prop == "Give" )
			agent::config.bodyRedChannel = agent::BRC_GIVE;
		else
		{
			agent::config.bodyRedChannel = agent::BRC_CONST;
			agent::config.bodyRedChannelConstValue = (float)prop;
		}
	}
	{
		proplib::Property &prop = doc.get( "BodyGreenChannel" );
		if( (string)prop == "I" )
			agent::config.bodyGreenChannel = agent::BGC_ID;
		else if( (string)prop == "L" )
			agent::config.bodyGreenChannel = agent::BGC_LIGHT;
		else if( (string)prop == "E" )
			agent::config.bodyGreenChannel = agent::BGC_EAT;
		else
		{
			agent::config.bodyGreenChannel = agent::BGC_CONST;
			agent::config.bodyGreenChannelConstValue = (float)prop;
		}
	}
	{
		proplib::Property &prop = doc.get( "BodyBlueChannel" );
		if( (string)prop == "Mate" )
			agent::config.bodyBlueChannel = agent::BBC_MATE;
		else if( (string) prop == "Energy" )
			agent::config.bodyBlueChannel = agent::BBC_ENERGY;
		else
		{
			agent::config.bodyBlueChannel = agent::BBC_CONST;
			agent::config.bodyBlueChannelConstValue = (float)prop;
		}
	}
	{
		proplib::Property &prop = doc.get( "NoseColor" );
		if( (string)prop == "L" )
			agent::config.noseColor = agent::NC_LIGHT;
		else if( (string)prop == "B" )
			agent::config.noseColor = agent::NC_BODY;
		else
		{
			agent::config.noseColor = agent::NC_CONST;
			agent::config.noseColorConstValue = (float)prop;
		}
	}
    agent::config.hasLightBehavior = agent::config.bodyGreenChannel == agent::BGC_LIGHT || agent::config.noseColor == agent::NC_LIGHT;
    agent::config.randomSeedEnergy = doc.get( "RandomSeedEnergy" );
    agent::config.energyUseMultiplier = doc.get( "EnergyUseMultiplier" );
    agent::config.ageEnergyMultiplier = doc.get( "AgeEnergyMultiplier" );
    agent::config.dieAtMaxAge = doc.get( "DieAtMaxAge" );
    agent::config.starvationEnergyFraction = doc.get( "StarvationEnergyFraction" );
    agent::config.starvationWait = doc.get( "StarvationWait" );
    agent::config.eat2Energy = doc.get( "EnergyUseEat" );
	agent::config.mate2Energy = doc.get( "EnergyUseMate" );
    agent::config.fight2Energy = doc.get( "EnergyUseFight" );
    agent::config.give2Energy = doc.get( "EnergyUseGive" );
	agent::config.minSizePenalty = doc.get( "MinSizeEnergyPenalty" );
    agent::config.maxSizePenalty = doc.get( "MaxSizeEnergyPenalty" );
    agent::config.speed2Energy = doc.get( "EnergyUseMove" );
    agent::config.yaw2Energy = doc.get( "EnergyUseTurn" );
    agent::config.light2Energy = doc.get( "EnergyUseLight" );
    agent::config.focus2Energy = doc.get( "EnergyUseFocus" );
	agent::config.pickup2Energy = doc.get( "EnergyUsePickup" );
	agent::config.drop2Energy = doc.get( "EnergyUseDrop" );
	agent::config.carryAgent2Energy = doc.get( "EnergyUseCarryAgent" );
	agent::config.carryAgentSize2Energy = doc.get( "EnergyUseCarryAgentSize" );
    agent::config.fixedEnergyDrain = doc.get( "EnergyUseFixed" );

	agent::config.enableMateWaitFeedback = doc.get( "EnableMateWaitFeedback" );
	agent::config.enableSpeedFeedback = doc.get( "EnableSpeedFeedback" );
	agent::config.enableGive = doc.get( "EnableGive" );
	agent::config.enableCarry = doc.get( "EnableCarry" );
	agent::config.enableVisionPitch = doc.get( "EnableVisionPitch" );
	agent::config.enableVisionYaw = doc.get( "EnableVisionYaw" );
}

//---------------------------------------------------------------------------
// agent::agent
//---------------------------------------------------------------------------
agent::agent(TSimulation* sim, gstage* stage)
	:	fSimulation(sim),
		fAlive(false), 		// must grow() to be truly alive
    	fDeathByPatch(false),
		fMass(0.0), 		// mass - not used
		fHeuristicFitness(0.0),  	// crude guess for keeping minimum population early on
		fComplexity(-1.0),	// < 0.0 indicates not yet computed
		fGenome(NULL),
		fCns(NULL),
		fRetina(NULL),
		fRandomSensor(NULL),
		fEnergySensor(NULL),
		fMateWaitSensor(NULL),
		fSpeedSensor(NULL),
		fCarryingSensor(NULL),
		fBeingCarriedSensor(NULL)
{
	AgentAttachedData::alloc( this );
	
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

	fLastEatPosition[0] = 0.0;
	fLastEatPosition[1] = 0.0;
	fLastEatPosition[2] = 0.0;

	fGenome = GenomeUtil::createGenome();
	fCns = new NervousSystem();
	fMetabolism = NULL;
	
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
	delete fRandomSensor;
	delete fEnergySensor;
	delete fMateWaitSensor;
	delete fSpeedSensor;
	delete fCarryingSensor;
	delete fBeingCarriedSensor;
	delete fRetina;

	AgentAttachedData::dispose( this );
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
	// Create the new agent
	agent* c = new agent(simulation, stage);	
	
    // Increase current total of creatures alive
    agent::agentsliving++;
    
    // Set number to total creatures that have ever lived (note this is 1-based)
    c->setTypeNumber( ++agent::agentsEver );
	c->fCns->getRNG()->seedIfLocal( agent::agentsEver );

	simulation->GetAgentPovRenderer()->add( c );
		
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
    WARN_ONCE("agent::agentload called. Not supported.");
#if 0
    in >> agent::agentsEver;
    in >> agent::agentsliving;

    agent::agentlist->load(in);
	
	long i;
		
    for (i = 0; i < agent::config.maxNumAgents; i++)
        if (!agent::pc[i])
            break;
    if (i)
        error(2,"agent::pc[] array not empty during agentload");
        
    //    if (agent::config.xSortedAgents.count())
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
            //agent::pc[i]->listLink = agent::config.xSortedAgents.add(agent::pc[i]);
	    	agent::pc[i]->listLink = allxsortedlist::gXSortedAll.add(agent::pc[i]);
            globals::worldstage.addobject(agent::pc[i]);
        }
    }
#endif
}


//---------------------------------------------------------------------------
// agent::dump
//---------------------------------------------------------------------------
void agent::dump(ostream& out)
{
    out << getTypeNumber() nl;
    out << fAge nl;
    out << fLastMate nl;
    assert( false ); // out << fEnergy nl;
    assert( false ); // out << fFoodEnergy nl;
    assert( false ); // out << fMaxEnergy nl;
    out << fSpeed2Energy nl;
    out << fYaw2Energy nl;
    out << fSizeAdvantage nl;
    out << fMass nl;
    out << fLastPosition[0] sp fLastPosition[1] sp fLastPosition[2] nl;
    out << fVelocity[0] sp fVelocity[1] sp fVelocity[2] nl;
    out << fNoseColor[0] sp fNoseColor[1] sp fNoseColor[2] nl;
    out << fHeuristicFitness nl;

    gobject::dump(out);

    	assert( false ); //fGenome->dump(out); // must port this to AbstractFile

		/* implement
    if (fBrain != NULL)
        fBrain->Dump(out);
    else
        error(1, "Attempted to dump a agent with no brain");
		*/
}


//---------------------------------------------------------------------------
// agent::load
//---------------------------------------------------------------------------    
void agent::load(istream& in)
{
	WARN_ONCE("fix domain issue");
	
	unsigned long agentNumber;
	
    in >> agentNumber;
	setTypeNumber( agentNumber );
    in >> fAge;
    in >> fLastMate;
    assert( false ); // in >> fEnergy;
    assert( false ); // in >> fFoodEnergy;
    assert( false ); // in >> fMaxEnergy;
    in >> fSpeed2Energy;
    in >> fYaw2Energy;
    in >> fSizeAdvantage;
    in >> fMass;
    in >> fLastPosition[0] >> fLastPosition[1] >> fLastPosition[2];
    in >> fVelocity[0] >> fVelocity[1] >> fVelocity[2];
    in >> fNoseColor[0] >> fNoseColor[1] >> fNoseColor[2];
    in >> fHeuristicFitness;

    gobject::load(in);
	
    assert( false ); //fGenome->load(in); // must port this to AbstractFile
	/* implement
    if (fBrain == NULL)
    {
        fBrain = new Brain(fCns);
        Q_CHECK_PTR(fBrain);
    }
    fBrain->Load(in);
	*/

    // done loading in raw information, now setup some derived quantities
    SetGeometry();
    SetGraphics();
    fDomain = fSimulation->WhichDomain(fPosition[0], fPosition[2], 0);
}

//---------------------------------------------------------------------------
// agent::setGenomeReady
//---------------------------------------------------------------------------
void agent::setGenomeReady()
{
	switch( Metabolism::selectionMode )
	{
	case Metabolism::Gene:
		fMetabolism = GenomeUtil::getMetabolism( fGenome );
		break;
	case Metabolism::Random:
		{
			int index = nint( (Metabolism::getNumberOfDefinitions() - 1) * fCns->getRNG()->drand() );
			fMetabolism = Metabolism::get( index );
		}
		break;
	default:
		assert( false );
	}
}

//---------------------------------------------------------------------------
// agent::grow
//---------------------------------------------------------------------------
void agent::grow( long mateWait, bool seeding )
{    
	InitGeneCache();

	// ---
	// --- Create Input Nerves
	// ---
#define INPUT_NERVE( NAME ) fCns->createNerve( Nerve::INPUT, NAME )
	INPUT_NERVE( "Random" );
	INPUT_NERVE( "Energy" );
	if( agent::config.enableMateWaitFeedback )
		INPUT_NERVE( "MateWaitFeedback" );
	if( agent::config.enableSpeedFeedback )
		INPUT_NERVE( "SpeedFeedback" );
	if( agent::config.enableCarry )
	{
		INPUT_NERVE( "Carrying" );
		INPUT_NERVE( "BeingCarried" );
	}
	INPUT_NERVE( "Red" );
	INPUT_NERVE( "Green" );
	INPUT_NERVE( "Blue" );
#undef INPUT_NERVE

	// ---
	// --- Create Output Nerves
	// ---
#define OUTPUT_NERVE(FIELD, NAME) outputNerves.FIELD = fCns->createNerve( Nerve::OUTPUT, NAME )
	OUTPUT_NERVE(eat, "Eat");
	OUTPUT_NERVE(mate, "Mate");
	OUTPUT_NERVE(fight, "Fight");
	OUTPUT_NERVE(speed, "Speed");
	OUTPUT_NERVE(yaw, "Yaw");
	if( agent::config.yawEncoding == YE_OPPOSE )
		OUTPUT_NERVE(yawOppose, "YawOppose");
	if( agent::config.hasLightBehavior )
		OUTPUT_NERVE(light, "Light");
	OUTPUT_NERVE(focus, "Focus");
	if( agent::config.enableVisionPitch )
		OUTPUT_NERVE(visionPitch, "VisionPitch");
	if( agent::config.enableVisionYaw )
		OUTPUT_NERVE(visionYaw, "VisionYaw");
	if( agent::config.enableGive )
		OUTPUT_NERVE(give, "Give");
	if( agent::config.enableCarry )
	{
		OUTPUT_NERVE(pickup, "Pickup");
		OUTPUT_NERVE(drop, "Drop");
	}
#undef OUTPUT_NERVE

	// ---
	// --- Create Sensors
	// ---
	fCns->addSensor( fRetina = new Retina(Brain::config.retinaWidth) );
	fCns->addSensor( fEnergySensor = new EnergySensor(this) );
	fCns->addSensor( fRandomSensor = new RandomSensor(fCns->getRNG()) );
	if( agent::config.enableMateWaitFeedback )
		fCns->addSensor( fMateWaitSensor = new MateWaitSensor(this, mateWait) );
	if( agent::config.enableSpeedFeedback )
		fCns->addSensor( fSpeedSensor = new SpeedSensor(this) );
	if( agent::config.enableSpeedFeedback )
		fCns->addSensor( fSpeedSensor = new SpeedSensor(this) );
	if( agent::config.enableCarry )
	{
		fCns->addSensor( fCarryingSensor = new CarryingSensor(this) );
		fCns->addSensor( fBeingCarriedSensor = new BeingCarriedSensor(this) );
	}

	// ---
	// --- Grow Nervous System (Brain)
	// ---
	fCns->grow( fGenome );
	logs->postEvent( BrainGrownEvent(this) );

	fCns->prebirth();
	if( Brain::config.learningMode == Brain::Configuration::LEARN_PREBIRTH )
		fCns->getBrain()->freeze();

    // setup the agent's geometry
    SetGeometry();

	// initially set red & blue to 0
    fColor[0] = fColor[2] = 0.0;
    
	// set body red channel
	switch(agent::config.bodyRedChannel)
	{
	case BRC_CONST:
		fColor[0] = agent::config.bodyRedChannelConstValue;
		break;
	case BRC_FIGHT:
	case BRC_GIVE:
		// no-op
		break;
	default:
		assert(false);
		break;
	}
    
	// set body green channel
	switch(agent::config.bodyGreenChannel)
	{
	case BGC_ID:
		fColor[1] = fGenome->get( "ID" );
		break;
	case BGC_CONST:
		fColor[1] = agent::config.bodyGreenChannelConstValue;
		break;
	case BGC_LIGHT:
	case BGC_EAT:
		// no-op
		break;
	default:
		assert(false);
		break;
	}
    
	// set body blue channel
	switch(agent::config.bodyBlueChannel)
	{
	case BBC_CONST:
		fColor[2] = agent::config.bodyBlueChannelConstValue;
		break;
	case BBC_MATE:
	case BBC_ENERGY:
		// no-op
		break;
	default:
		assert(false);
		break;
	}
    
	// set the initial nose color
	float noseColor;
	switch(agent::config.noseColor)
	{
	case NC_CONST:
		noseColor = agent::config.noseColorConstValue;
		break;
	case NC_LIGHT:
	case NC_BODY:
		// start neutral gray
		noseColor = 0.5;
		break;
	default:
		assert(false);
		break;
	}
    fNoseColor[0] = fNoseColor[1] = fNoseColor[2] = noseColor;
    
    UpdateColor();
    
    fIsSeed = seeding;
    fAge = 0;
    if( seeding )
    {
        if( agent::config.randomSeedMateWait )
        {
            fLastMate = (long)(randpw() * -mateWait);
        }
        else
        {
            fLastMate = -mateWait;
        }
    }
    else
    {
        fLastMate = agent::config.initMateWait;
    }
    
	float size_rel = geneCache.size - agent::config.minAgentSize;

    float maxEnergy = agent::config.minMaxEnergy + (size_rel
    				  * (agent::config.maxMaxEnergy - agent::config.minMaxEnergy) / (agent::config.maxAgentSize - agent::config.minAgentSize) );
    fMaxEnergy = maxEnergy;
    fStarvationFoodEnergy = agent::config.starvationEnergyFraction * maxEnergy;
	
    fEnergy = fMaxEnergy;
	fFoodEnergy = fMaxEnergy;
	
	if( seeding && agent::config.randomSeedEnergy )
	{
		float energy = (agent::config.starvationEnergyFraction + randpw() * (1.0 - agent::config.starvationEnergyFraction)) * maxEnergy;
		fEnergy = energy;
		fFoodEnergy = energy;
	}
	
//	printf( "%s: energy initialized to %g\n", __func__, fEnergy );
    
	// Note: gMinSizePenalty can be used to prevent size_rel==0 from giving
	// the agents "free" speed.
    fSpeed2Energy = agent::config.speed2Energy * geneCache.maxSpeed
		            * (agent::config.minSizePenalty + size_rel) * (agent::config.maxSizePenalty - 1.0)
					/ (agent::config.maxAgentSize - agent::config.minAgentSize);
    
	// Note: gMinSizePenalty can be used to prevent size_rel==0 from giving
	// the agents "free" yaw.
    fYaw2Energy = agent::config.yaw2Energy * geneCache.maxSpeed
		          * (agent::config.minSizePenalty + size_rel) * (agent::config.maxSizePenalty - 1.0)
              	  / (agent::config.maxAgentSize - agent::config.minAgentSize);
    
    fSizeAdvantage = 1.0 + ( size_rel *
                (agent::config.maxSizeAdvantage - 1.0) / (agent::config.maxAgentSize - agent::config.minAgentSize) );

    // now setup the camera & window for our agent to see the world in
    SetGraphics();

    fAlive = true;

	logs->postEvent( AgentGrownEvent(this) );
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
//---------------------------------------------------------------------------
void agent::eat( food* f,
				 float eatFitnessParameter,
				 float eat2consume,
				 float eatthreshold,
				 long step,
				 Energy &return_lost,
				 Energy &return_actuallyEat )
{
	return_lost = 0;
	return_actuallyEat = 0;
	
	if (outputNerves.eat->get() > eatthreshold)
	{
		Energy trytoeat = outputNerves.eat->get() * eat2consume;
		trytoeat.constrain( 0, fMaxEnergy - fEnergy );
		
		return_actuallyEat =
			f->eat(trytoeat)
			* fMetabolism->energyPolarity
			* f->getEnergyPolarity()
			* fMetabolism->eatMultiplier;

		// The eatMultiplier could have made us exceed our limits.
		return_actuallyEat.constrain( fEnergy * -1, fMaxEnergy - fEnergy );

		fEnergy += return_actuallyEat;
		fFoodEnergy += return_actuallyEat;

	#ifdef OF1
		// this isn't right anymore... it's from before multi-nutrients.
		assert( false );
		mytotein += return_actuallyEat;
	#endif

		fFoodEnergy.constrain( 0, fMaxEnergy, return_lost );
				
		fHeuristicFitness += eatFitnessParameter * return_actuallyEat.sum() / (eat2consume * MaxAge());
		
		if( !return_actuallyEat.isZero() )
		{
			fLastEat = step;
			fLastEatPosition[0] = fPosition[0];
			fLastEatPosition[1] = fPosition[1];
			fLastEatPosition[2] = fPosition[2];
		}
	}
}

//---------------------------------------------------------------------------
// agent::receive
//---------------------------------------------------------------------------    
Energy agent::receive( agent *giver, const Energy &requested )
{
	Energy amount = requested;
	amount.constrain( 0, fMaxEnergy - fEnergy );

	fEnergy += amount;
	giver->fEnergy -= amount;

	return amount;
}

//---------------------------------------------------------------------------
// agent::damage
//---------------------------------------------------------------------------    
Energy agent::damage(const Energy &e, bool nullMode)
{
	double scaleFactor = fSimulation->fLowPopulationAdvantageFactor
					   * fSimulation->fGlobalEnergyScaleFactor
					   * fSimulation->fDomains[fDomain].energyScaleFactor;
	Energy actual = e * scaleFactor;
	actual.constrain( 0, fEnergy );

	if( !nullMode )
		fEnergy -= actual;

	return actual;
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
//---------------------------------------------------------------------------    
Energy agent::mating( float mateFitnessParam, long mateWait, bool lockstep )
{
	if( mateWait <= 0 )
		mateWait = 1;
	
	Energy mymateenergy = fGenome->get( "MateEnergyFraction" ) * fEnergy;

	if( !lockstep )
	{
		fLastMate = fAge;
		fHeuristicFitness += mateFitnessParam * mateWait / MaxAge();
		fEnergy -= mymateenergy;
		fFoodEnergy -= mymateenergy;
		fFoodEnergy.constrain( 0, fMaxEnergy );
	}
		
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
    fHeuristicFitness += energyFitness * NormalizedEnergy()
              + ageFitness * fAge / MaxAge();
}
    
 
//---------------------------------------------------------------------------
// agent::ProjectedHeuristicFitness
//---------------------------------------------------------------------------        
float agent::ProjectedHeuristicFitness()
{
	if( fSimulation->LifeFractionSamples() >= 50 )
		return( fHeuristicFitness * fSimulation->LifeFractionRecent() * MaxAge() / fAge +
				fSimulation->EnergyFitnessParameter() * NormalizedEnergy() +
				fSimulation->AgeFitnessParameter() * fSimulation->LifeFractionRecent() );
	else
		return( fHeuristicFitness );
}

//---------------------------------------------------------------------------
// agent::Die
//---------------------------------------------------------------------------    
void agent::Die()
{
	// No longer alive. :(		
	fAlive = false;

	itfor( AgentListeners, listeners, it )
	{
		(*it)->died( this );
	}
	listeners.clear();

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
	assert(agent::agentsliving >= 0);
	
	fSimulation->GetAgentPovRenderer()->remove( this );
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
    srPrint( "agent::%s(): min=%g, max=%g, speed=%g, size=%g, lx=%g, lz=%g\n", __FUNCTION__, agent::config.minAgentSize, agent::config.maxAgentSize, geneCache.maxSpeed, Size(), fLengthX, fLengthZ );
    for (long i = 0; i < fNumPolygons; i++)
    {
        for (long j = 0; j < fPolygon[i].fNumPoints; j++)
        {
            fPolygon[i].fVertices[j * 3  ] *= fLengthX;
            fPolygon[i].fVertices[j * 3 + 1] *= agent::config.agentHeight;
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

	fCamera.SetAspect(fovx * Brain::config.retinaHeight / (agent::config.agentFOV * Brain::config.retinaWidth));
    fCamera.settranslation(0.0, (agent::config.eyeHeight - 0.5) * agent::config.agentHeight, -0.5 * fLengthZ);
	fCamera.SetNear(.01);
	fCamera.SetFar(1.5 * globals::worldsize);
	fCamera.SetFOV(agent::config.agentFOV);

	if( fSimulation->glFogFunction() != 'O' )
		fCamera.SetFog(true, fSimulation->glFogFunction(), fSimulation->glExpFogDensity(), fSimulation->glLinearFogEnd() );
		
	fCamera.AttachTo(this);
}


//---------------------------------------------------------------------------
// agent::InitGeneCache
//---------------------------------------------------------------------------    
void agent::InitGeneCache()
{
	geneCache.maxSpeed = fGenome->get("MaxSpeed");
	geneCache.strength = fGenome->get("Strength");
	geneCache.size = fGenome->get("Size");
	if( agent::config.dieAtMaxAge )
		geneCache.lifespan = fGenome->get("LifeSpan");
	else
		geneCache.lifespan = fSimulation->GetMaxSteps();
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
    if (agent::config.vision)
    {
		// create retinal pixmap, based on values of focus & numvisneurons
        const float fovx = outputNerves.focus->get() * (agent::config.maxFocus - agent::config.minFocus) + agent::config.minFocus;
        		
		fFrustum.Set(fPosition[0], fPosition[2], fAngle[0], fovx, agent::config.maxRadius);
		fCamera.SetAspect(fovx * Brain::config.retinaHeight / (agent::config.agentFOV * Brain::config.retinaWidth));

		if( agent::config.enableVisionPitch )
		{
			const float pitch = outputNerves.visionPitch->get() * (agent::config.maxVisionPitch - agent::config.minVisionPitch) + agent::config.minVisionPitch;
			fCamera.setpitch( pitch );
		}
		
		if( agent::config.enableVisionYaw )
		{
			const float yaw = outputNerves.visionYaw->get() * (agent::config.maxVisionYaw - agent::config.minVisionYaw) + agent::config.minVisionYaw;
			fCamera.setyaw( yaw );
		}
		
		fSimulation->GetAgentPovRenderer()->render( this );

		debugcheck( "after DrawAgentPOV" );
	}
}


//---------------------------------------------------------------------------
// agent::UpdateBrain
//---------------------------------------------------------------------------
void agent::UpdateBrain()
{
	fCns->update( false );

	logs->postEvent( BrainUpdatedEvent(this) );
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
	assert( lxor( !BeingCarried(), carrier ) );
	
	// In some simulations, we use a dynamic energy delta to shape difficulty.
	if( !fMetabolism->energyDelta.isZero() )
	{
		fEnergy += fMetabolism->energyDelta;
		fEnergy.constrain( 0, fMaxEnergy );
	}

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
		float dpos = outputNerves.speed->get() * geneCache.maxSpeed * agent::config.speed2DPosition;
		if( dpos > agent::config.maxVelocity )
			dpos = agent::config.maxVelocity;
		dx = -dpos * sin( yaw() * DEGTORAD );
		dz = -dpos * cos( yaw() * DEGTORAD );
		addx( dx );
		addz( dz );
	#endif
	}

#if ! TestWorld
  #if DirectYaw
	setyaw( outputNerves.yaw->get() * 360.0 );
  #else
	float dyaw;
	switch(agent::config.yawEncoding)
	{
	case YE_OPPOSE:
		dyaw = outputNerves.yaw->get() - outputNerves.yawOppose->get();
		break;
	case YE_SQUASH:
		dyaw = 2.0 * outputNerves.yaw->get() - 1.0;
		break;
	default:
		assert(false);
		break;
	}
	addyaw( dyaw * geneCache.maxSpeed * agent::config.yaw2DYaw );
  #endif
#endif

	// Whether being carried or not, behaviors cost energy
    float energyused = outputNerves.eat->get()   * agent::config.eat2Energy
                     + outputNerves.mate->get()  * agent::config.mate2Energy
                     + outputNerves.fight->get() * agent::config.fight2Energy
                     + outputNerves.speed->get() * fSpeed2Energy
                     + fabs(dyaw) * fYaw2Energy
                     + fCns->getEnergyUse()
                     + agent::config.fixedEnergyDrain;

	if( agent::config.hasLightBehavior )
	{
		energyused += outputNerves.light->get() * agent::config.light2Energy;
	}

	if( agent::config.enableGive )
	{
		energyused += outputNerves.give->get() * agent::config.give2Energy;
	}
	
	if( agent::config.enableCarry )
	{
		energyused += CarryEnergy();	// depends on number and size of items being carried
	}

    float denergy = energyused * Strength() * (1.0f + agent::config.ageEnergyMultiplier * fAge);

	// Apply large-population energy penalty (only if NumDepletionSteps > 0)
	float populationEnergyPenalty;
#if UniformPopulationEnergyPenalty
	populationEnergyPenalty = fSimulation->fPopulationPenaltyFraction * 0.5 * (agent::config.maxMaxEnergy + agent::config.minMaxEnergy);
#else
	populationEnergyPenalty = fSimulation->fPopulationPenaltyFraction * fMaxEnergy.mean();
#endif
	denergy += populationEnergyPenalty;
	
	// Apply energy-based population controls
	double scaleFactor = fSimulation->fLowPopulationAdvantageFactor			// if ApplyLowPopulationAdvantage True
					   * fSimulation->fGlobalEnergyScaleFactor	// if EnergyBasedPopulationControl True
					   * fSimulation->fDomains[fDomain].energyScaleFactor;
	denergy *= scaleFactor;	// if population is getting too low or too high, adjust energy consumption
	
	denergy *= agent::config.energyUseMultiplier;	// global control over rate at which energy is consumed

    fEnergy -= denergy;
    fFoodEnergy -= denergy;

	energyUsed = denergy;

	UpdateColor();

    fAge++;

	bool skipDomainCheck = false;

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
						if( barrier::gStickyBarriers )
						{
							fPosition[0] = LastX();
							fPosition[2] = LastZ();
						}
						else
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
						}

						logs->postEvent( CollisionEvent(this, OT_BARRIER) );
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
		if( globals::blockedEdges )
		{
			bool collision = false;

			if( fPosition[0] > globals::worldsize )
			{
				collision = true;
				fPosition[0] = globals::worldsize;
			}
			else if( fPosition[0] < 0.0 )
			{
				collision = true;
				fPosition[0] = 0.0;
			}
			
			if( fPosition[2] < -globals::worldsize )
			{
				collision = true;
				fPosition[2] = -globals::worldsize;
			}
			else if( fPosition[2] > 0.0 )
			{
				collision = true;
				fPosition[2] = 0.0;
			}

			if( collision )
			{
				if( globals::stickyEdges )
				{
					fPosition[0] = LastX();
					fPosition[2] = LastZ();
				}

				logs->postEvent( CollisionEvent(this, OT_EDGE) );
			}
		}
		else if( globals::wraparound )
		{
			if( fPosition[0] > globals::worldsize )
				fPosition[0] -= globals::worldsize;
			else if( fPosition[0] < 0.0 )
				fPosition[0] += globals::worldsize;
			
			if( fPosition[2] < -globals::worldsize )
				fPosition[2] += globals::worldsize;
			else if( fPosition[2] > 0.0 )
				fPosition[2] -= globals::worldsize;
		}
		else
		{
			if( fPosition[0] > globals::worldsize || fPosition[0] < 0.0 ||
				fPosition[2] < -globals::worldsize || fPosition[2] > 0.0 )
				// The agent fell off a tabletop world, so it's no longer in a domain
				// We can avoid an error below by skipping the call to TSimulation::WhichDomain
				// Unfortunately, TSimulation::DeathAndStats may fail to subsequently kill the agent
				// E.g., if we're in lockstep mode or the death is prevented by population controls
				// When this occurs, the agent floats off the edge of the world without dying
				// This generally leads to an error but is in any case undesirable
				// Force an error here until the issue is resolved
				error( 2, "Possible Wile E. Coyote detected" );
				skipDomainCheck = true;
		}
	} // if( ! BeingCarried() )


	// Keep track of the domain in which the agent resides
    if( !skipDomainCheck )
    {
        short newDomain = fSimulation->WhichDomain( fPosition[0], fPosition[2], fDomain );
        if( newDomain != fDomain )
        {
            fSimulation->SwitchDomain( newDomain, fDomain, AGENTTYPE );
            fDomain = newDomain;
        }
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
				ERR( "updating carried objects; encountered unknown object type" );
		}
	}

	logs->postEvent( AgentBodyUpdatedEvent(this, energyused) );

    return energyUsed;
}


//---------------------------------------------------------------------------
// agent::UpdateColor
//---------------------------------------------------------------------------
void agent::UpdateColor()
{
	if( agent::config.bodyRedChannel == BRC_FIGHT )
	{
		SetRed( outputNerves.fight->get() );	// set red color according to desire to fight
	}
	else if( agent::config.bodyRedChannel == BRC_GIVE )
	{
		SetRed( outputNerves.give->get() );	// set red color according to desire to give
	}

  	if( agent::config.bodyGreenChannel == BGC_LIGHT )
	{
		SetGreen(outputNerves.light->get());
	}
	else if( agent::config.bodyGreenChannel == BGC_EAT )
	{
		SetGreen(outputNerves.eat->get());
	}

	if( agent::config.bodyBlueChannel == BBC_MATE )
	{
		SetBlue( outputNerves.mate->get() ); 	// set blue color according to desire to mate
	}
	else if( agent::config.bodyBlueChannel == BBC_ENERGY )
	{
		SetBlue( 1 - NormalizedEnergy() );
	}

	if( agent::config.noseColor == NC_LIGHT )
	{
		fNoseColor[0] = fNoseColor[1] = fNoseColor[2] = outputNerves.light->get();
	}
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

			logs->postEvent( CollisionEvent(this, ot) );
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

	logs->postEvent( CarryEvent(this,
								CarryEvent::Pickup,
								o) );
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

	logs->postEvent( CarryEvent(this,
								CarryEvent::DropRecent,
								o) );
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

	logs->postEvent( CarryEvent(this,
								CarryEvent::DropObject,
								o) );
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
		if( agent::config.noseColor == agent::NC_BODY )
			gpolyobj::drawcolpolyrange(0, 4, fColor);
		else
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
    //cout << "  fEnergy = " << fEnergy nl; // etodo
    //cout << "  fFoodEnergy = " << fFoodEnergy nl; // etodo
    //cout << "  fMaxEnergy = " << fMaxEnergy nl; // etodo
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
	      
	cout << "  fBrain->brainenergy() = " << fCns->getEnergyUse() nl;
	cout << "  fBrain->eat() = " << outputNerves.eat->get() nl;
	cout << "  fBrain->mate() = " << outputNerves.mate->get() nl;
	cout << "  fBrain->fight() = " << outputNerves.fight->get() nl;
	cout << "  fBrain->speed() = " << outputNerves.speed->get() nl;
	cout << "  fBrain->yaw() = " << outputNerves.yaw->get() nl;
	if (agent::config.hasLightBehavior)
	{
		cout << "  fBrain->light() = " << outputNerves.light->get() nl;
	}

    cout.flush();
}


//---------------------------------------------------------------------------
// agent::NormalizedYaw
//---------------------------------------------------------------------------
float agent::NormalizedYaw()
{
	switch(agent::config.yawEncoding)
	{
	case YE_OPPOSE:
		return clamp( ((outputNerves.yaw->get() - outputNerves.yawOppose->get()) * geneCache.maxSpeed) / agent::config.maxmaxspeed, -1.0, 1.0 );
	case YE_SQUASH:
		return clamp( ((2.0 * outputNerves.yaw->get() - 1.0) * geneCache.maxSpeed) / agent::config.maxmaxspeed, -1.0, 1.0 );
	default:
		assert(false);
		return 0.0f;
	}
}

//---------------------------------------------------------------------------
// agent::FieldOfView
//---------------------------------------------------------------------------
float agent::FieldOfView()
{
	return outputNerves.focus->get() * (agent::config.maxFocus - agent::config.minFocus) + agent::config.minFocus;
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
void agent::Heal( float healingRate, float minFoodEnergy)
{
	// etodo
	assert( false );
	/*
	// if agent has some FoodEnergy to spare, and agent can receive some Energy.
	if( ( fFoodEnergy > minFoodEnergy) && (fMaxEnergy > fEnergy) )		
	{
		// which is the smallest: healing rate, amount agent can give, or amount agent can receive?
		float cangive = fminf( healingRate, fFoodEnergy - minFoodEnergy );
		float delta = fminf( cangive, fMaxEnergy - fEnergy );

		fFoodEnergy -= delta;					// take delta away from FoodEnergy
		fEnergy     += delta;					// and add it to Energy
	}
	*/
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
				energy += agent::config.carryAgent2Energy +
						  agent::config.carryAgentSize2Energy * (((agent*)o)->Size() - agent::config.minAgentSize) / (agent::config.maxAgentSize - agent::config.minAgentSize);
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
