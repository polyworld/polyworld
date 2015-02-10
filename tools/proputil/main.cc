#include <stdlib.h>
#include <string.h>

#include <fstream>
#include <iostream>
#include <list>
#include <string>
#include <utility>

#include "misc.h"
#include "builder.h"
#include "editor.h"
#include "overlay.h"
#include "schema.h"
#include "writer.h"

using namespace proplib;
using namespace std;

void err( string msg )
{
	cerr << msg << endl;
	exit( 1 );
}

void usage( string msg = "" )
{
	cerr << "usage: proputil [-w] apply path_schema path_doc" << endl;
	cerr << "       proputil [-w] get [-s path_schema] path_doc propname" << endl;
	cerr << "       proputil [-w] set [-s path_schema] path_doc propname=propvalue..." << endl;
	cerr << "       proputil [-w] len [-s path_schema] path_doc propname" << endl;
	cerr << "       proputil [-w] overlay [-s path_schema] path_doc path_overlay index" << endl;
	cerr << "       proputil scalarnames path_doc depth_start" << endl;
	cerr << "       proputil dbgsyntax path_doc" << endl;
	cerr << "";
	cerr << "   -w: Treat doc as worldfile, which may entail property conversion." << endl;
	cerr << "       If used, then -s must also be used." << endl;

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

bool isWorldfile = false;

void apply( const char *pathSchema, const char *pathDoc );
void get( const char *pathDoc, const char *name );
void get( const char *pathSchema, const char *pathDoc, const char *name );
void set( const char *pathDoc, NameValuePairList &pairs );
void set( const char *pathSchema, const char *pathDoc, NameValuePairList &pairs );
void len( const char *pathSchema, const char *pathDoc, const char *name );
void len( const char *pathDoc, const char *name );
void overlay( const char *pathDoc, const char *pathOverlay, const char *index );
void overlay( const char *pathSchema, const char *pathDoc, const char *pathOverlay, const char *index );
void scalarnames( const char *pathDoc, const char *depth_start );
void dbgsyntax( const char *pathDoc );

int main( int argc, const char **argv )
{
	if( argc >= 2 )
	{
		if( string(argv[1]) == "-w" )
		{
			isWorldfile = true;

			bool schema = false;

			argc--;
			for( int i = 1; i < argc; i++ )
			{
				argv[i] = argv[ i + 1 ];
				string arg = argv[i];
				if( arg == "-s" || arg == "apply" )
				{
					schema = true;
				}
			}

			if( !schema )
				usage( "-w requires -s" );
		}
	}

	if( argc < 2 )
	{
		usage( "Must specify mode" );
	}

	Interpreter::init();

	string mode = argv[1];

	if( mode == "apply" )
	{
		if( argc != 4 )
		{
			usage();
		}

		apply( argv[2], argv[3] );
	}
	else if( mode == "get" )
	{
		if( argc == 4 )
		{
			get( argv[2], argv[3] );
		}
		else if( argc == 6 )
		{
			if( string(argv[2]) != "-s" )
				usage();

			get( argv[3], argv[4], argv[5] );
		}
		else
			usage();
	}
	else if( mode == "set" )
	{
		if( argc < 4 )
		{
			usage();
		}

		if( string(argv[2]) == "-s" )
		{
			NameValuePairList pairs;

			for( int i = 5; i < argc; i++ )
				pairs.push_back( NameValuePair::parse(argv[i]) );

			::set( argv[3], argv[4], pairs );
		}
		else
		{
			NameValuePairList pairs;

			for( int i = 3; i < argc; i++ )
				pairs.push_back( NameValuePair::parse(argv[i]) );

			::set( argv[2], pairs );
		}
	}
	else if( mode == "len" )
	{
		if( argc == 4 )
		{
			len( argv[2], argv[3] );
		}
		else if( argc == 6 )
		{
			if( string(argv[2]) != "-s" )
				usage();

			len( argv[3], argv[4], argv[5] );
		}
		else
			usage();

	}
	else if( mode == "overlay" )
	{
		if( argc == 5 )
		{
			overlay( argv[2], argv[3], argv[4] );
		}
		else if( argc == 7 )
		{
			if( string(argv[2]) != "-s" )
				usage();

			overlay( argv[3], argv[4], argv[5], argv[6] );
		}
	}
	else if( mode == "scalarnames" )
	{
		if( argc != 4 )
		{
			usage();
		}

		scalarnames( argv[2], argv[3] );
	}
	else if( mode == "dbgsyntax" )
	{
		if( argc != 3 )
		{
			usage();
		}

		dbgsyntax( argv[2] );
	}
	else
	{
		usage( "Invalid mode" );
	}

	Interpreter::dispose();

	return 0;
}

Document *parseDoc( SchemaDocument *schema, const char *pathDoc )
{
	DocumentBuilder builder;

	if( isWorldfile )
		return builder.buildWorldfileDocument( schema, pathDoc );
	else
		return builder.buildDocument( pathDoc );
}

void apply( const char *pathSchema, const char *pathDoc )
{
	DocumentBuilder builder;
	SchemaDocument *schema = builder.buildSchemaDocument( pathSchema );
	Document *doc = parseDoc( schema, pathDoc );

	schema->apply( doc );

	DocumentWriter writer( cout );
	writer.write( doc );

	delete schema;
	delete doc;
}

void __get( Document *doc, const char *name )
{
	DocumentBuilder builder;
	SymbolPath *symbolPath = builder.buildSymbolPath( name );

	Symbol sym;
	if( !doc->props().begin()->second->findSymbol(symbolPath, sym) )
		err( string("Cannot locate '") + symbolPath->toString() + "'" );

	if( (sym.type == Symbol::Property)
		&& (sym.prop->getType() == Node::Scalar)
		&& (sym.prop->getSubtype() == Node::Const) )
	{
		cout << (string)*sym.prop << endl;
	}
	else
		err( "Not a valid property type for querying." );

	delete symbolPath;
}

void get( const char *pathDoc, const char *name )
{
	DocumentBuilder builder;
	Document *doc = builder.buildDocument( pathDoc );

	__get( doc, name );

	delete doc;
}

void get( const char *pathSchema, const char *pathDoc, const char *name )
{
	DocumentBuilder builder;
	SchemaDocument *schema = builder.buildSchemaDocument( pathSchema );
	Document *doc = parseDoc( schema, pathDoc );

	schema->apply( doc );

	__get( doc, name );

	delete schema;
	delete doc;
}

void set( const char *pathDoc, NameValuePairList &pairs )
{
	DocumentBuilder builder;
	Document *doc = builder.buildDocument( pathDoc );
	DocumentEditor editor( doc );

	itfor( NameValuePairList, pairs, it )
		editor.set( it->name, it->value );

	DocumentWriter writer( cout );
	writer.write( doc );

	delete doc;
}

void set( const char *pathSchema, const char *pathDoc, NameValuePairList &pairs )
{
	DocumentBuilder builder;
	SchemaDocument *schema = builder.buildSchemaDocument( pathSchema );
	Document *doc = parseDoc( schema, pathDoc );
	DocumentEditor editor( schema, doc );

	itfor( NameValuePairList, pairs, it )
		editor.set( it->name, it->value );

	DocumentWriter writer( cout );
	writer.write( doc );

	delete schema;
	delete doc;
}

void __len( Document *doc, const char *name )
{
	DocumentBuilder builder;
	SymbolPath *symbolPath = builder.buildSymbolPath( name );

	Symbol sym;
	if( !doc->props().begin()->second->findSymbol(symbolPath, sym) )
		err( string("Cannot locate '") + symbolPath->toString() + "'" );

	if( (sym.type == Symbol::Property)
		&& (sym.prop->getType() == Node::Array) )
		cout << sym.prop->size() << endl;
	else
		err( "Not an array property." );

	delete symbolPath;
}

void len( const char *pathDoc, const char *name )
{
	DocumentBuilder builder;
	Document *doc = builder.buildDocument( pathDoc );

	__len( doc, name );

	delete doc;
}

void len( const char *pathSchema, const char *pathDoc, const char *name )
{
	DocumentBuilder builder;
	SchemaDocument *schema = builder.buildSchemaDocument( pathSchema );
	Document *doc = parseDoc( schema, pathDoc );

	schema->apply( doc );

	__len( doc, name );

	delete schema;
	delete doc;
}

void overlay( const char *pathDoc, const char *pathOverlay, const char *selector )
{
	DocumentBuilder builder;
	Document *doc = builder.buildDocument( pathDoc );
	Document *overlayDoc = builder.buildDocument( pathOverlay );

	DocumentEditor editor( doc );
	Overlay overlay;
	overlay.applyDocument( overlayDoc, atoi(selector), &editor );

	DocumentWriter writer( cout );
	writer.write( doc );
}

void overlay( const char *pathSchema, const char *pathDoc, const char *pathOverlay, const char *selector )
{
	DocumentBuilder builder;
	SchemaDocument *schema = builder.buildSchemaDocument( pathSchema );
	Document *doc = parseDoc( schema, pathDoc );
	Document *overlayDoc = builder.buildDocument( pathOverlay );

	DocumentEditor editor( schema, doc );
	Overlay overlay;
	overlay.applyDocument( overlayDoc, atoi(selector), &editor );

	DocumentWriter writer( cout );
	writer.write( doc );
}

void scalarnames( const char *pathDoc, const char *depth_start )
{
	DocumentBuilder builder;
	Document *doc = builder.buildDocument( pathDoc );

	DocumentWriter writer( cout );
	writer.writeScalarNames( doc, atoi(depth_start) );

	delete doc;
}

void dbgsyntax( const char *pathDoc )
{
	Parser parser;
	parser.parseDocument( pathDoc, new ifstream(pathDoc) )->dump( cout );
}
