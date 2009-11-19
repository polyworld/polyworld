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
#include "barrier.h"
#include "AgentPOVWindow.h"
#include "debug.h"
#include "food.h"
#include "globals.h"
#include "graphics.h"
#include "graybin.h"
#include "misc.h"
#include "Resources.h"
#include "Simulation.h"

#pragma mark -

// UniformPopulationEnergyPenalty controls whether or not the population energy penalty
// is the same for all agents (1) or based on each agent's own maximum energy capacity (0).
// It is off by default.
#define UniformPopulationEnergyPenalty 0

#define UseLightForOpposingYawHack 0
#define DirectYaw 0

// Agent globals
bool		agent::gClassInited;
long		agent::agentsever;
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
float		agent::gFixedEnergyDrain;
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

// agent::gNumDepletionSteps and agent:gMaxPopulationFraction must both be initialized to zero
// for backward compatibility, and we depend on that fact in ReadWorldFile(), so don't change it

int			agent::gNumDepletionSteps = 0;
double		agent::gMaxPopulationPenaltyFraction = 0.0;
double		agent::gPopulationPenaltyFraction = 0.0;
double		agent::gLowPopulationAdvantageFactor = 1.0;


// [TODO] figure out a better way to track agent indices
bitset<1000> gAgentIndex;

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
    c->fAgentNumber = ++agent::agentsever;

	// Set agent index.  Used for POV drawing.
	for (size_t index = 0; index < gAgentIndex.size(); ++index)
	{
		if (!gAgentIndex.test(index))
		{
			c->fIndex = index;
			gAgentIndex.set(index);
//			cout << "getfreeagent: c = " << c << ", fAgentNumber = " << c->fAgentNumber << ", fIndex = " << c->fIndex << endl;
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
    out << agent::agentsever nl;
    out << agent::agentsliving nl;

}


//---------------------------------------------------------------------------
// agent::agentload
//---------------------------------------------------------------------------    
void agent::agentload(istream&)
{
	qWarning("agent::agentload called. Not supported.");
#if 0
    in >> agent::agentsever;
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
// agent::agent
//---------------------------------------------------------------------------
agent::agent(TSimulation* sim, gstage* stage)
	:	xleft(-1),  		// to show it hasn't been initialized
		fSimulation(sim),
		fAlive(false), 		// must grow() to be truly alive
		fMass(0.0), 		// mass - not used
		fHeuristicFitness(0.0),  	// crude guess for keeping minimum population early on
		fGenome(NULL),
		fBrain(NULL),
		fBrainFuncFile(NULL)
{
	Q_CHECK_PTR(sim);
	Q_CHECK_PTR(stage);
	
	/* Set object type to be AGENTTYPE */
	setType(AGENTTYPE);
	
	if (!gClassInited)
		agentinit();

	fVelocity[0] = 0.0;
	fVelocity[1] = 0.0;
	fVelocity[2] = 0.0;
	
	fSpeed = 0.0;
	fMaxSpeed = 0.0;
	fLastEat = 0;

	fGenome = new genome();
	Q_CHECK_PTR(fGenome);
	
	fBrain = new brain(this);
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
	delete fBrain;
}


//---------------------------------------------------------------------------
// agent::dump
//---------------------------------------------------------------------------
void agent::dump(ostream& out)
{
    out << fAgentNumber nl;
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

    fGenome->Dump(out);
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
	
    in >> fAgentNumber;
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

    fGenome->Load(in);
    if (fBrain == NULL)
    {
        fBrain = new brain(this);
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
void agent::grow( bool recordBrainAnatomy, bool recordBrainFunction )
{    
	Q_CHECK_PTR(fBrain);
	Q_CHECK_PTR(fGenome);
	
	// grow the brain from the genome's specifications
	fBrain->Grow( fGenome, fAgentNumber, recordBrainAnatomy );

	// If we're recording brain function,
	// open the file to be used to write out neural activity
	if( recordBrainFunction )
		fBrainFuncFile = fBrain->startFunctional( fAgentNumber );

    // setup the agent's geometry
    SetGeometry();

	// initially set red & blue to 0
    fColor[0] = fColor[2] = 0.0;
    
	// set green color by the "id"
    fColor[1] = fGenome->ID();
    
	// start neutral gray
    fNoseColor[0] = fNoseColor[1] = fNoseColor[2] = 0.5;
    
    fAge = 0;
    fLastMate = gInitMateWait;
    
    fMaxEnergy = gMinMaxEnergy + ((fGenome->Size(gMinAgentSize, gMaxAgentSize) - gMinAgentSize)
    			 * (gMaxMaxEnergy - gMinMaxEnergy) / (gMaxAgentSize - gMinAgentSize) );
	
    fEnergy = fMaxEnergy;
	
	fFoodEnergy = fMaxEnergy;
	
//	printf( "%s: energy initialized to %g\n", __func__, fEnergy );
    
    fSpeed2Energy = gSpeed2Energy * fGenome->MaxSpeed()
				     * (fGenome->Size(gMinAgentSize, gMaxAgentSize) - gMinAgentSize) * (gMaxSizePenalty - 1.0)
					/ (gMaxAgentSize - gMinAgentSize);
    
    fYaw2Energy = gYaw2Energy * fGenome->MaxSpeed()
              	   * (fGenome->Size(gMinAgentSize, gMaxAgentSize) - gMinAgentSize) * (gMaxSizePenalty - 1.0)
              	   / (gMaxAgentSize - gMinAgentSize);
    
    fSizeAdvantage = 1.0 + ( (fGenome->Size(gMinAgentSize, gMaxAgentSize) - gMinAgentSize) *
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
	
	if (fBrain->Eat() > eatthreshold)
	{
		float trytoeat = fBrain->Eat() * eat2consume;
		
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
				
		fHeuristicFitness += eatFitnessParameter * actuallyeat / (eat2consume * fGenome->Lifespan());
		
		if( actuallyeat > 0.0 )
			fLastEat = step;
	}
	
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
	return fGenome->MateProbability(c->Genes());
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
	fHeuristicFitness += mateFitnessParam * mateWait / fGenome->Lifespan();
	
	float mymateenergy = fGenome->MateEnergy() * fEnergy;	
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
				/ (fGenome->MaxSpeed() * speed2dpos * fGenome->Lifespan());
}
    

//---------------------------------------------------------------------------
// agent::lastrewards
//---------------------------------------------------------------------------        
void agent::lastrewards(float energyFitness, float ageFitness)
{
    fHeuristicFitness += energyFitness * fEnergy / fMaxEnergy
              + ageFitness * fAge / fGenome->Lifespan();
}
    
 
//---------------------------------------------------------------------------
// agent::ProjectedHeuristicFitness
//---------------------------------------------------------------------------        
float agent::ProjectedHeuristicFitness()
{
	if( fSimulation->LifeFractionSamples() >= 50 )
		return( fHeuristicFitness * fSimulation->LifeFractionRecent() * fGenome->Lifespan() / fAge +
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
	// Decrement total number of agents
	agent::agentsliving--;	
	Q_ASSERT(agent::agentsliving >= 0);
	
	// Clear index in bitset
//	cout << "agent::Die: this = " << this << ", fAgentNumber = " << fAgentNumber << ", fIndex = " << fIndex << "----------" << endl;
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
    fLengthX = fGenome->Size(gMinAgentSize, gMaxAgentSize) / sqrt(fGenome->MaxSpeed());
    fLengthZ = fGenome->Size(gMinAgentSize, gMaxAgentSize) * sqrt(fGenome->MaxSpeed());
    srPrint( "agent::%s(): min=%g, max=%g, speed=%g, size=%g, lx=%g, lz=%g\n", __FUNCTION__, gMinAgentSize, gMaxAgentSize, fGenome->MaxSpeed(), fGenome->Size(gMinAgentSize, gMaxAgentSize), fLengthX, fLengthZ );
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
        const float fovx = fBrain->Focus() * (gMaxFocus - gMinFocus) + gMinFocus;
        		
		fFrustum.Set(fPosition[0], fPosition[2], fAngle[0], fovx, gMaxRadius);
		fCamera.SetAspect(fovx * brain::retinaheight / (gAgentFOV * brain::retinawidth));
		
		fSimulation->GetAgentPOVWindow()->DrawAgentPOV( this );
	#ifdef DEBUGCHECK
		debugcheck("agent::UpdateVision after DrawAgentPOV");
	#endif // DEBUGCHECK

		if (fBrain->retinaBuf != NULL)
		{
			// The POV window must be the current GL context,
			// when agent::UpdateVision is called, for both the
			// DrawAgentPOV() call above and the glReadPixels()
			// call below.  It is set in TSimulation::Step().
			
			glReadPixels(xleft,
				 		 ypix,
				 		 brain::retinawidth,
				 		 1,
				 		 GL_RGBA,
				 		 GL_UNSIGNED_BYTE,
				 		 fBrain->retinaBuf);
		#ifdef DEBUGCHECK
			debugcheck("agent::UpdateVision after glReadPixels");
		#endif // DEBUGCHECK
		#if 0
			printf( "retina pixels:" );
			for( int i = 0; i < brain::retinawidth; i++ )
			{
				printf( " " );
				for( int j = 0; j < 4; j++ )
					printf( "%02x", fBrain->retinaBuf[i*4 + j] );
				
			}
			printf( "\n" );
		#endif
		}
	}
}


//---------------------------------------------------------------------------
// agent::UpdateBrain
//---------------------------------------------------------------------------
void agent::UpdateBrain()
{
#if SPIKING_MODEL
    fBrain->UpdateSpikes( fEnergy / fMaxEnergy, fBrainFuncFile );
#else
    fBrain->Update( fEnergy / fMaxEnergy );
	// If we're recording brain function, do it here
	if( fBrainFuncFile )
		fBrain->writeFunctional( fBrainFuncFile );
#endif
}


//---------------------------------------------------------------------------
// agent::UpdateBody
//
// Return energy consumed
//---------------------------------------------------------------------------
const float FF = 1.01;

float agent::UpdateBody(float moveFitnessParam, float speed2dpos, int solidObjects)
{
#ifdef DEBUGCHECK
    debugcheck("agent::Update entry");
#endif // DEBUGCHECK

	float energyUsed = 0;
	
	// just do x & z dimensions in this version
    SaveLastPosition();
    
    float dpos = fBrain->Speed() * fGenome->MaxSpeed() * gSpeed2DPosition;
    if (dpos > gMaxVelocity)
        dpos = gMaxVelocity;

    float dx = -dpos * sin(yaw() * DEGTORAD);
    float dz = -dpos * cos(yaw() * DEGTORAD);
#if UseLightForOpposingYawHack
    float dyaw = (fBrain->Yaw() - fBrain->Light()) * fGenome->MaxSpeed() * gYaw2DYaw;
#else
    float dyaw = (2.0 * fBrain->Yaw() - 1.0) * fGenome->MaxSpeed() * gYaw2DYaw;
#endif
//	printf( "%4ld  %4ld  dyaw = %4.2f b->y = %4.2f, 2*b->y - 1 = %4.2f, g->maxSpeed = %4.2f, y2dy = %4.2f\n",
//			TSimulation::fAge, fAgentNumber, dyaw, fBrain->Yaw(), 2.0*fBrain->Yaw() - 1.0, fGenome->MaxSpeed(), gYaw2DYaw );

#if TestWorld
	dx = dz = dyaw = 0.0;
#endif

    addx(dx);
    addz(dz);
#if DirectYaw
	setyaw( fBrain->Yaw() * 360.0 );
#else
    addyaw(dyaw);
#endif
    
    float energyused = fBrain->Eat()   * gEat2Energy
                     + fBrain->Mate()  * gMate2Energy
                     + fBrain->Fight() * gFight2Energy
                     + fBrain->Speed() * fSpeed2Energy
                     + fabs(2.0*fBrain->Yaw() - 1.0) * fYaw2Energy
                     + fBrain->Light() * gLight2Energy
                     + fBrain->BrainEnergy()
                     + gFixedEnergyDrain;

    double denergy = energyused * fGenome->Strength();
//	printf( "%s: energy consumed = %g + ", __func__, denergy );
	double populationEnergyPenalty;
#if UniformPopulationEnergyPenalty
	populationEnergyPenalty = gPopulationPenaltyFraction * 0.5 * (gMaxMaxEnergy + gMinMaxEnergy);
#else
	populationEnergyPenalty = gPopulationPenaltyFraction * fMaxEnergy;
#endif
	denergy += populationEnergyPenalty;
	denergy *= gLowPopulationAdvantageFactor;	// if population is getting too low, reduce energy consumption
//	printf( "%g = %g\n", populationEnergyPenalty, denergy );
    fEnergy -= denergy;
    fFoodEnergy -= denergy;

	energyUsed = denergy;

	SetRed(fBrain->Fight());	// set red color according to desire to fight
	SetBlue(fBrain->Mate()); 	// set blue color according to desire to mate
	fNoseColor[0] = fNoseColor[1] = fNoseColor[2] = fBrain->Light();
    fAge++;

	// Do barrier overrun testing here...
	// apply a multiplicative Fudge Factor to keep agents from mating
	// *across* the barriers
	barrier* b = NULL;
	barrier::gXSortedBarriers.reset();
	while (barrier::gXSortedBarriers.next(b))
	{
		if( (b->xmax() > (    x() - FF * radius())) ||
			(b->xmax() > (LastX() - FF * radius())) )
        {
			// end of barrier comes after beginning of agent
			// in either its new or old position
            if( (b->xmin() > (    x() + FF * radius())) &&
            	(b->xmin() > (LastX() + FF * radius())) )
            {
                // beginning of barrier comes after end of agent,
                // in both new and old positions,
                // so there is no overlap, and we can stop searching
                // for this agent's possible barrier overlaps
                break;  // get out of the sorted barriers while loop
            }
            else // we have an overlap in x
            {
                if( ((b->zmin() < ( z() + FF * radius())) || (b->zmin() < (LastZ() + FF * radius()))) &&
                	((b->zmax() > ( z() - FF * radius())) || (b->zmax() > (LastZ() - FF * radius()))) )
                {
                    // also overlap in z, so there may be an intersection
                    float dist  = b->dist(    x(),     z());
                    float disto = b->dist(LastX(), LastZ());
                    float p;
                    
                    if (fabs(dist) < FF * radius())
                    {
                        // they actually overlap/intersect
                        if ((disto*dist) < 0.0)
                        {   // sign change, so crossed the barrier already
                            p = fabs(dist) + FF * radius();
                            if (disto < 0.0) p = -p;
                        }
                        else
                        {
                            p = FF * radius() - fabs(dist);
                            if (dist < 0.) p = -p;
                        }

                        addz( p * b->sina());
                        addx(-p * b->cosa());

                    } // actual intersection
                    else if ((disto * dist) < 0.0)
                    {
                        // the agent completely passed through the barrier
                        p = fabs(dist) + FF * radius();
                        
                        if (disto < 0.0)
                        	p = -p;
                        	                        	
                        addz( p * b->sina());
                        addx(-p * b->cosa());
                    }
                } // overlap in z
            } // beginning of barrier comes after end of agent
        } // end of barrier comes after beginning of agent
    } // while (barrier::gXSortedBarriers.next(b))
	
#if Bricks
	// If there are any bricks, then we need to test for collisions
	if( brick::GetNumBricks() > 0 )
	{
		// If the agent moves, then we want to do collision avoidance
		if( dx != 0.0 || dz != 0.0 )
		{
			AvoidCollisions( solidObjects );
		}
	}
#endif

#if 0
	TODO
	// Do a check
	barrier::gXSortedBarriers.reset();
	while (barrier::gXSortedBarriers.next(b))
	{
		float dist  = b->dist(    x(),     z());
        float disto = b->dist(LastX(), LastZ());

        if ((disto * dist) < 0.0)
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
    if (globals::edges)
    {
        if (fPosition[0] > globals::worldsize)
        {
            if (globals::wraparound)
                fPosition[0] -= globals::worldsize;
            else
                fPosition[0] = globals::worldsize;
        }
        else if (fPosition[0] < 0.0)
        {
            if (globals::wraparound)
                fPosition[0] += globals::worldsize;
            else
                fPosition[0] = 0.0;
        }
        
        if (fPosition[2] < -globals::worldsize)
        {
            if (globals::wraparound)
                fPosition[2] += globals::worldsize;
            else
                fPosition[2] = -globals::worldsize;
        }
        else if (fPosition[2] > 0.0)
        {
            if (globals::wraparound)
                fPosition[2] -= globals::worldsize;
            else
                fPosition[2] = 0.0;
        }
    }

	// Keep track of the domain in which the agent resides
    short newDomain = fSimulation->WhichDomain(fPosition[0], fPosition[2], fDomain);
    if (newDomain != fDomain)
    {
        fSimulation->SwitchDomain(newDomain, fDomain);
        fDomain = newDomain;
    }
        
#ifdef OF1
    if (fDomain == 0)
        myt0++;
#endif

    fVelocity[0] = x() - LastX();
    fVelocity[2] = z() - LastZ();

	fSpeed = sqrt( fVelocity[0]*fVelocity[0] + fVelocity[2]*fVelocity[2] );
	
	if( fSpeed > fMaxSpeed )
		fMaxSpeed = fSpeed;

	rewardmovement(moveFitnessParam, speed2dpos);
    
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
		
		// If we reach here, then the two objects appear to have had contact this time step
		
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
    cout << "Printing agent #" << fAgentNumber nl;
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
        cout << "  fGenome->Lifespan() = " << fGenome->Lifespan() nl;
        cout << "  fGenome->MutationRate() = " << fGenome->MutationRate() nl;
        cout << "  fGenome->Strength() = " << fGenome->Strength() nl;
        cout << "  fGenome->Size() = " << fGenome->Size(gMinAgentSize, gMaxAgentSize) nl;
        cout << "  fGenome->MaxSpeed() = " << fGenome->MaxSpeed() nl;
    }
    else
    {
        cout << "  genome is not yet defined" nl;
	}
	      
    if (fBrain != NULL)
    {
        cout << "  fBrain->brainenergy() = " << fBrain->BrainEnergy() nl;
        cout << "  fBrain->eat() = " << fBrain->Eat() nl;
        cout << "  fBrain->mate() = " << fBrain->Mate() nl;
        cout << "  fBrain->fight() = " << fBrain->Fight() nl;
        cout << "  fBrain->speed() = " << fBrain->Speed() nl;
        cout << "  fBrain->yaw() = " << fBrain->Yaw() nl;
        cout << "  fBrain->light() = " << fBrain->Light() nl;
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
	return fBrain->Focus() * (gMaxFocus - gMinFocus) + gMinFocus;
}


//---------------------------------------------------------------------------
// agent::NumberToName
//---------------------------------------------------------------------------
void agent::NumberToName()
{
	char tempstring[256];
	sprintf(tempstring, "agent#%ld", agentsever);
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
