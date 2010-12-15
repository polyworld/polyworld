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

	bool eval( const char *expr,
			   map<string,string> symbols,
			   char *result, size_t result_size )
	{
		string locals = "{";
		for(map<string,string>::iterator
				it = symbols.begin(),
				itend = symbols.end();
			it != itend;
			it++ )
		{
			locals.append( "'" + it->first + "': " + it->second );
			locals.append( "," ); // python allows final dangling comma
		}
		locals.append( "}" );

		char script[1024 * 4];
		sprintf( script,
				 "import sys\n"
				 "try:\n"
				 "  print eval('%s', %s)\n"
				 "except:\n"
				 "  print sys.exc_info()[1]\n"
				 "  sys.exit(1)\n",
				 expr,
				 locals.c_str());

		char cmd[1024 * 4];
		sprintf( cmd, "python -c \"%s\"", script );

		FILE *f = popen( cmd, "r" );

		char *result_orig = result;

		size_t n;
		while( (result_size > 0) && (n = fread(result, 1, result_size, f)) != 0 )
		{
			result[n] = 0;
			result += n;
			result_size -= n;
		}

		int rc = pclose( f );

		for( char *tail = result - 1; tail >= result_orig; tail-- )
		{
			if( *tail == '\n' )
				*tail = 0;
			else
				break;
		}

		return rc == 0;
	}

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS Identifier
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------

	Identifier::Identifier( const string &name )
	{
		this->name = name;
	}

	Identifier::Identifier( int index )
	{
		char buf[32];
		sprintf( buf, "%d", index );
		this->name = buf;
	}

	Identifier::Identifier( unsigned int index )
	{
		char buf[32];
		sprintf( buf, "%u", index );
		this->name = buf;
	}

	Identifier::Identifier( size_t index )
	{
		char buf[32];
		sprintf( buf, "%lu", index );
		this->name = buf;
	}

	Identifier::Identifier( const char *name)
	{
		this->name = name;
	}

	Identifier::Identifier( const Identifier &other )
	{
		this->name = other.name;
	}

	Identifier::~Identifier()
	{
	}

	const char *Identifier::getName() const
	{
		return this->name.c_str();
	}

	bool operator<( const Identifier &a, const Identifier &b )
	{
		return strcmp( a.getName(), b.getName() ) < 0;
	}

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS DocumentLocation
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	DocumentLocation::DocumentLocation( Document *_doc, unsigned int _lineno )
		: doc( _doc )
		, lineno( _lineno )
	{
	}

	string DocumentLocation::getDescription()
	{
		char buf[1024 * 4];

		sprintf( buf, "%s:%u", doc->getName(), lineno );

		return buf;
	}

	string DocumentLocation::getPath()
	{
		return doc->getName();
	}

	void DocumentLocation::err( string msg )
	{
		fprintf( stderr, "%s:%u: %s\n", doc->getName(), lineno, msg.c_str() );
		exit( 1 );
	}

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS Property
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------

	Property::Property( DocumentLocation _loc, Identifier _id, bool _isArray )
		: parent( NULL )
		, symbolSource( NULL )
		, loc( _loc )
		, id( _id )
		, isArray( _isArray )
		, isExpr( false )
		, isEval( false )
	{
		type =  OBJECT;
		oval = new PropertyMap();
	}

	Property::Property( DocumentLocation _loc, Identifier _id, const char *val )
		: parent( NULL )
		, symbolSource( NULL )
		, loc( _loc )
		, id( _id )
		, isEval( false )
	{
		type = SCALAR;

		size_t n = strlen( val );
		isExpr = (n > 3) && (val[0] == '$') && (val[1] == '(') && (val[n - 1] == ')');
		if( isExpr )
		{
			val += 2; n -= 2; // skip $(
			n -= 1; // ignore )

			for( ; isspace(*val); val++, n-- ) {}
			for( ; isspace(val[n - 1]); n-- ) {}

			sval = strndup( val, n );
		}
		else
		{
			sval = strdup( val );
		}
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

	Property &Property::get( Identifier id )
	{
		assert( type == OBJECT );

		Property *prop = getp( id );

		if( prop == NULL )
		{
			err( string("Expecting property '") + id.getName() + "' for '" + getName() + "'");
		}

		return *prop;
	}

	Property *Property::getp( Identifier id )
	{
		assert( type == OBJECT );

		PropertyMap::iterator it = oval->find( id );
		if( it != oval->end() )
		{
			return it->second;
		}
		else
		{
			return NULL;
		}
	}

	Property *Property::find( Identifier id )
	{
		if( symbolSource != NULL )
		{
			return symbolSource->find( id );
		}
		else
		{
			Property *result = NULL;

			if( type == OBJECT )
			{
				result = getp( id );
			}

			if( result == NULL && parent != NULL )
			{
				result = parent->find( id );
			}

			return result;
		}
	}

	Property::operator int()
	{
		int ival;

		if( (type != SCALAR) || !Parser::parseInt(getScalarValue(), &ival) )
		{
			err( string("Expecting INT value for property '") + getName() + "'." );
		}

		return ival;
	}

	Property::operator float()
	{
		float fval;

		if( (type != SCALAR) || !Parser::parseFloat(getScalarValue(), &fval) )
		{
			err( string("Expecting FLOAT value for property '") + getName() + "'." );
		}

		return fval;
	}

	Property::operator bool()
	{
		bool bval;

		if( (type != SCALAR) || !Parser::parseBool(getScalarValue(), &bval) )
		{
			err( string("Expecting BOOL value for property '") + getName() + "'." );
		}

		return bval;
	}

	Property::operator string()
	{
		assert( type == SCALAR );

		return string( sval );
	}

	void Property::add( Property *prop )
	{
		assert( type == OBJECT );

		prop->parent = this;

		(*oval)[ prop->id ] = prop;
	}

	Property *Property::clone()
	{
		Property *clone = NULL;

		switch( type )
		{
		case SCALAR:
			clone = new Property( loc, id, sval );
			clone->isExpr = isExpr;
			break;
		case OBJECT:
			clone = new Property( loc, id, isArray );

			clone->oval = new PropertyMap();
			itfor( PropertyMap, *oval, it )
			{
				clone->add( it->second->clone() );
			}
			break;
		default:
			assert( false );
		}

		return clone;
	}

	void Property::err( string message )
	{
		loc.err( message );
	}

	void Property::dump( ostream &out, const char *indent )
	{
		out << loc.getDescription() << " ";
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
				out << loc.getDescription() << " ";
				out << indent << "]" << endl;
			}
		}
		else
		{
			out << " = " << sval << endl;
		}
	}

	string Property::getScalarValue()
	{
		if( type != SCALAR )
		{
			loc.err( "Referenced as scalar, probably from expression." );
		}

		if( isExpr )
		{
			// We eventually want to move the isEval state out of the object and the stack so we are thread safe.
			if( isEval )
			{
				loc.err( "Expression dependency cycle." );
			}
			isEval = true;

			map<string, string> symbolTable;

			Parser::StringList ids;
			Parser::scanIdentifiers( sval, ids );

			itfor( Parser::StringList, ids, it )
			{
				Property *prop = find( *it );
				if( prop != NULL )
				{
					symbolTable[ prop->getName() ] = prop->getScalarValue();
				}
			}

			char buf[1024];

			bool success = eval( sval, symbolTable,
								 buf, sizeof(buf) );

			if( !success )
			{
				loc.err( buf );
			}

			isEval = false;

			return buf;
		}
		else
		{
			return sval;
		}
	}

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS Document
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------

	Document::Document( const char *name )
		: Property( DocumentLocation(this,0), string(name) )
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
		DocumentLocation loc( doc, 0 );
		PropertyStack propertyStack;

		propertyStack.push( doc );

		ifstream in( path );
		char *line;

		while( (line = readline(in, loc)) != NULL )
		{
			CStringList tokens;
			tokenize( loc, line, tokens );

			processLine( doc,
						 loc,
						 propertyStack,
						 tokens );

			freeall( tokens );
			delete line;
		}

		if( propertyStack.size() > 1 )
		{
			loc.err( "Unexpected end of document. Likely missing '}' or ']'" );
		}

		return doc;
	}

	bool Parser::isValidIdentifier( const string &_text )
	{
		const char *text = _text.c_str();
		bool valid = false;

		if( isalpha(*text) )
		{
			valid = true;

			for( const char *c = text; *c && valid; c++ )
			{
				valid = isalnum(*c);
			}
		}

		return valid;
	}

	void Parser::scanIdentifiers( const string &_expr, StringList &ids )
	{
		const char *expr = _expr.c_str();
		const char *start = NULL;
		const char *end = expr;

#define ADDID()												\
		if( start )											\
		{													\
			ids.push_back( string(start, end - start) );	\
			start = NULL;									\
		}

		for( end = expr; *end; end++ )
		{
			if( !start )
			{
				if( isalpha(*end) )
				{
					start = end;
				}
			}
			else
			{
				if( !isalnum(*end) )
				{
					ADDID();
				}
			}
		}

		ADDID();
	}


	bool Parser::parseInt( const string &text, int *result )
	{
		char *end;
		int ival = (int)strtol( text.c_str(), &end, 10 );
		if( *end != '\0' )
		{
			return false;
		}

		if( result != NULL )
		{
			*result = ival;
		}

		return true;
	}

	bool Parser::parseFloat( const string &text, float *result )
	{
		char *end;
		float fval = (float)strtof( text.c_str(), &end );
		if( *end != '\0' )
		{
			return false;
		}

		if( result != NULL )
		{
			*result = fval;
		}

		return true;
	}

	bool Parser::parseBool( const string &text, bool *result )
	{
		bool bval;

		if( text == "True" )
		{
			bval = true;
		}
		else if( text == "False" )
		{
			bval = false;
		}
		else
		{
			return false;
		}

		if( result != NULL )
		{
			*result = bval;
		}

		return true;
	}

	char *Parser::readline( istream &in,
							DocumentLocation &loc )
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

			loc.lineno++;

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

	void Parser::tokenize( DocumentLocation &loc,
						   char *line,
						   CStringList &list )
	{
		char *start = NULL;
		char *end;
		int paren = 0;

#define ADDTOK( END )											\
		if( start )												\
		{														\
			list.push_back( strndup(start, (END) - start) );	\
			start = NULL;										\
			paren = 0;											\
		}

		for( end = line; *end; end++ )
		{
			bool expr = paren > 0;

			if( !start )
			{
				if( *end == '$' && *(end + 1) == '(' )
				{
					start = end;
					end++;
					paren++;
					expr = true;
				}
				else if( !isspace(*end) )
				{
					start = end;
				}
			}
			else
			{
				if( paren )
				{
					if( *end == '(' )
					{
						paren++;
					}
					else if( *end == ')' )
					{
						if( --paren == 0 )
						{
							ADDTOK( end + 1 );
						}
					}
				}
				else
				{
					if( isspace(*end) )
					{
						ADDTOK( end );
					}
				}
			}

			if( !expr )
			{
				if( *end == '(' )
				{
					loc.err( "Unexpected '('" );
				}
				else if( *end == ')' )
				{
					loc.err( "Unexpected ')'" );
				}
			}
		}

		if( paren )
		{
			loc.err( "Missing ')'" );
		}
		else
		{
			ADDTOK( end );
		}
	}

	void Parser::processLine( Document *doc,
							  DocumentLocation &loc,
							  PropertyStack &propertyStack,
							  CStringList &tokens )
	{
		size_t ntokens = tokens.size();
		string nameToken = tokens.front();
		string valueToken = tokens.back();
		

		if( valueToken == "]" )
		{
			if( ntokens != 1 )
			{
				loc.err( "Expecting only ']'" );
			}
			if( propertyStack.size() < 2 )
			{
				loc.err( "Extraneous ']'" );
			}

			
			if( !propertyStack.top()->isArray )
			{
				// close out the object for the last element.
				propertyStack.pop();

				if( !propertyStack.top()->isArray )
				{
					loc.err( "Unexpected ']'." );
				}

				if( propertyStack.size() < 2 )
				{
					loc.err( "Extraneous ']'" );
				}
			}
			else
			{
				Property *propArray = propertyStack.top();
				int nelements = (int)propArray->oval->size();

				if( nelements > 0 )
				{
					if( propArray->get(0).type == Property::OBJECT )
					{
						// we must have had a hanging ','. We force an empty object as a final element.
						propArray->add( new Property( loc, nelements ) );
					}
				}
			}

			propertyStack.pop();
		}
		else
		{
			bool isScalarArray = false;

			if( propertyStack.top()->isArray )
			{
				Property *propArray = propertyStack.top();

				// If this is the first element.
				if( propArray->oval->size() == 0 )
				{
					if( (ntokens == 1) && (valueToken != ",") )
					{
						isScalarArray = true;
					}
				}
				else
				{
					isScalarArray = propArray->get(0).type == Property::SCALAR;
				}

				if( !isScalarArray )
				{
					// we need a container for the coming element.
					size_t index = propertyStack.top()->oval->size();
					Property *prop = new Property( loc, index );
					propertyStack.top()->add( prop );
					propertyStack.push( prop );
				}
			}

			if( isScalarArray )
			{
				if( ntokens != 1 )
				{
					loc.err( "Expecting only 1 token in scalar array element." );
				}
				if( valueToken == "," )
				{
					loc.err( "Commas not allowed in scalar array." );
				}

				size_t index = propertyStack.top()->oval->size();
				propertyStack.top()->add( new Property(loc, index, valueToken.c_str()) );
			}
			else
			{
				if( valueToken == "{" )
				{
					if( ntokens != 2 )
					{
						loc.err( "Expecting 'NAME {'" );
					}

					string &objectName = nameToken;
			
					Property *prop = new Property( loc, objectName );
					propertyStack.top()->add( prop );
					propertyStack.push( prop );
				}
				else if( valueToken == "}" )
				{
					if( ntokens != 1 )
					{
						loc.err( "Expecting '}' only." );
					}

					if( propertyStack.size() < 2 )
					{
						loc.err( "Extraneous '}'" );
					}

					propertyStack.pop();
				}
				else if( valueToken == "[" )
				{
					if( ntokens != 2 )
					{
						loc.err( "Expecting 'NAME ['" );
					}

					string &arrayName = nameToken;
			
					Property *propArray = new Property( loc, arrayName, true );
					propertyStack.top()->add( propArray );
					propertyStack.push( propArray );
				}
				else if( valueToken == "," )
				{
					if( ntokens != 1 )
					{
						loc.err( "Expecting comma only." );
					}

					propertyStack.pop();
				}
				else
				{
					if( ntokens != 2 )
					{
						loc.err( "Expecting 'NAME VALUE'" );
					}

					string &propName = nameToken;
					string &value = valueToken;

					if( !isValidIdentifier(propName.c_str()) )
					{
						loc.err( "Invalid name: " + propName );
					}

					Property *prop = new Property( loc, propName, value.c_str() );
			
					propertyStack.top()->add( prop );
				}
			}
		}
	}

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS Schema
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------

	void Schema::apply( Document *docSchema, Document *docValues )
	{
		injectDefaults( *docSchema, *docValues );
		validateChildren( *docSchema, *docValues );
	}

	void Schema::injectDefaults( Property &propSchema, Property &propValue )
	{
		assert( propSchema.type == Property::OBJECT );
		assert( propValue.type == Property::OBJECT );

		itfor( Property::PropertyMap, *(propSchema.oval), it )
		{
			Property *childSchema = it->second;
			Property *childValue = propValue.getp( childSchema->id );

			if( childValue == NULL )
			{
				Property *propDefault = childSchema->getp( "default" );
				if( propDefault == NULL )
				{
					childSchema->err( string(childSchema->getName()) + " not found in " + propValue.loc.getPath() + "." );
				}
				else
				{
					Property *propClone = propDefault->clone();
					propClone->id = childSchema->id;

					propValue.add( propClone );
				}
			}
		}
	}

	void Schema::validateChildren( Property &propSchema, Property &propValue )
	{
		assert( propSchema.type == Property::OBJECT );
		assert( propValue.type == Property::OBJECT );

		itfor( Property::PropertyMap, *(propValue.oval), it )
		{
			Property *childValue = it->second;
			Property *childSchema = propSchema.getp( childValue->id );

			if( childSchema == NULL )
			{
				childValue->err( string(childValue->getName()) + " not in schema (" + propSchema.loc.getPath() + ")" );
			}

			validateProperty( *childSchema, *childValue );
		}
	}

	void Schema::validateProperty(  Property &propSchema, Property &propValue )
	{
		string name = propValue.getName();
		Property &propType = propSchema.get( "type" );
		string type = propType;

		// --
		// -- Verify the value is of the correct type.
		// --
		if( type == "INT" )
		{
			(int)propValue;
		}
		else if( type == "FLOAT" )
		{
			(float)propValue;
		}
		else if( type == "BOOL" )
		{
			(bool)propValue;
		}
		else if( type == "ARRAY" )
		{
			if( !propValue.isArray )
			{
				propValue.err( string("Expecting ARRAY value for property '") + propValue.getName() + "'" );
			}
		}
		else if( type == "OBJECT" )
		{
			if( (propValue.type != Property::OBJECT) || propValue.isArray )
			{
				propValue.err( string("Expecting OBJECT value for property '") + propValue.getName() + "'" );
			}
		}
		else if( type == "ENUM" )
		{
			Property &propEnumValues = propSchema.get( "values" );
			if( !propEnumValues.isArray )
			{
				propEnumValues.err( "Expecting array of enum values." );
			}

			bool valid = false;

			itfor( Property::PropertyMap, *(propEnumValues.oval), it )
			{
				if( (string)propValue == (string)*(it->second) )
				{
					valid = true;
					break;
				}
			}

			if( !valid )
			{
				Property *propScalar = propSchema.getp( "scalar" );
				if( propScalar )
				{
					string scalarType = propScalar->get( "type" );
					if( scalarType == "INT" || scalarType == "FLOAT" || scalarType == "BOOL" )
					{
						validateProperty( *propScalar, propValue );
						valid = true;
					}
					else
					{
						propScalar->err( "Invalid scalar type for enum." );
					}
				}
			}

			if( !valid )
			{
				propValue.err( "Invalid enum value." );
			}
		}
		else
		{
			propType.err( string("Invalid type '") + type + "'"  );
		}

		// --
		// -- Iterate over schema attributes for this property.
		// --
		itfor( Property::PropertyMap, *propSchema.oval, it )
		{
			string attrName = it->first.getName();
			Property &attrVal = *(it->second);

			attrVal.symbolSource = &propValue;

			// --
			// -- min
			// --
			if( attrName == "min" )
			{
				if( type == "ARRAY" )
				{
					bool valid = propValue.isArray
						&& (int)propValue.oval->size() >= (int)attrVal;

					if( !valid )
					{
						propValue.err( name + " element count less than min " + (string)attrVal );
					}
				}
				else
				{
					bool valid = true;

					if( type == "INT" )
					{
						valid = (int)propValue >= (int)attrVal;
					}
					else if( type == "FLOAT" )
					{
						valid = (float)propValue >= (float)attrVal;
					}
					else
					{
						propSchema.err( string("'min' is not valid for type ") + type );
					}

					if( !valid )
					{
						propValue.err( (string)propValue + " less than min " + (string)attrVal );
					}
				}
			}

			// --
			// -- xmin
			// --
			else if( attrName == "xmin" )
			{
				if( type == "ARRAY" )
				{
					bool valid = propValue.isArray
						&& (int)propValue.oval->size() > (int)attrVal;

					if( !valid )
					{
						propValue.err( name + " element count <= xmin " + (string)attrVal );
					}
				}
				else
				{
					bool valid = true;

					if( type == "INT" )
					{
						valid = (int)propValue > (int)attrVal;
					}
					else if( type == "FLOAT" )
					{
						valid = (float)propValue > (float)attrVal;
					}
					else
					{
						propSchema.err( string("'xmin' is not valid for type ") + type );
					}

					if( !valid )
					{
						propValue.err( (string)propValue + " <= xmin " + (string)attrVal );
					}
				}
			}

			// --
			// -- max
			// --
			else if( attrName == "max" )
			{
				if( type == "ARRAY" )
				{
					bool valid = propValue.isArray
						&& (int)propValue.oval->size() <= (int)attrVal;

					if( !valid )
					{
						propValue.err( name + " element count greater than max " + (string)attrVal );
					}
				}
				else
				{
					bool valid = true;

					if( type == "INT" )
					{
						valid = (int)propValue <= (int)attrVal;
					}
					else if( type == "FLOAT" )
					{
						valid = (float)propValue <= (float)attrVal;
					}
					else
					{
						propSchema.err( string("'max' is not valid for type ") + type );
					}

					if( !valid )
					{
						propValue.err( (string)propValue + " greater than max " + (string)attrVal );
					}
				}
			}

			// --
			// -- xmax
			// --
			else if( attrName == "xmax" )
			{
				if( type == "ARRAY" )
				{
					bool valid = propValue.isArray
						&& (int)propValue.oval->size() < (int)attrVal;

					if( !valid )
					{
						propValue.err( name + " element count >= than xmax " + (string)attrVal );
					}
				}
				else
				{
					bool valid = true;

					if( type == "INT" )
					{
						valid = (int)propValue < (int)attrVal;
					}
					else if( type == "FLOAT" )
					{
						valid = (float)propValue < (float)attrVal;
					}
					else
					{
						propSchema.err( string("'xmax' is not valid for type ") + type );
					}

					if( !valid )
					{
						propValue.err( (string)propValue + " >= xmax " + (string)attrVal );
					}
				}
			}

			// --
			// -- element
			// --
			else if( attrName == "element" )
			{
				if( type != "ARRAY" )
				{
					attrVal.err( "'element' only valid for ARRAY" );
				}

				itfor( Property::PropertyMap, *(propValue.oval), itelem )
				{
					Property *elementValue = itelem->second;

					validateProperty( attrVal, *elementValue );
				}

			}

			// --
			// -- values & scalar
			// --
			else if( attrName == "values" || attrName == "scalar" )
			{
				// These attributes were handled prior to this loop.
				if( type != "ENUM" )
				{
					attrVal.err( string("Invalid schema attribute for non-ENUM type.") );
				}
			}

			// --
			// -- properties
			// --
			else if( attrName == "properties" )
			{
				if( type != "OBJECT" )
				{
					attrVal.err( "'properties' only valid for OBJECT type." );
				}

				validateChildren( attrVal, propValue );
			}

			// --
			// -- Invalid attr
			// --
			else if( (attrName != "type") && (attrName != "default") )
			{
				attrVal.err( string("Invalid schema attribute '") + attrName + "'" );
			}
		}
	}
}

#ifdef PROPDEV
using namespace PropertyFile;

int main( int argc, char **argv )
{
	Document *docValues = Parser::parse( "values.txt" );
	Document *docSchema = Parser::parse( "schema.txt" );

	Schema::apply( docSchema, docValues );

	docValues->dump( cout );

	delete docValues;
	delete docSchema;
	
	return 0;
}
#endif
