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
// Resources::parseWorldFile()
//---------------------------------------------------------------------------

void Resources::parseWorldFile( proplib::Document **ret_docWorldFile,
								proplib::Document **ret_docSchema,
								const char *argWorldfilePath)
{
	string worldfilePath;
	string schemaPath;

	if( argWorldfilePath )
	{
		if( !exists( argWorldfilePath ) )
			error(2, "Failed locating file", argWorldfilePath );

		worldfilePath = argWorldfilePath;
	}
	else
	{
		if( exists( "./current.wf" ) )
			worldfilePath = "./current.wf";
		else if( exists( "./nominal.wf" ) )
			worldfilePath = "./nominal.wf";
		else
			worldfilePath = "";
	}

	proplib::Document *docWorldFile;
	if( worldfilePath == "" )
	{
		docWorldFile = new proplib::Document( "(from schema)", "" );
	}
	else
	{
		docWorldFile = proplib::Parser::parseFile( worldfilePath.c_str() );
	}

	if( exists( "./.dumpwf" ) )
	{
		docWorldFile->write( cout );
	}

	schemaPath = "./default.wfs";

	proplib::Document *docSchema = proplib::Parser::parseFile( schemaPath.c_str() );
	proplib::Schema::apply( docSchema, docWorldFile );

	*ret_docWorldFile = docWorldFile;
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
