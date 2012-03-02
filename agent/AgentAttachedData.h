#pragma once

//===========================================================================
// AgentAttachedData
//
// Allows for data to be associated with an agent without having to declare
// additional fields in the agent class.
//===========================================================================
class AgentAttachedData
{
 public:
	typedef unsigned int SlotHandle;
	typedef void *SlotData;

	static SlotHandle createSlot();

	static void alloc( class agent *a );
	static void dispose( class agent *a );

	static void set( class agent *a, SlotHandle handle, SlotData data );
	static SlotData get( class agent *a, SlotHandle handle );

 private:
	static bool allocatedAgent;
	static unsigned int nslots;
};
