#pragma once

// Global
#include <istream>
#include <ostream>
#include <string>

// Local
#include "NeuronModel.h"
#include "objectlist.h"


#define DesignerBrains false
#define DebugBrainGrow false
#if DebugBrainGrow
	extern bool DebugBrainGrowPrint;
#endif

#define PrintBrain false
#if PrintBrain
	#define BPRINT(x...) if(bprint) {printf(x);}
	#define IF_BPRINT(stmt) if(bprint) stmt
	#define IF_BPRINTED(stmt) if(bprint && !bprinted) {bprinted = true; stmt;}
#else
	#define BPRINT(x...)
	#define IF_BPRINT(stmt)
	#define IF_BPRINTED(stmt)
#endif

#define DebugRender 0
#if DebugRender
	#define rPrint( x... ) printf( x )
#else
	#define rPrint( x... )
#endif

// Forward declarations
class AbstractFile;
class agent;
namespace genome
{
	class Genome;
}
class NervousSystem;
class NeuronModel;
class RandomNumberGenerator;

//===========================================================================
// Brain
//===========================================================================
class Brain
{	
public:
	static void braininit();
	static void braindestruct();
	
    Brain(NervousSystem *cns);
    ~Brain();
    
    void Dump(std::ostream& out);
    void Load(std::istream& in);
    void Grow( genome::Genome* g );
	void GrowSynapses( genome::Genome *g,
					   int groupIndex_to,
					   short neuronCount_to,
					   float *remainder,
					   short neuronLocalIndex_to,
					   short neuronIndex_to,
					   short *firstneur,
					   long &synapseCount_brain,
					   genome::SynapseType *synapseType );
	void Prebirth();
    void Update( bool bprint );

    float BrainEnergy();
    short GetNumNeurons();
	long  GetNumSynapses();
    short GetNumNonInputNeurons();
	short NumNeuronGroups( bool ignoreEmpty );
        
	void dumpAnatomical( AbstractFile *file, long index, float fitness );
	
	void startFunctional( AbstractFile *file, long index );
	void endFunctional( AbstractFile* file, float fitness );
	void writeFunctional( AbstractFile* file );
        
    void Render(short patchwidth, short patchheight);
	
protected:
	friend class agent;
	
    static bool classinited;

	NervousSystem *cns;
	NeuronModel *neuralnet;
	NeuronModel::Dimensions dims;

	genome::Genome* mygenes;
    float energyuse;

	RandomNumberGenerator *rng;
    
    void InitNeuralNet( float initial_activation );
    short NearestFreeNeuron(short iin, bool* used, short num, short exclude);
	
	void GrowDesignedBrain( genome::Genome* g );
	float DesignedEfficacy( short toGroup, short fromGroup, short isyn, int synapseType );
};

//===========================================================================
// inlines
//===========================================================================
inline float Brain::BrainEnergy() { return energyuse; }
inline short Brain::GetNumNeurons() { return dims.numneurons; }
inline long Brain::GetNumSynapses() { return dims.numsynapses; }
inline short Brain::GetNumNonInputNeurons() { return dims.numNonInputNeurons; }
inline short Brain::NumNeuronGroups( bool ignoreEmpty = true ) { return ignoreEmpty ? dims.numgroupsWithNeurons : dims.numgroups; }

// Macros
