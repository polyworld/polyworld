#include "condprop.h"

namespace condprop
{

//===========================================================================
// InterpolateFunction_float
//===========================================================================
float InterpolateFunction_float( float value, float endValue, long time )
{
	return value + ( (endValue - value) / time );
}

}
