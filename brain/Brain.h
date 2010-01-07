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
	void Prebirth( long agentNumber, bool recordBrainAnatomy );
	void InheritState( Brain *parent );
    void Update( bool bprint );

    float BrainEnergy();
    short GetNumNeurons();
    short GetNumNonInputNeurons();
	short NumNeuronGroups();
        
	void dumpAnatomical( const char* directoryName, const char* suffix, long index, float fitness );
	FILE* startFunctional( long index );
	void endFunctional( FILE* file, float fitness );
	void writeFunctional( FILE* file );
        
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
inline short Brain::GetNumNonInputNeurons() { return dims.numNonInputNeurons; }
inline short Brain::NumNeuronGroups() { return dims.numgroups; }

// Macros
