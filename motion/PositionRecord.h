#ifndef POSITION_RECORD_H
#define POSITION_RECORD_H


//===========================================================================
// PositionEpochHeader
//===========================================================================
#pragma pack(push, 1)
struct PositionEpochHeader
{
#define EPOCH_HEADER_VERSION 1
	int version;

	int sizeof_header;

	long step_begin;
	long step_end;
};
#pragma pack(pop)


//===========================================================================
// PositionHeader
//===========================================================================
#pragma pack(push, 1)
struct PositionHeader
{
#define POSITION_HEADER_VERSION 1
	int version;

	int sizeof_header;
	int sizeof_record;
};
#pragma pack(pop)


//===========================================================================
// PositionRecord
//===========================================================================
#pragma pack(push, 1)
struct PositionRecord
{
	long step;
	float x;
	float y;
	float z;
};
#pragma pack(pop)


#endif // POSITION_RECORD_H
