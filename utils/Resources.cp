#include "Resources.h"

#include <string.h>
#include <sys/stat.h>

#include "error.h"
#include "gpolygon.h"

using namespace std;

static const char *RPATH[] = {"./Polyworld.app/Contents/Resources/",
							  "./objects/",
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
// Resources::openWorldFile()
//---------------------------------------------------------------------------

bool Resources::openWorldFile( std::filebuf *fb,
							   string filename )
{
    if( fb->open( filename.c_str(), ios_base::in ) == 0 )
	{
		error( 0, "unable to open world file \"", filename.c_str(), "\"; will use default, internal worldfile" );

		string path = find( "worldfile" );

		if( (path == "") || (fb->open( path.c_str(), ios_base::in ) == 0) )
		{
			error( 0, "unable to open internal world file; will use built-in code defaults" );
			error( 2, "built-in code defaults not currently functional; exiting" );

			return false;
		}
	}

	return true;
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

		struct stat _stat;
		if( 0 == stat(buf, &_stat) )
		{
			return buf;
		}
	}

	return "";
}
