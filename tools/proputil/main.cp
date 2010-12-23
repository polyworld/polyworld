#include <stdlib.h>

#include <iostream>
#include <string>

#include "proplib.h"

using namespace proplib;
using namespace std;

void apply( const char *pathSchema, const char *pathValues );
void set( const char *pathValues, const char *name, const char *value );
void reduce( const char *pathSchema, const char *pathValues );

void usage( string msg = "" )
{
	cout << "usage: proputil apply path_schema path_values" << endl;
	cout << "       proputil set path_values propname propvalue" << endl;
	cout << "       proputil reduce path_schema path_values" << endl;

	if( msg.length() > 0 )
	{
		cout << "--------------------------------------------------------------------------------" << endl;
		cout << msg << endl;
	}

	exit( 1 );
}

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
	else if( mode == "set" )
	{
		if( argc != 5 )
		{
			usage();
		}

		set( argv[2], argv[3], argv[4] );
	}
	else if( mode == "reduce" )
	{
		if( argc != 4 )
		{
			usage();
		}

		reduce( argv[2], argv[3] );
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

void set( const char *pathValues, const char *name, const char *value )
{
	Document *docValues = Parser::parseFile( pathValues );

	docValues->props().erase( name );

	docValues->add( new Property(DocumentLocation(docValues, 0), name, value) );

	docValues->write( cout );

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
