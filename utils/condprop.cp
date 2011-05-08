#include "condprop.h"

#include <math.h>

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
#define dist(x1,z1,x2,z2) (sqrt((x1-x2)*(x1-x2) + (z1-z2)*(z1-z2)))
#define segdist(c,d) (dist(c.xa,c.za,d.xa,d.za) + dist(c.xb,c.zb,d.xb,d.zb))

	return segdist( a, b );
}

}
