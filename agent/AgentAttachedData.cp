#include "AgentAttachedData.h"

#include <assert.h>

#include "agent.h"

//===========================================================================
// AgentAttachedData
//===========================================================================

bool AgentAttachedData::allocatedAgent = false;
unsigned int AgentAttachedData::nslots = 0;

//---------------------------------------------------------------------------
// AgentAttachedData::createSlot
//---------------------------------------------------------------------------
AgentAttachedData::SlotHandle AgentAttachedData::createSlot()
{
	assert( !allocatedAgent );

	return nslots++;
}

//---------------------------------------------------------------------------
// AgentAttachedData::alloc
//---------------------------------------------------------------------------
void AgentAttachedData::alloc( agent *a )
{
	allocatedAgent = true;
	a->attachedData = new SlotData[ nslots ];
	memset( a->attachedData, 0, sizeof(SlotData) * nslots );
}

//---------------------------------------------------------------------------
// AgentAttachedData::dispose
//---------------------------------------------------------------------------
void AgentAttachedData::dispose( agent *a )
{
	delete [] a->attachedData;
	a->attachedData = NULL;
}

//---------------------------------------------------------------------------
// AgentAttachedData::set
//---------------------------------------------------------------------------
void AgentAttachedData::set( agent *a, SlotHandle handle, SlotData data )
{
	a->attachedData[ handle ] = data;
}

//---------------------------------------------------------------------------
// AgentAttachedData::get
//---------------------------------------------------------------------------
AgentAttachedData::SlotData AgentAttachedData::get( agent *a, SlotHandle handle )
{
	return a->attachedData[ handle ];
}
