#pragma once

#include "Brain.h"
#include "NeuronModel.h"
#include "GroupsSynapseType.h"

namespace genome { class GroupsGenome; }

class GroupsBrain : public Brain
{
 public:
	static struct Configuration
	{
		short maxneurons;
		long maxsynapses;

		short numinputneurgroups;
		short numoutneurgroups;
		short minvisneurpergroup;
		short maxvisneurpergroup;
		short mininternalneurgroups;
		short maxinternalneurgroups;
		short mineneurpergroup;
		short maxeneurpergroup;
		short minineurpergroup;
		short maxineurpergroup;
		short maxneurpergroup;
		short maxneurgroups;
		short maxnoninputneurgroups;
		short maxinternalneurons;
		short maxinputneurons;
		short maxnoninputneurons;
		float minconnectiondensity;
		float maxconnectiondensity;
		float mintopologicaldistortion;
		float maxtopologicaldistortion;

		bool enableTopologicalDistortionRngSeed;
		long minTopologicalDistortionRngSeed;
		long maxTopologicalDistortionRngSeed;

		bool enableInitWeightRngSeed;
		long minInitWeightRngSeed;
		long maxInitWeightRngSeed;
	} config;

	static void processWorldfile( proplib::Document &doc );
	static void init();

 public:
	GroupsBrain( NervousSystem *cns, genome::GroupsGenome *g );
	virtual ~GroupsBrain();

	short NumNeuronGroups( bool ignoreEmpty = true );

 private:
	void initNeuralNet( float initial_activation );
	short nearestFreeNeuron(short iin, bool* used, short num, short exclude);

	void grow();
	void growSynapses( int groupIndex_to,
					   short neuronCount_to,
					   float *remainder,
					   short neuronLocalIndex_to,
					   short neuronIndex_to,
					   short *firstneur,
					   long &synapseCount_brain,
					   genome::GroupsSynapseType *synapseType );

	genome::GroupsGenome *_genome;
	int _numgroups;
	int _numgroupsWithNeurons;
};

inline short GroupsBrain::NumNeuronGroups( bool ignoreEmpty ) { return ignoreEmpty ? _numgroupsWithNeurons : _numgroups; }
