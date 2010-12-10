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

	Property::Property( Identifier _id, bool _isArray )
		: id( _id )
		, isArray( _isArray )
	{
		type =  OBJECT;
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
			if( isArray )
			{
				out << " [";
			}
			out << endl;
			itfor( PropertyMap, *oval, it )
			{
				it->second->dump( out, (string(indent) + "  ").c_str() );
			}
			if( isArray )
			{
				out << indent << "]" << endl;
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
		PropertyStack propertyStack;

		propertyStack.push( doc );

		ifstream in( path );
		char *line;

		while( (line = readline(in)) != NULL )
		{
			cout << "'" << line << "'" << endl;

			StringList tokens;
			tokenize( line, tokens );

			processLine( doc,
						 propertyStack,
						 tokens );

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

	void Parser::processLine( Document *doc,
							  PropertyStack &propertyStack,
							  StringList &tokens )
	{
		size_t ntokens = tokens.size();
		char *nameToken = tokens.front();
		char *valueToken = tokens.back();
		

		if( 0 == strcmp("]", valueToken) )
		{
			assert( ntokens == 1 );

			if( !propertyStack.top()->isArray )
			{
				// close out the object for the last element.
				propertyStack.pop();

				assert( propertyStack.top()->isArray );
			}

			propertyStack.pop();
		}
		else
		{
			if( propertyStack.top()->isArray )
			{
				// we need a container for the coming element.

				Property *prop = new Property( propertyStack.top()->oval->size() );
				propertyStack.top()->add( prop );
				propertyStack.push( prop );				
			}

			if( 0 == strcmp("{", valueToken) )
			{
				assert( ntokens == 2 );

				char *objectName = nameToken;
			
				Property *prop = new Property( objectName );
				propertyStack.top()->add( prop );
				propertyStack.push( prop );
			}
			else if( 0 == strcmp("}", valueToken) )
			{
				assert( ntokens == 1 );

				propertyStack.pop();
			}
			else if( 0 == strcmp("[", valueToken) )
			{
				assert( ntokens == 2 );

				char *arrayName = nameToken;
			
				Property *propArray = new Property( arrayName, true );
				propertyStack.top()->add( propArray );
				propertyStack.push( propArray );
			}
			else if( 0 == strcmp(",", valueToken) )
			{
				propertyStack.pop();
			}
			else
			{
				assert( ntokens == 2 );

				char *propName = nameToken;
				char *value = valueToken;

				Property *prop = new Property( propName, value );
			
				propertyStack.top()->add( prop );
			}
		}
	}
}


#if 0
using namespace PropertyFile;

int main( int argc, char **argv )
{
	Document *_doc = Parser::parse( "foo.txt" );
	Document &doc = *_doc;

	doc.dump( cout );

	delete _doc;
	
	return 0;
}
#endif
