#include "Resources.h"

#include <string.h>

#include "error.h"
#include "gpolygon.h"
#include "misc.h"
#include "proplib.h"

using namespace std;

static const char *RPATH[] = {"./Polyworld.app/Contents/Resources/",
							  "./etc/objects/",
							  "./",
							  NULL};

//===========================================================================
// Resources
//===========================================================================

//---------------------------------------------------------------------------
// Resources::loadPolygons()
//---------------------------------------------------------------------------

bool Resources::loadPolygons( gpolyobj *poly,
							  string name )
{
	string path = find(name + ".obj");
	if( path == "" )
	{
		error(1, "Failed finding polygon object ", name.c_str());
		return false;
	}

	path.c_str() >> *poly;

	return true;
}

//---------------------------------------------------------------------------
// Resources::getInterpreterScript()
//---------------------------------------------------------------------------

string Resources::getInterpreterScript()
{
    return find("src/library/proplib/interpreter.py");
}

//---------------------------------------------------------------------------
// Resources::find()
//---------------------------------------------------------------------------

string Resources::find( string name )
{
	for( const char **path = RPATH;
		 *path;
		 path++ )
	{
		char buf[1024];

		strcpy(buf, *path);
		strcat(buf, name.c_str());

		if( exists(buf) )
		{
			return buf;
		}
	}

	return "";
}
