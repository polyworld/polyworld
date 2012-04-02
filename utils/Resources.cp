#include "Resources.h"

#include <string.h>

#include "error.h"
#include "gpolygon.h"
#include "misc.h"
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

void Resources::parseConfiguration( string schemaPath,
									string docPath,
									bool isWorldfile,
									proplib::SchemaDocument **ret_schema,
									proplib::Document **ret_doc )
{
	if( !exists(schemaPath) )
		error(2, "Failed locating file", schemaPath.c_str() );
	if( !exists(docPath) )
		error(2, "Failed locating file", docPath.c_str() );

	proplib::DocumentBuilder builder;
	proplib::SchemaDocument *schema = builder.buildSchemaDocument( schemaPath );
	proplib::Document *doc;
	if( isWorldfile )
		doc = builder.buildWorldfileDocument( schema, docPath );
	else
		doc = builder.buildDocument( docPath );
	schema->apply( doc );

	*ret_schema = schema;
	*ret_doc = doc;
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
