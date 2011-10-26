#include "graphics.h"

#include <assert.h>

using namespace std;

Color::Color()
{
	set( 0, 0, 0, 0 );
}

Color::Color( GLfloat _r, GLfloat _g, GLfloat _b, GLfloat _a )
{
	set( _r, _g, _b, _a );
}

Color::Color( proplib::Property &prop )
{
	set( (float)prop.get( "R"), (float)prop.get( "G"), (float)prop.get( "B") );
}

void Color::set( GLfloat _r, GLfloat _g, GLfloat _b, GLfloat _a )
{
	r = _r; g = _g; b = _b; a = _a;
}

void Color::set( GLfloat _r, GLfloat _g, GLfloat _b )
{
	set( _r, _g, _b, 1.0 );
}

std::ostream &operator<<( std::ostream &out, const Color &color )
{
	out << "(" << color.r << "," << color.g << "," << color.b << ")";

	return out;
}

std::istream &operator>>( std::istream &in, Color &color )
{
#define CHANNEL(CHAN) \
	in >> color.CHAN; \
	assert( color.CHAN >= 0.0 && color.CHAN <= 1.0 )

	CHANNEL( r );
	CHANNEL( g );
	CHANNEL( b );

	color.a = 1.0;

	return in;
}
