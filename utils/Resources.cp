#include "Resources.h"

#include <string.h>
#include <sys/stat.h>

#include "error.h"
#include "gpolygon.h"
#include "proplib.h"

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
// Resources::parseConfiguration()
//---------------------------------------------------------------------------

void Resources::parseConfiguration( proplib::Document **ret_docValues,
									proplib::Document **ret_docSchema,
									string valuesPath,
									string schemaPath )
{
	if( !exists(valuesPath) )
		error(2, "Failed locating file", valuesPath.c_str() );

	proplib::Document *docValues = proplib::Parser::parseFile( valuesPath.c_str() );

	proplib::Document *docSchema = proplib::Parser::parseFile( schemaPath.c_str() );
	proplib::Schema::apply( docSchema, docValues );

	*ret_docValues = docValues;
	*ret_docSchema = docSchema;
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

//---------------------------------------------------------------------------
// Resources::exists()
//---------------------------------------------------------------------------

bool Resources::exists( string path )
{
	struct stat _stat;
	return 0 == stat(path.c_str(), &_stat);
}
