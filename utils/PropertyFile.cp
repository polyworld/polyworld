#include "PropertyFile.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fstream>
#include <iostream>

#include "misc.h"

using namespace std;

namespace PropertyFile
{

	static void dump( list<char *> &l )
	{
		itfor( list<char *>, l, it )
		{
			cout << *it << "/";
		}
		cout << endl;
	}

	static void freeall( list<char *> &l )
	{
		itfor( list<char *>, l, it )
		{
			free( *it );
		}
		l.clear();
	}

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS Identifier
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------

	Identifier::Identifier( const char *name )
	{
		this->name = strdup( name );
	}

	Identifier::Identifier( int index )
	{
		char buf[32];
		sprintf( buf, "%d", index );
		this->name = strdup( buf );
	}

	Identifier::Identifier( const Identifier &other )
	{
		this->name = strdup( other.name );
	}

	Identifier::~Identifier()
	{
		free( this->name );
		this->name = NULL;
	}

	const char *Identifier::getName() const
	{
		return this->name;
	}

	bool operator<( const Identifier &a, const Identifier &b )
	{
		return strcmp( a.getName(), b.getName() ) < 0;
	}

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS Property
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------

	Property::Property( Identifier _id )
		: id( _id )
	{
		type = OBJECT;
		oval = new PropertyMap();
	}

	Property::Property( Identifier _id, const char *val )
		: id( _id )
	{
		type = SCALAR;
		sval = strdup( val );
	}

	Property::~Property()
	{
		switch( type )
		{
		case OBJECT:
			itfor( PropertyMap, *oval, it )
			{
				delete it->second;
			}
			delete oval;
			break;
		case SCALAR:
			free( sval );
			break;
		default:
			assert( false );
		}
	}

	const char *Property::getName()
	{
		return id.getName();
	}

	Property *Property::get( Identifier id )
	{
		assert( type == OBJECT );

		return (*oval)[ id ];
	}

	int Property::get( Identifier id, int def )
	{
		char *val = getscalar( id );
		if( val == NULL )
		{
			return def;
		}

		char *end;
		int ival = (int)strtol( sval, &end, 10 );
		assert( *end == '\0' );

		return ival;
	}

	bool Property::get( Identifier id, bool def )
	{
		char *val = getscalar( id );
		if( val == NULL )
		{
			return def;
		}

		return 0 != strcmp( val, "0" );
	}

	Property &Property::operator[]( Identifier id )
	{
		return *get( id );
	}

	void Property::add( Property *prop )
	{
		assert( type == OBJECT );

		(*oval)[ prop->id ] = prop;
	}

	void Property::dump( ostream &out, const char *indent )
	{
		out << indent << getName();

		if( type == OBJECT )
		{
			out << endl;
			itfor( PropertyMap, *oval, it )
			{
				it->second->dump( out, (string(indent) + "  ").c_str() );
			}
		}
		else
		{
			out << " = " << sval << endl;
		}

	}

	char *Property::getscalar( Identifier id )
	{
		assert( type == OBJECT );

		Property *prop = get( id );

		if( prop == NULL )
		{
			return NULL;
		}
		
		assert( prop->type == SCALAR );

		return prop->sval;
	}

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS Document
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------

	Document::Document( Identifier id )
		: Property( id )
	{
	}

	Document::~Document()
	{
	}

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS Parser
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	
	Document *Parser::parse( const char *path )
	{
		Document *doc = new Document( path );

		ifstream in( path );
		char *line;

		while( (line = readline(in)) != NULL )
		{
			cout << "'" << line << "'" << endl;

			StringList tokens;
			tokenize( line, tokens );

			addProperty( doc, tokens );

			freeall( tokens );
			delete line;
		}

		return doc;
	}

	char *Parser::readline( istream &in )
	{
		char buf[1024 * 4];
		char *result = NULL;

		do
		{
			in.getline( buf, sizeof(buf) );
			if( !in.good() )
			{
				break;
			}

			char *line = buf;

			// remove comments
			{
				char *comment = strchr( line, '#' );
				if( comment != NULL )
				{
					*comment = '\0';
				}
			}

			// remove leading whitespace
			{
				for( ; *line != '\0' && isspace(*line); line++ )
				{
				}
			}

			// remove trailing whitespace
			{
				char *end = line + strlen(line) - 1;
				for( ; end >= line && isspace(*end); end-- )
				{
					*end = '\0';
				}
			}

			// is it a non-empty string?
			if( *line != '\0' )
			{
				result = strdup( line );
			}

		} while( result == NULL );

		return result;
	}

	void Parser::tokenize( char *line, StringList &list )
	{
		char *saveptr;
		char *tok;

		for( ; (tok = strtok_r(line, " \t\n", &saveptr)) != NULL; line = NULL )
		{
			list.push_back( strdup(tok) );
		}
	}

	void Parser::addProperty( Document *doc, StringList &tokens )
	{
		assert( tokens.size() > 1 );

		char *label = tokens.back();
		StringList path;
		Property *parent = doc;
		
		parsePath( label, path );

		for( StringList::iterator
				 it = path.begin(),
				 end = --path.end();
			 it != end;
			 it++ )
		{
			Property *child = parent->get( *it );

			if( child == NULL )
			{
				child = new Property( *it );
				parent->add( child );
			}

			parent = child;
		}

		if( tokens.size() == 2 )
		{
			Property *prop = new Property( path.back(), tokens.front() );
			parent->add( prop );
		}
		else
		{
			Property *list = new Property( path.back() );
			parent->add( list );

			int index = 0;
			for( StringList::iterator
					 it = tokens.begin(),
					 end = --tokens.end();
				 it != end;
				 it++ )
			{
				Property *element = new Property( index++, *it );
				list->add( element );
			}
		}

		freeall( path );
	}

	void Parser::parsePath( char *label, StringList &path )
	{
	    char *start = label;
		char *end;

#define ADDID() \
		if( end != start)									\
		{													\
			path.push_back( strndup(start, end - start) );	\
			start = end + 1;								\
		}													\
		else												\
		{													\
			start++;										\
		}

		for( end = start; *end; end++ )
		{
			switch( *end )
			{
			case '[':
			case ']':
			case '.':
				ADDID();
			break;
			}
		}

		ADDID();
	}
}

#if 0
using namespace PropertyFile;

int main( int argc, char **argv )
{
	/*
	Document doc("Doc");
	Property *array = new Property("array");
	Property *iprop = new Property("iprop", 42);

	doc.add( array );
	doc.add( iprop );

	cout << doc.getName() << endl;
	cout << doc["array"].getName() << endl;
	cout << doc.get( "iprop", -1 ) << endl;
	cout << doc.get( "iprop_bad", -1 ) << endl;
	*/
	
	Document *_doc = Parser::parse( "foo.txt" );
	Document &doc = *_doc;

	doc.dump( cout );

	delete _doc;
	
	return 0;
}
#endif
