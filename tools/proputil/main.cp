#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <list>
#include <string>
#include <utility>

#include "misc.h"
#include "proplib.h"

using namespace proplib;
using namespace std;

void usage( string msg = "" )
{
	cerr << "usage: proputil apply path_schema path_values" << endl;
	cerr << "       proputil reduce path_schema path_values" << endl;
	cerr << "       proputil get path_values propname" << endl;
	cerr << "       proputil set path_values propname=propvalue..." << endl;
	cerr << "       proputil len path_values propname" << endl;

	if( msg.length() > 0 )
	{
		cerr << "--------------------------------------------------------------------------------" << endl;
		cerr << msg << endl;
	}

	exit( 1 );
}

class NameValuePair
{
 public:
	string name;
	string value;

	static NameValuePair parse( const char *encoding )
	{
		const char *equals = strchr( encoding, '=' );
		if( equals == NULL )
		{
			usage( string("Invalid name=value arg (") + encoding + ")" );
		}

		NameValuePair pair;
		pair.name = string( encoding, equals - encoding );
		pair.value = string( equals + 1 );

		return pair;
	}
};
typedef list<NameValuePair> NameValuePairList;

void apply( const char *pathSchema, const char *pathValues );
void reduce( const char *pathSchema, const char *pathValues );
void get( const char *pathValues, const char *name );
void set( const char *pathValues, NameValuePairList &pairs );
void len( const char *pathValues, const char *name );

int main( int argc, char **argv )
{
	if( argc < 2 )
	{
		usage( "Must specify mode" );
	}

	string mode = argv[1];

	if( mode == "apply" )
	{
		if( argc != 4 )
		{
			usage();
		}

		apply( argv[2], argv[3] );
	}
	else if( mode == "reduce" )
	{
		if( argc != 4 )
		{
			usage();
		}

		reduce( argv[2], argv[3] );
	}
	else if( mode == "get" )
	{
		if( argc != 4 )
		{
			usage();
		}

		get( argv[2], argv[3] );
	}
	else if( mode == "set" )
	{
		if( argc < 4 )
		{
			usage();
		}

		NameValuePairList pairs;

		for( int i = 3; i < argc; i++ )
		{
			pairs.push_back( NameValuePair::parse(argv[i]) );
		}

		set( argv[2], pairs );
	}
	else if( mode == "len" )
	{
		if( argc != 4 )
		{
			usage();
		}

		len( argv[2], argv[3] );
	}
	else
	{
		usage( "Invalid mode" );
	}

	return 0;
}

void apply( const char *pathSchema, const char *pathValues )
{
	Document *docSchema = Parser::parseFile( pathSchema );
	Document *docValues = Parser::parseFile( pathValues );

	Schema::apply( docSchema, docValues );

	docValues->write( cout );

	delete docSchema;
	delete docValues;
}

void reduce( const char *pathSchema, const char *pathValues )
{
	Document *docSchema = Parser::parseFile( pathSchema );
	Document *docValues = Parser::parseFile( pathValues );

	Schema::reduce( docSchema, docValues );

	docValues->write( cout );

	delete docSchema;
	delete docValues;
}

void get( const char *pathValues, const char *name )
{
	Document *docValues = Parser::parseFile( pathValues );

	cout << docValues->get( name ).evalScalar() << endl;

	delete docValues;
}

void set( const char *pathValues, NameValuePairList &pairs )
{
	Document *docValues = Parser::parseFile( pathValues );

	itfor( NameValuePairList, pairs, it )
	{
		StringList path = Parser::parsePropertyPath( it->name );
		
		docValues->set( path, it->value );
	}

	docValues->write( cout );

	delete docValues;
}

void len( const char *pathValues, const char *name )
{
	Document *docValues = Parser::parseFile( pathValues );
	Property *prop = docValues->getp( name );
	if( !prop )
	{
		cerr << "Unknown property '" << name << "'" << endl;
		exit( 1 );
	}
	if( prop->isContainer() )
	{
		cout << prop->props().size() << endl;
	}
	else
	{
		cerr << "Cannot get length of scalar." << endl;
		exit( 1 );
	}

	delete docValues;
}
