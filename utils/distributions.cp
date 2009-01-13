
/*
  Get random, non-uniformly distributed random numbers.

  getNormal(sigma, mu): to get a random float from a normal distribution
  with std. deviation of sigma and mean of mu.

  getLinear(slope, yIntercept): to get a random float from a linear distribution
  with given slope and yIntercept

*/

// Self
#include "distributions.h"

// Math
#include "math.h"

// System
#include <ostream>
#include <stdlib.h>

// qt
#include <qapplication.h>

// Polyworld
#include "misc.h"

using namespace std;


//
// Probability distribution functions
//
float normalPDF( float x, float sigma, float mu )
{
    float left, rightTop, rightBottom, right;
    float pi = 3.1415927;
    float e = 2.7182818;
	
    left = 1.0 / sqrt( 2 * pi * pow( sigma, 2 ) );
    rightTop = - pow( (x - mu), 2 );
    rightBottom = 2 * pow( sigma, 2 );
    right = pow( e, (rightTop / rightBottom) );
    return( left * right );
}

float linearPDF( float x, float slope, float yIntercept )
{
	if( x <= 0.5 )
		return( -slope * x );
	else
		return( slope * x  +  yIntercept );
}


// Get distributions by generating uniform random numbers and then filtering them based on PDFs
// Slowish, but simple
float getLinear( float slope, float yIntercept )
{
    float x, y, z;
	
    x = randpw();
    y = linearPDF( x, slope, yIntercept );
    z = randpw();
    if( (z < y) && (z >= 0.0) && (z <= 1.0) )
		return( x );
	else
		return( getLinear( slope, yIntercept ) );
}

float getNormal( float sigma, float mu )
{
    float x, y, z;
	
    x = randpw();
    y = normalPDF( x, sigma, mu );
    z = randpw();
    if( (z < y) && (z >= 0.0) && (z <= 1.0) )
		return( x );
    else
		return( getNormal( sigma, mu ) );
}
