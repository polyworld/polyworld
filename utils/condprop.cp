#include "condprop.h"

#include <math.h>

#include "globals.h"
#include "misc.h"

namespace condprop
{

//===========================================================================
// InterpolateFunction_float
//===========================================================================
float InterpolateFunction_float( float value, float endValue, float ratio )
{
	return value + ( (endValue - value) * ratio );
}

//===========================================================================
// InterpolateFunction_long
//===========================================================================
long InterpolateFunction_long( long value, long endValue, float ratio )
{
	return long( value + ( (endValue - value) * ratio ) );
}

//===========================================================================
// InterpolateFunction_LineSegment
//===========================================================================
 LineSegment InterpolateFunction_LineSegment( LineSegment value, LineSegment endValue, float ratio )
 {
	 return LineSegment( InterpolateFunction_float(value.xa, endValue.xa, ratio),
						 InterpolateFunction_float(value.za, endValue.za, ratio),
						 InterpolateFunction_float(value.xb, endValue.xb, ratio),
						 InterpolateFunction_float(value.zb, endValue.zb, ratio) );
 }

//===========================================================================
// InterpolateFunction_EnergyMultiplier
//===========================================================================
EnergyMultiplier InterpolateFunction_EnergyMultiplier( EnergyMultiplier value, EnergyMultiplier endValue, float ratio )
{
	float result[globals::numEnergyTypes];

	for( int i = 0; i < globals::numEnergyTypes; i++ )
		result[i] = InterpolateFunction_float( value[i], endValue[i], ratio );

	return EnergyMultiplier( result );
}

//===========================================================================
// DistanceFunction_float
//===========================================================================
float DistanceFunction_float( float a, float b )
{
	return fabs( a - b );
}

//===========================================================================
// DistanceFunction_long
//===========================================================================
float DistanceFunction_long( long a, long b )
{
	return abs( a - b );
}

//===========================================================================
// DistanceFunction_LineSegment
//===========================================================================
float DistanceFunction_LineSegment( LineSegment a, LineSegment b )
{
	return dist(a.xa,a.za,b.xa,b.za) + dist(a.xb,a.zb,b.xb,b.zb);
}

//===========================================================================
// DistanceFunction_EnergyMultiplier
//===========================================================================
float DistanceFunction_EnergyMultiplier( EnergyMultiplier a, EnergyMultiplier b )
{
	float result = 0;

	for( int i = 0; i < globals::numEnergyTypes; i++ )
		result += fabs( a[i] - b[i] );

	return result;
}

}
