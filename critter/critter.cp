/********************************************************************/
/* PolyWorld:  An Artificial Life Ecological Simulator              */
/* by Larry Yaeger                                                  */
/* Copyright Apple Computer 1990,1991,1992                          */
/********************************************************************/

// Self
#include "critter.h"

// System
#include <gl.h>
#include <string.h>

// stl
#include <bitset>

// qt
#include <qgl.h>

// Local
#include "barrier.h"
#include "CritterPOVWindow.h"
#include "debug.h"
#include "food.h"
#include "globals.h"
#include "graphics.h"
#include "graybin.h"
#include "misc.h"
#include "Simulation.h"


#pragma mark -

// Critter globals
bool critter::gClassInited;
long critter::crittersever;
long critter::crittersliving;
gpolyobj* critter::critterobj;
short critter::povcols;
short critter::povrows;
short critter::povwidth;
short critter::povheight;

float critter::gCritterHeight;
float critter::gMinCritterSize;
float critter::gMaxCritterSize;
float critter::gEat2Energy;
float critter::gMate2Energy;
float critter::gFight2Energy;
float critter::gMaxSizePenalty;
float critter::gSpeed2Energy;
float critter::gYaw2Energy;
float critter::gLight2Energy;
float critter::gFocus2Energy;
float critter::gFixedEnergyDrain;
bool critter::gVision;
long critter::gInitMateWait;
float critter::gSpeed2DPosition;
float critter::gMaxRadius;
float critter::gMaxVelocity;
float critter::gMinMaxEnergy;
float critter::gMaxMaxEnergy;
float critter::gYaw2DYaw;
float critter::gMinFocus;
float critter::gMaxFocus;
float critter::gCritterFOV;
float critter::gMaxSizeAdvantage;

cxsortedlist critter::gXSortedCritters;

// [TODO] figure out a better way to track critter indices
bitset<1000> gCritterIndex;

//-------------------------------------------------------------------------------------------
// critter::critterinit
//
// Static global class initializer
//-------------------------------------------------------------------------------------------
void critter::critterinit()
{
    if (critter::gClassInited)
        return;

    critter::gClassInited = true;
    critter::crittersliving = 0;
    critter::critterobj = new gpolyobj();
    "critter.obj" >> (*(critter::critterobj));
	critter::critterobj->SetName("critterobj");
	
	// If we decide we want the width W (in cells) to be a multiple of N (call it I)
	// and we want the aspect ratio of W to height H (in cells) to be at least A,
	// and we call maxCritters M, then (do the math or trust me):
	// I = floor( (sqrt(M*A) + (N-1)) / N )
	// W = I * N
	// H = floor( (M + W - 1) / W )

	int n = 10;
	int a = 3;
	int i = (int) (sqrt( (float) (TSimulation::fMaxCritters * a) ) + n - 1) / n;
	critter::povcols = i * n;   // width in cells
	critter::povrows = (TSimulation::fMaxCritters + critter::povcols - 1) / critter:: povcols;
	critter::povwidth = critter::povcols * (brain::retinawidth + kPOVCellPad);
	critter::povheight = critter::povrows * (brain::retinaheight + kPOVCellPad);
//	cout << "numCols" ses critter::povcols cms "numRows" ses critter::povrows cms "w" ses critter::povwidth cms "h" ses critter::povheight nl;
}


//-------------------------------------------------------------------------------------------
// critter::critterdestruct
//
// Static global class destructor
//-------------------------------------------------------------------------------------------
void critter::critterdestruct()
{
	delete critter::critterobj;
}


//-------------------------------------------------------------------------------------------
// critter::getfreecritter
//
// Return a newly allocated critter, but enforce the max number of allowed critters
//-------------------------------------------------------------------------------------------
critter* critter::getfreecritter(TSimulation* simulation, gstage* stage)
{
	Q_CHECK_PTR(simulation);
	Q_CHECK_PTR(stage);
	
	if (critter::crittersliving + 1 > simulation->GetMaxCritters())
	{
		printf("PolyWorld ERROR:: Unable to satisfy request for free critter. Max allocated: %ld", simulation->GetMaxCritters());
		return NULL;
	}

	// Create the new critter
	critter* c = new critter(simulation, stage);	
	Q_CHECK_PTR(c);
	
    // Increase current total of creatures alive
    critter::crittersliving++;
    
    // Set number to total creatures that have ever lived
    c->fCritterNumber = ++critter::crittersever;

	// Set critter index.  Used for POV drawing.
	for (size_t index = 0; index < gCritterIndex.size(); ++index)
	{
		if (!gCritterIndex.test(index))
		{
			c->fIndex = index;
			gCritterIndex.set(index);
//			cout << "getfreecritter: c = " << c << ", fCritterNumber = " << c->fCritterNumber << ", fIndex = " << c->fIndex << endl;
			break;
		}
	}
		
    return c;
}


//---------------------------------------------------------------------------
// critter::critterdump
//---------------------------------------------------------------------------    
void critter::critterdump(ostream& out)
{
    out << critter::crittersever nl;
    out << critter::crittersliving nl;

}


//---------------------------------------------------------------------------
// critter::critterload
//---------------------------------------------------------------------------    
void critter::critterload(istream&)
{
	qWarning("critter::critterload called. Not supported.");
#if 0
    in >> critter::crittersever;
    in >> critter::crittersliving;

    critter::critterlist->load(in);
	
	long i;
		
    for (i = 0; i < gMaxNumCritters; i++)
        if (!critter::pc[i])
            break;
    if (i)
        error(2,"critter::pc[] array not empty during critterload");
        
    if (critter::gXSortedCritters.count())
        error(2,"gXSortedCritters list not empty during critterload");
        
    long numcrittersallocated = 0;
    in >> numcrittersallocated;
    for (i = 0; i < numcrittersallocated; i++)
    {
        critter::pc[i] = new critter;
        if (critter::pc[i] == NULL)
            error(2,"Insufficient memory for new critter in critterload");
        if (critter::critterlist->isone(i))
        {
            (critter::pc[i])->load(in);
            critter::gXSortedCritters.add(critter::pc[i]);
            globals::worldstage.addobject(critter::pc[i]);
            if ((critter::pc[i])->fIndex != i)
            {
                char msg[256];
                sprintf(msg,
                    	"pc[i]->fIndex (%ld) does not match actual index (%ld)",
                    	(critter::pc[i])->fIndex,i);
                error(2,msg);
            }
        }
        else
        {
            (critter::pc[i])->fIndex = i;
        }
    }
#endif
}


#pragma mark -


//---------------------------------------------------------------------------
// critter::critter
//---------------------------------------------------------------------------
critter::critter(TSimulation* sim, gstage* stage)
	:	fSimulation(sim),
		fAlive(false), 		// must grow() to be truly alive
		fMass(0.0), 		// mass - not used
		fFitness(0.0),  	// crude guess for keeping minimum population early on
		fGenome(NULL),
		xleft(-1),  		// to show it hasn't been initialized
		fBrain(NULL)
{
	Q_CHECK_PTR(sim);
	Q_CHECK_PTR(stage);
	
	if (!gClassInited)
		critterinit();

	fVelocity[0] = 0.0;
	fVelocity[1] = 0.0;
	fVelocity[2] = 0.0;
			
	fGenome = new genome();
	Q_CHECK_PTR(fGenome);
	
	fBrain = new brain();
	Q_CHECK_PTR(fBrain);
	
	// Set up critter POV	
	fScene.SetStage(stage);	
	fScene.SetCamera(&fCamera);
}


//---------------------------------------------------------------------------
// critter::~critter
//---------------------------------------------------------------------------
critter::~critter()
{
	delete fGenome;
	delete fBrain;
}


//---------------------------------------------------------------------------
// critter::dump
//---------------------------------------------------------------------------
void critter::dump(ostream& out)
{
    out << fCritterNumber nl;
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
    out << fFitness nl;

    gobject::dump(out);

    fGenome->Dump(out);
    if (fBrain != NULL)
        fBrain->Dump(out);
    else
        error(1, "Attempted to dump a critter with no brain");
}


//---------------------------------------------------------------------------
// critter::load
//---------------------------------------------------------------------------    
void critter::load(istream& in)
{
	qWarning("fix domain issue");
	
    in >> fCritterNumber;
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
    in >> fFitness;

    gobject::load(in);

    fGenome->Load(in);
    if (fBrain == NULL)
    {
        fBrain = new brain();
        Q_CHECK_PTR(fBrain);
    }
    fBrain->Load(in);

    // done loading in raw information, now setup some derived quantities
    SetGeometry();
    SetGraphics();
    fDomain = fSimulation->WhichDomain(fPosition[0], fPosition[2], 0);
}


//---------------------------------------------------------------------------
// critter::grow
//---------------------------------------------------------------------------
void critter::grow()
{    
	Q_CHECK_PTR(fBrain);
	Q_CHECK_PTR(fGenome);
	
	// grow the brain from the genome's specifications
    fBrain->Grow(fGenome); 

    // setup the critter's geometry
    SetGeometry();

	// initially set red & blue to 0
    fColor[0] = fColor[2] = 0.0;
    
	// set green color by the "id"
    fColor[1] = fGenome->ID();
    
	// start neutral gray
    fNoseColor[0] = fNoseColor[1] = fNoseColor[2] = 0.5;
    
    fAge = 0;
    fLastMate = -gInitMateWait;
    
    fMaxEnergy = gMinMaxEnergy + ((fGenome->Size(gMinCritterSize, gMaxCritterSize) - gMinCritterSize)
    			 * (gMaxMaxEnergy - gMinMaxEnergy) / (gMaxCritterSize - gMinCritterSize) );
    
    fEnergy = fMaxEnergy;
	
	fFoodEnergy = fMaxEnergy;
    
    fSpeed2Energy = gSpeed2Energy * fGenome->MaxSpeed()
				     * (fGenome->Size(gMinCritterSize, gMaxCritterSize) - gMinCritterSize) * (gMaxSizePenalty - 1.0)
					/ (gMaxCritterSize - gMinCritterSize);
    
    fYaw2Energy = gYaw2Energy * fGenome->MaxSpeed()
              	   * (fGenome->Size(gMinCritterSize, gMaxCritterSize) - gMinCritterSize) * (gMaxSizePenalty - 1.0)
              	   / (gMaxCritterSize - gMinCritterSize);
    
    fSizeAdvantage = 1.0 + ( (fGenome->Size(gMinCritterSize, gMaxCritterSize) - gMinCritterSize) *
                (gMaxSizeAdvantage - 1.0) / (gMaxCritterSize - gMinCritterSize) );

    // now setup the camera & window for our critter to see the world in
    SetGraphics();

    fAlive = true;
}


//---------------------------------------------------------------------------
// critter::setradius
//---------------------------------------------------------------------------
void critter::setradius()
{
	// only set radius anew if not set manually
	if( !fRadiusFixed ) 
		fRadius = sqrt( fLength[0]*fLength[0] + fLength[2]*fLength[2] ) * fRadiusScale * fScale * 0.5;
	srPrint( "critter::%s(): r=%g%s lx=%g lz=%g rs=%g s=%g\n", __FUNCTION__, fRadius, fRadiusFixed ? " (fixed)" : "", fLength[0], fLength[2], fRadiusScale, fScale );
}


//---------------------------------------------------------------------------
// critter::eat
//
//	Return the amount of food energy actually lost to the world (if any)
//---------------------------------------------------------------------------
float critter::eat(food* f, float eatFitnessParameter, float eat2consume, float eatthreshold)
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
				
		fFitness += eatFitnessParameter * actuallyeat / (eat2consume * fGenome->Lifespan());
	}
	
	return result;
}


//---------------------------------------------------------------------------
// critter::damage
//---------------------------------------------------------------------------    
void critter::damage(float e)
{
	fEnergy -= (e<fEnergy) ? e : fEnergy;
}


//---------------------------------------------------------------------------
// critter::MateProbability
//---------------------------------------------------------------------------    
float critter::MateProbability(critter* c)
{
	return fGenome->MateProbability(c->Genes());
}


//---------------------------------------------------------------------------
// critter::mating
//---------------------------------------------------------------------------    
float critter::mating(float fitness, long wait)
{
	fLastMate = fAge;
	fFitness += fitness * wait / fGenome->Lifespan();
	
	float mymateenergy = fGenome->MateEnergy() * fEnergy;	
	fEnergy -= mymateenergy;
	fFoodEnergy -= mymateenergy;
	
	return mymateenergy;
}
    

//---------------------------------------------------------------------------
// critter::rewardmovement
//---------------------------------------------------------------------------        
void critter::rewardmovement(float moveFitnessParam, float speed2dpos)
{
	fFitness += moveFitnessParam
				* (fabs(fPosition[0] - fLastPosition[0]) + fabs(fPosition[2] - fLastPosition[2]))
				/ (fGenome->MaxSpeed() * speed2dpos * fGenome->Lifespan());
}
    

//---------------------------------------------------------------------------
// critter::lastrewards
//---------------------------------------------------------------------------        
void critter::lastrewards(float energyFitness, float ageFitness)
{
    fFitness += energyFitness * fEnergy / fMaxEnergy
              + ageFitness * fAge / fGenome->Lifespan();
}
    
 
//---------------------------------------------------------------------------
// critter::Die
//---------------------------------------------------------------------------    
void critter::Die()
{
	// Decrement total number of critters
	critter::crittersliving--;	
	Q_ASSERT(critter::crittersliving >= 0);
	
	// Clear index in bitset
//	cout << "critter::Die: this = " << this << ", fCritterNumber = " << fCritterNumber << ", fIndex = " << fIndex << "----------" << endl;
	gCritterIndex.set(fIndex, false);
	
	// Used to clear this critter's pane in the POV window/region, and call endbrainmonitoring()
	
	// No longer alive. :(		
	fAlive = false;
}



//---------------------------------------------------------------------------
// critter::SetGeometry
//---------------------------------------------------------------------------    
void critter::SetGeometry()
{
	// obtain a fresh copy of the basic critter geometry
    clonegeom(*critterobj);
        
    // then adjust the geometry to fit size, speed, & critterheight
    fLengthX = fGenome->Size(gMinCritterSize, gMaxCritterSize) / sqrt(fGenome->MaxSpeed());
    fLengthZ = fGenome->Size(gMinCritterSize, gMaxCritterSize) * sqrt(fGenome->MaxSpeed());
    srPrint( "critter::%s(): min=%g, max=%g, speed=%g, size=%g, lx=%g, lz=%g\n", __FUNCTION__, gMinCritterSize, gMaxCritterSize, fGenome->MaxSpeed(), fGenome->Size(gMinCritterSize, gMaxCritterSize), fLengthX, fLengthZ );
    for (long i = 0; i < fNumPolygons; i++)
    {
        for (long j = 0; j < fPolygon[i].fNumPoints; j++)
        {
            fPolygon[i].fVertices[j * 3  ] *= fLengthX;
            fPolygon[i].fVertices[j * 3 + 1] *= gCritterHeight;
            fPolygon[i].fVertices[j * 3 + 2] *= fLengthZ;
        }
    }
    setlen();
}


//---------------------------------------------------------------------------
// critter::SetGraphics
//---------------------------------------------------------------------------    
void critter::SetGraphics()
{
    // setup the camera & window for our critter to see the world in
	float fovx = FieldOfView();
	fCamera.SetAspect(fovx * brain::retinaheight / (gCritterFOV * brain::retinawidth));
    fCamera.settranslation(0.0, 0.0, -0.5 * fLengthZ);
    
    if (xleft < 0)  // not initialized yet
    {
        short irow = short(fIndex / povcols);            
        short icol = short(fIndex) - (povcols * irow);

        xleft = icol * (brain::retinawidth + kPOVCellPad)  +  kPOVCellPad;
        xright = xleft + brain::retinawidth - 1;
		ytop = critter::povheight  -  (irow) * (brain::retinaheight + kPOVCellPad)  -  kPOVCellPad  -  1;
		ybottom = ytop  -  brain::retinaheight  +  1;
		ypix = ybottom  +  brain::retinaheight / 2  +  1;

//		cout << "****povheight" ses povheight cms "retinaheight" ses brain::retinaheight cms "povrows" ses povrows cms "irow" ses irow nl;
//		cout << "    povwidth " ses povwidth  cms "retinawidth " ses brain::retinawidth  cms "povcols" ses povcols cms "icol" ses icol nl;
//		cout << "    index" ses fIndex cms "xleft" ses xleft cms "xright" ses xright cms "ytop" ses ytop cms "ybottom" ses ybottom cms "ypix" ses ypix nl;

        fCamera.SetNear(.01);
        fCamera.SetFar(1.5 * globals::worldsize);
        fCamera.SetFOV(gCritterFOV);
        fCamera.AttachTo(this);
    }
}


//---------------------------------------------------------------------------
// critter::SaveLastPosition
//---------------------------------------------------------------------------    
void critter::SaveLastPosition()
{
	fLastPosition[0] = fPosition[0];
	fLastPosition[1] = fPosition[1];
	fLastPosition[2] = fPosition[2];
}


//---------------------------------------------------------------------------
// critter::Behave
//---------------------------------------------------------------------------    
void critter::Behave()
{
#ifdef DEBUGCHECK
    debugcheck("critter::Behave entry");
#endif DEBUGCHECK

    if (gVision)
    {
		// create retinal pixmap, based on values of focus & numvisneurons
        const float fovx = fBrain->Focus() * (gMaxFocus - gMinFocus) + gMinFocus;
        		
		fFrustum.Set(fPosition[0], fPosition[2], fAngle[0], fovx, gMaxRadius);
		fCamera.SetAspect(fovx * brain::retinaheight / (gCritterFOV * brain::retinawidth));
		
		fSimulation->GetCritterPOVWindow()->DrawCritterPOV( this );
	#ifdef DEBUGCHECK
		debugcheck("critter::Behave after DrawCritterPOV");
	#endif DEBUGCHECK

		if (brain::gRetinaBuf != NULL)
		{
			// The POV window must be the current GL context,
			// when critter::Behave is called, for both the
			// DrawCritterPOV() call above and the glReadPixels()
			// call below.  It is set in TSimulation::Step().
			
			glReadPixels(xleft,
				 		 ypix,
				 		 brain::retinawidth,
				 		 1,
				 		 GL_RGBA,
				 		 GL_BYTE,
				 		 brain::gRetinaBuf);
		#ifdef DEBUGCHECK
			debugcheck("critter::Behave after glReadPixels");
		#endif DEBUGCHECK
		#if 0
			printf( "retina pixels:" );
			for( int i = 0; i < brain::retinawidth; i++ )
			{
				printf( " " );
				for( int j = 0; j < 4; j++ )
					printf( "%02x", brain::gRetinaBuf[i*4 + j] );
				
			}
			printf( "\n" );
		#endif
		}
	}
	        
    // now update the brain
    fBrain->Update(fEnergy / fMaxEnergy);
}


//---------------------------------------------------------------------------
// critter::Update
//
// Return energy consumed
//---------------------------------------------------------------------------
const float FF = 1.01;

float critter::Update(float moveFitnessParam, float speed2dpos)
{
#ifdef DEBUGCHECK
    debugcheck("critter::Update entry");
#endif DEBUGCHECK

	float energyUsed = 0;
	
	// Update vision and brain
	Behave(); 

	// just do x & z dimensions in this version
    SaveLastPosition();
    
    float dpos = fBrain->Speed() * fGenome->MaxSpeed() * gSpeed2DPosition;
    if (dpos > gMaxVelocity)
        dpos = gMaxVelocity;

    float dx = -dpos * sin(yaw() * DEGTORAD);
    float dz = -dpos * cos(yaw() * DEGTORAD);
    float dyaw = (2.0 * fBrain->Yaw() - 1.0) * fGenome->MaxSpeed() * gYaw2DYaw;

    addx(dx);
    addz(dz);
    addyaw(dyaw);
    
    float energyused = fBrain->Eat()   * gEat2Energy
                     + fBrain->Mate()  * gMate2Energy
                     + fBrain->Fight() * gFight2Energy
                     + fBrain->Speed() * fSpeed2Energy
                     + fBrain->Yaw()   * fYaw2Energy
                     + fBrain->Light() * gLight2Energy
                     + fBrain->BrainEnergy()
                     + gFixedEnergyDrain;

    float denergy = energyused * fGenome->Strength();
    fEnergy -= denergy;
    fFoodEnergy -= denergy;

	energyUsed = denergy;

	SetRed(fBrain->Fight());	// set red color according to desire to fight
	SetBlue(fBrain->Mate()); 	// set blue color according to desire to mate
	fNoseColor[0] = fNoseColor[1] = fNoseColor[2] = fBrain->Light();
    fAge++;

	// Hardwire knowledge of world boundaries for now
	// Need to do something special with the critter list,
	// when the critters do a wraparound (for the sake of efficiency
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

	// Keep track of the domain in which the critter resides
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

	// Also do boundary overrun testing here...
	// apply a multiplicative Fudge Factor to keep critters from mating
	// *across* the barriers
	barrier* b = NULL;
	barrier::gXSortedBarriers.reset();
	while (barrier::gXSortedBarriers.next(b))
	{
		if( (b->xmax() > (    x() - FF * radius())) ||
			(b->xmax() > (LastX() - FF * radius())) )
        {
			// end of barrier comes after beginning of critter
			// in either its new or old position
            if( (b->xmin() > (    x() + FF * radius())) &&
            	(b->xmin() > (LastX() + FF * radius())) )
            {
                // beginning of barrier comes after end of critter,
                // in both new and old positions,
                // so there is no overlap, and we can stop searching
                // for this critter's possible barrier overlaps
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
                        // the critter completely passed through the barrier
                        p = fabs(dist) + FF * radius();
                        
                        if (disto < 0.0)
                        	p = -p;
                        	                        	
                        addz( p * b->sina());
                        addx(-p * b->cosa());
                    }
                } // overlap in z
            } // beginning of barrier comes after end of critter
        } // end of barrier comes after beginning of critter
    } // while (barrier::gXSortedBarriers.next(b))
	
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

    fVelocity[0] = x() - LastX();
    fVelocity[2] = z() - LastZ();

	rewardmovement(moveFitnessParam, speed2dpos);
    
    return energyUsed;
}


void critter::draw()
{
	glPushMatrix();
		position();
		glScalef(fScale, fScale, fScale);
		gpolyobj::drawcolpolyrange(0, 4, fNoseColor);
		gpolyobj::drawcolpolyrange(5, 9, fColor);
	glPopMatrix();
}


void critter::print()
{
    cout << "Printing critter #" << fCritterNumber nl;
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
    cout << "  fFitness = " << fFitness nl;
    
    if (fGenome != NULL)
    {
        cout << "  fGenome->Lifespan() = " << fGenome->Lifespan() nl;
        cout << "  fGenome->MutationRate() = " << fGenome->MutationRate() nl;
        cout << "  fGenome->Strength() = " << fGenome->Strength() nl;
        cout << "  fGenome->Size() = " << fGenome->Size(gMinCritterSize, gMaxCritterSize) nl;
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
// critter::FieldOfView
//---------------------------------------------------------------------------
float critter::FieldOfView()
{
	return fBrain->Focus() * (gMaxFocus - gMinFocus) + gMinFocus;
}


//---------------------------------------------------------------------------
// critter::NumberToName
//---------------------------------------------------------------------------
void critter::NumberToName()
{
	char tempstring[256];
	sprintf(tempstring, "critter#%ld", crittersever);
	this->SetName(tempstring);
}


//---------------------------------------------------------------------------
// cxsortedlist::add
//---------------------------------------------------------------------------
void cxsortedlist::add(critter* a)
{
#ifdef DEBUGCALLS
    pushproc("cxsortedlist::add");
#endif DEBUGCALLS
    bool inserted = false;
    critter* o;
    this->reset();
    while (this->next(o))
    {
        if ((a->x()-a->radius()) < (o->x()-o->radius()))
        {
            this->inserthere(a);
            inserted = true;
            break;
        }
    }
    if (!inserted)
    {
        this->append(a);
    }
#ifdef DEBUGCALLS
    popproc();
#endif DEBUGCALLS
}


//---------------------------------------------------------------------------
// cxsortedlist::sort
//---------------------------------------------------------------------------
void cxsortedlist::sort()
{
#ifdef DEBUGCALLS
    pushproc("cxsortedlist::sort");
#endif DEBUGCALLS
// This technique assumes that the list is almost entirely sorted at the start
// Hopefully, with some reasonable frame-to-frame coherency, this will be true!
    gdlink<critter*> *savecurr;
    critter* o = NULL;
    critter* p = NULL;
    critter* b = NULL;
    this->reset();
    this->next(p);
    savecurr = currItem;
    while (this->next(o))
    {
        if ((o->x() - o->radius()) < (p->x() - p->radius()))
        {
            gdlink<critter*>* link = this->unlink();  // at o, just unlink
            currItem = savecurr;  // back up to previous one directly
            while (this->prev(b)) // then use iterator to move back again
                if ((b->x() - b->radius()) < (o->x() - o->radius()))
                    break; // until we have one that starts before o
            if (currItem)  // we did find one, and didn't run off beginning of list
                this->appendhere(link);
            else  // we have a new head of the list
                this->insert(link);
            currItem = savecurr;
            o = p;
        }
        p = o;
        savecurr = currItem;
    }
#ifdef DEBUGCALLS
    popproc();
#endif DEBUGCALLS
}


//---------------------------------------------------------------------------
// cxsortedlist::list
//---------------------------------------------------------------------------
void cxsortedlist::list()
{
#ifdef DEBUGCALLS
    pushproc("cxsortedlist::list");
#endif DEBUGCALLS
    gdlink<critter*> *savecurr;
    critter* pcrit;
    savecurr = currItem;
    this->reset();
    while(this->next(pcrit))
        cout sp pcrit->Number();
    cout nlf;
    currItem = savecurr;
#ifdef DEBUGCALLS
    popproc();
#endif DEBUGCALLS
}
