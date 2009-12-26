#include "SynapseType.h"

#include <assert.h>

#include "GenomeSchema.h"

using namespace genome;
using namespace std;

// ================================================================================
// ===
// === CLASS SynapseType
// ===
// ================================================================================
SynapseType::SynapseType( GenomeSchema *_schema,
						  const char *_name,
						  int _index )
: name(_name)
, schema(_schema)
, index(_index)
{
	nt_from = _name[0] == 'I' ? INHIBITORY : EXCITATORY;
	nt_to = _name[1] == 'I' ? INHIBITORY : EXCITATORY;
}

int SynapseType::getOffset( int from, int to )
{
	assert( to >= groupCount[NGT_INPUT] );

	return (index * mutableSize) + from + ( (to - groupCount[NGT_INPUT]) * groupCount[NGT_ANY] );
}

void SynapseType::complete( int *groupCount )
{
	this->groupCount = groupCount;

	mutableSize = sizeof(char)
		* groupCount[NGT_ANY]
		* (groupCount[NGT_ANY] - groupCount[NGT_INPUT]);
}

int SynapseType::getMutableSize()
{
	return mutableSize;
}
