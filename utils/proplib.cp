#include "proplib.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fstream>
#include <iostream>

#include "misc.h"

using namespace std;

#define DEBUG_INPUT false
#define DEBUG_EVAL false

namespace proplib
{
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- FUNCTION _strndup()
	// ---
	// --- Mac doesn't have strndup
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	static char* _strndup(const char *s, size_t n)
	{
		n = min( n, strlen(s) );
		char *result = (char *)malloc( n + 1 );
		memcpy( result, s, n );
		result[ n ] = '\0';
		return result;
	}

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- FUNCTION eval()
	// ---
	// --- Use python to evaluate an expression
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	typedef map<string,string> SymbolTable;

	static bool eval( const char *expr,
					  SymbolTable &symbols,
					  char *result, size_t result_size )
	{
		// build a dict() mapping symbol name to value
		string locals = "{";
		itfor(SymbolTable, symbols, it )
		{
			string value = it->second;
			if( !Parser::parseInt(value)
				&& !Parser::parseFloat(value)
				&& !Parser::parseBool(value) )
			{
				value = string("'") + value + "'";
			}
			locals.append( "'" + it->first + "': " + value + ",");
		}
		locals.append( "}" );

		char script[1024 * 4];
		sprintf( script,
				 "import sys\n"
				 "try:\n"
				 "  print eval('%s', %s)\n"
				 "  sys.stdout.flush()\n"
				 "except:\n"
				 "  print sys.exc_info()[1]\n"
				 "  sys.stdout.flush()\n"
				 "  sys.exit(1)\n",
				 expr,
				 locals.c_str());

		char cmd[1024 * 4];
		sprintf( cmd, "python -c \"%s\"", script );

#if DEBUG_EVAL
		cout << "<EVAL>" << endl;
		cout << script << endl;
		cout << "</EVAL>" << endl;
#endif

		FILE *f = popen( cmd, "r" );

		char *result_orig = result;

		size_t n;
		while( !feof(f) && (result_size > 0) )
		{
			n = fread(result, 1, result_size, f);
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

	Identifier::Identifier( const char *name)
	{
		this->name = name;
	}

	Identifier::Identifier( int index )
	{
		char buf[32];
		sprintf( buf, "%d", index );
		this->name = buf;
	}

	Identifier::Identifier( size_t index )
	{
		char buf[32];
		sprintf( buf, "%lu", index );
		this->name = buf;
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
		fprintf( stderr, "%s:%u: ERROR! %s\n", doc->getName(), lineno, msg.c_str() );
		exit( 1 );
	}

	void DocumentLocation::warn( string msg )
	{
		fprintf( stderr, "%s:%u: WARNING! %s\n", doc->getName(), lineno, msg.c_str() );
	}

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS Node
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	Node::Node( DocumentLocation _loc,
				Type _type )
		: loc( _loc )
		, type( _type )
	{
	}

	Node::~Node()
	{
	}

	Node::Type Node::getNodeType()
	{
		return type;
	}

	DocumentLocation &Node::getLocation()
	{
		return loc;
	}

	bool Node::isProp()
	{
		return type == PROPERTY;
	}

	bool Node::isCond()
	{
		return type == CONDITION;
	}

	bool Node::isClause()
	{
		return type == CLAUSE;
	}

	Property *Node::toProp()
	{
		assert( isProp() );

		return (Property *)this;
	}

	Condition *Node::toCond()
	{
		assert( isCond() );

		return (Condition *)this;
	}

	Clause *Node::toClause()
	{
		assert( isClause() );

		return (Clause *)this;
	}

	void Node::err( string message )
	{
		loc.err( message );
	}

	void Node::warn( string message )
	{
		loc.warn( message );
	}

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS Property
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------

	Property::Property( DocumentLocation _loc, Identifier _id, bool isArray )
		: Node( _loc, Node::PROPERTY )
		, parent( NULL )
		, symbolSource( NULL )
		, id( _id )
		, _isArray( isArray )
		, isExpr( false )
		, isEvaling( false )
	{
		type =  CONTAINER;
		oval.props = new PropertyMap();
		oval.conds = new ConditionList();
	}

	Property::Property( DocumentLocation _loc, Identifier _id, const char *val )
		: Node( _loc, Node::PROPERTY )
		, parent( NULL )
		, symbolSource( NULL )
		, id( _id )
		, _isArray( false )
		, isEvaling( false )
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

			sval = _strndup( val, n );
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
		case CONTAINER:
			itfor( PropertyMap, *oval.props, it )
			{
				delete it->second;
			}
			delete oval.props;

			itfor( ConditionList, *oval.conds, it )
			{
				delete *it;
			}
			delete oval.conds;
			break;
		case SCALAR:
			free( sval );
			break;
		default:
			assert( false );
		}
	}

	Property *Property::clone()
	{
		Property *clone = NULL;

		switch( type )
		{
		case SCALAR:
			clone = new Property( loc, id, sval );
			// we have to set 'isExpr' because $() is stripped
			clone->isExpr = isExpr;
			break;
		case CONTAINER:
			clone = new Property( loc, id, _isArray );

			itfor( PropertyMap, props(), it )
			{
				clone->add( it->second->clone() );
			}

			itfor( ConditionList, conds(), it )
			{
				clone->add( (*it)->clone() );
			}
			break;
		default:
			assert( false );
		}

		return clone;
	}

	bool Property::equals( Property *other )
	{
		if( isContainer() )
		{
			assert( conds().size() == 0 );

			if( props().size() != other->props().size() )
			{
				return false;
			}

			itfor( PropertyMap, props(), it )
			{
				Property *otherChild = other->getp( it->first );
				if( otherChild == NULL )
				{
					return false;
				}

				Property *child = it->second;
				if( !child->equals(otherChild) )
				{
					return false;
				}
			}

			return true;
		}
		else
		{
			return Parser::scalarValuesEqual( this->evalScalar(), other->evalScalar() );
		}
	}

	const char *Property::getName()
	{
		return id.getName();
	}

	std::string Property::getFullName( int minDepth )
	{
		if( getDepth() < minDepth )
		{
			return "";
		}
		else if( parent == NULL )
		{
			return getName();
		}
		else
		{
			string parentFullName = parent->getFullName( minDepth );
			if( parentFullName == "" )
			{
				return getName();
			}
			else
			{
				if( parent->isArray() )
					return parentFullName + "[" + getName() + "]";
				else
					return parentFullName + "." + getName();
			}
		}
	}

	bool Property::isContainer()
	{
		return type == CONTAINER;
	}

	bool Property::isArray()
	{
		return isContainer() && _isArray;
	}

	bool Property::isScalar()
	{
		return type == SCALAR;
	}

	Property &Property::get( Identifier id )
	{
		if( !isContainer() )
		{
			err( string("Not a property container, cannot contain ") + id.getName() );
		}

		Property *prop = getp( id );

		if( prop == NULL )
		{
			string name;
			if( parent != NULL && parent->isArray() )
			{
				name = string(parent->getName()) + "[" + getName() + "]";
			}
			else
			{
				name = getName();
			}

			err( string("Expecting property '") + id.getName() + "' for '" + name + "'");
		}

		return *prop;
	}

	Property *Property::getp( Identifier id )
	{
		assert( type == CONTAINER );

		PropertyMap::iterator it = props().find( id );
		if( it != props().end() )
		{
			return it->second;
		}
		else
		{
			return NULL;
		}
	}

	PropertyMap &Property::props()
	{
		assert( type == CONTAINER );

		return *oval.props;
	}

	PropertyMap &Property::elements()
	{
		assert( isArray() );

		return props();
	}

	size_t Property::size()
	{
		assert( isContainer() );

		return props().size();
	}

	ConditionList &Property::conds()
	{
		assert( type == CONTAINER );

		return *oval.conds;
	}

	Property &Property::replaceScalar( const string &newValue )
	{
		return parent->replaceScalar( this, newValue );
	}

	Property &Property::replaceScalar( Property *oldChild, const string &newValue )
	{
		assert( isContainer() );
		assert( oldChild->isScalar() );
		assert( oldChild->parent == this );
		
		// TODO: give a reasonable location indicating it was set.
		Property *newChild = new Property( oldChild->loc, oldChild->id, newValue.c_str() );

		props().erase( oldChild->id );
		add( newChild );

		return *newChild;
	}

	void Property::replace( PropertyMap &newprops, bool isArray )
	{
		assert( type == CONTAINER );

		itfor( PropertyMap, props(), it )
		{
			delete it->second;
		}
		props().clear();

		_isArray = isArray;

		itfor( PropertyMap, newprops, it )
		{
			add( it->second );
		}
	}

	Property::operator short()
	{
		return (int)*this;
	}

	Property::operator int()
	{
		int ival;

		if( (type != SCALAR) || !Parser::parseInt(evalScalar(), &ival) )
		{
			err( string("Expecting INT value for property '") + getName() + "'." );
		}

		return ival;
	}

	Property::operator long()
	{
		return (int)*this;
	}

	Property::operator float()
	{
		float fval;

		if( (type != SCALAR) || !Parser::parseFloat(evalScalar(), &fval) )
		{
			err( string("Expecting FLOAT value for property '") + getName() + "'." );
		}

		return fval;
	}

	Property::operator bool()
	{
		bool bval;

		if( (type != SCALAR) || !Parser::parseBool(evalScalar(), &bval) )
		{
			err( string("Expecting BOOL value for property '") + getName() + "'." );
		}

		return bval;
	}

	Property::operator string()
	{
		assert( type == SCALAR );

		return evalScalar();
	}

	string Property::evalScalar()
	{
		if( type != SCALAR )
		{
			loc.err( "Referenced as scalar, probably from expression." );
		}

		if( isExpr )
		{
			// We eventually want to move the isEvaling state out of the object and
			// onto the stack so we are thread safe.
			if( isEvaling )
			{
				loc.err( "Expression dependency cycle." );
			}
			isEvaling = true;

			SymbolTable symbolTable;

			StringList ids;
			Parser::scanIdentifiers( sval, ids );

			itfor( StringList, ids, it )
			{
				Property *prop = findSymbol( *it );
				if( prop != NULL )
				{
					if( prop->isScalar() )
					{
						symbolTable[ prop->getName() ] = prop->evalScalar();
					}
				}
			}

			char buf[1024];

			bool success = eval( sval, symbolTable,
								 buf, sizeof(buf) );

			if( !success )
			{
				loc.err( buf );
			}

			isEvaling = false;

			return buf;
		}
		else
		{
			return sval;
		}
	}

	void Property::add( Node *node )
	{
		assert( type == CONTAINER );

		switch( node->getNodeType() )
		{
		case PROPERTY:
			{
				Property *prop = node->toProp();
				Property *prev = props()[ prop->id ];
				if( prev != NULL )
				{
					prop->err( string("Duplicate definition. See ") + prev->loc.getDescription() );
				}
				prop->parent = this;

				props()[ prop->id ] = prop;
			}
			break;
		case CONDITION:
			{
				Condition *cond = node->toCond();
				
				conds().push_back( cond );
			}
			break;
		default:
			assert( false );
		}
	}

	void Property::dump( ostream &out, const char *indent )
	{
		out << loc.getDescription() << " ";
		out << indent << getName();

		if( type == CONTAINER )
		{
			if( _isArray )
			{
				out << " [";
			}
			out << endl;
			itfor( PropertyMap, props(), it )
			{
				it->second->dump( out, (string(indent) + "  ").c_str() );
			}
			itfor( ConditionList, conds(), it )
			{
				(*it)->dump( out, (string(indent) + "  ").c_str() );
			}
			if( _isArray )
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

	void Property::write( ostream &out, const char *indent )
	{
		out << indent << getName();

		if( type == CONTAINER )
		{
			if( _isArray )
			{
				out << " [" << endl;

				if( props().size() > 0 )
				{
					if( props()[0]->isScalar() )
					{
						string _indent = string(indent) + "  ";

						itfor( PropertyMap, props(), it )
						{
							out << _indent << it->second->getDecoratedScalar() << endl;
						}
					}
					else
					{
						PropertyMap::iterator itlast = --props().end();
						itfor( PropertyMap, props(), itObjectElement )
						{
							Property *objectElement = itObjectElement->second;
							itfor( PropertyMap, objectElement->props(), it )
							{
								it->second->write( out, (string(indent) + "  ").c_str() );
							}

							if( itObjectElement != itlast )
							{
								out << indent << "," << endl;
							}
						}
						
					}
				}

				out << indent << "]" << endl;
			}
			else
			{
				out << " {" << endl;

				itfor( PropertyMap, props(), it )
				{
					it->second->write( out, (string(indent) + "  ").c_str() );
				}

				out << indent << "}" << endl;
			}
		}
		else
		{
			out << " " << getDecoratedScalar() << endl;
		}
	}

	void Property::writeScalarNames( ostream &out, int depth_start )
	{
		if( isScalar() )
		{
			string fullName = getFullName( depth_start );
			if( ! fullName.empty() )
				out << fullName << endl;
		}
		else
		{
			itfor( PropertyMap, props(), it )
			{
				it->second->writeScalarNames( out, depth_start );
			}
		}
	}

	void Property::resolve( StringList::iterator curr, StringList::iterator end, PropertyList &result )
	{
		if( curr == end )
		{
			result.push_back( this );
		}
		else
		{
			string &childName = *curr;
			curr++;

			if( childName == "*" )
			{
				if( !isArray() )
				{
					err( "'*' only legal for array index." );
				}

				itfor( PropertyMap, props(), it )
				{
					it->second->resolve( curr, end, result );
				}
			}
			else
			{
				Property &propChild = get( childName );

				propChild.resolve( curr, end, result );
			}
		}
	}

	string Property::getDecoratedScalar()
	{
		assert( isScalar() );

		if( isExpr )
		{
			return string(" $( ") + sval + " )";
		}
		else
		{
			return sval;
		}
	}

	Property *Property::findSymbol( Identifier id )
	{
		if( symbolSource != NULL )
		{
			return symbolSource->findSymbol( id );
		}
		else
		{
			Property *result = NULL;

			if( type == CONTAINER )
			{
				result = getp( id );
			}

			if( result == NULL && parent != NULL )
			{
				result = parent->findSymbol( id );
			}

			return result;
		}
	}

	int Property::getDepth()
	{
		if( parent == NULL )
			return 0;
		else
			return parent->getDepth() + 1;
	}

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS Condition
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	Condition::Condition( DocumentLocation _loc )
		: Node( _loc, Node::CONDITION )
	{
	}

	Condition::~Condition()
	{
		itfor( ClauseList, clauses, it )
		{
			delete *it;
		}
	}

	Condition *Condition::clone()
	{
		Condition *clone = new Condition( loc );

		itfor( ClauseList, clauses, it )
		{
			Clause *clause = *it;

			clone->clauses.push_back( clause->clone() );
		}

		return clone;
	}

	Clause *Condition::selectClause( Property *symbolSource )
	{
		itfor( ClauseList, clauses, it )
		{
			Clause *clause = *it;

			if( clause->evalExpr(symbolSource) )
			{
				return clause;
			}
		}

		return NULL;
	}

	void Condition::add( Node *node )
	{
		if( node->isClause() )
		{
			Clause *clause = node->toClause();

			if( clauses.size() == 0 )
			{
				if( !clause->isIf() )
				{
					clause->err( "Expecting 'if'" );
				}

				clauses.push_back( clause );
			}
			else
			{
				if( clauses.back()->isElse() )
				{
					clause->err( "Cannot follow 'else'" ); 
				}
				else if( clause->isIf() )
				{
					clause->err( "Unexpected 'if'" );
				}
			
				clauses.push_back( clause );
			}
		}
		else
		{
			assert( clauses.size() > 0 );

			clauses.back()->add( node );
		}
	}

	void Condition::dump( ostream &out, const char *indent )
	{
		out << loc.getDescription() << " ";
		out << indent << "<CONDITION>" << endl;

		itfor( ClauseList, clauses, it )
		{
			(*it)->dump( out, (string(indent) + "  ").c_str() );
		}

		out << loc.getDescription() << " ";
		out << indent << "</CONDITION>" << endl;
	}

	void Condition::write( ostream &out, const char *indent )
	{
		assert( false ); // not yet supported
	}

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS Clause
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	
	Clause::Clause( DocumentLocation _loc, Type _type, string _expr )
		: Node( _loc, Node::CLAUSE )
		, type( _type )
	{
		exprProp = new Property( _loc,
								 string("Expr-")+loc.getDescription(),
								 _expr.c_str() );
		bodyProp = new Property( _loc,
								 string("Body-")+loc.getDescription() );
	}

	// private constructor for clone()
	Clause::Clause( DocumentLocation _loc )
		: Node( _loc, Node::CLAUSE )
	{
	}

	Clause::~Clause()
	{
		delete exprProp;
		delete bodyProp;
	}

	Clause *Clause::clone()
	{
		Clause *clone = new Clause( loc );
		clone->type = type;
		clone->exprProp = exprProp->clone();
		clone->bodyProp = bodyProp->clone();
		return clone;
	}

	bool Clause::isIf()
	{
		return type == IF;
	}

	bool Clause::isElif()
	{
		return type == ELIF;
	}

	bool Clause::isElse()
	{
		return type == ELSE;
	}

	bool Clause::evalExpr( Property *symbolSource )
	{
		exprProp->symbolSource = symbolSource;

		return (bool)*exprProp;
	}

	void Clause::add( Node *node )
	{
		bodyProp->add( node );
	}

	void Clause::dump( ostream &out, const char *indent )
	{
		out << loc.getDescription() << " ";
		out << indent << "<CLAUSE " << exprProp->sval << ">" << endl;

		bodyProp->dump( out, (string(indent) + "  ").c_str() );

		out << loc.getDescription() << " ";
		out << indent << "</CLAUSE>" << endl;
	}

	void Clause::write( ostream &out, const char *indent )
	{
		assert( false ); // not yet supported
	}

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS Document
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------

	Document::Document( const char *name, const char *path )
		: Property( DocumentLocation(this,0), string(name) )
	{
		this->path = path;
	}

	Document::~Document()
	{
	}

	string Document::getPath()
	{
		return path;
	}

	void Document::set( StringList &propertyPath, const string &value )
	{
		PropertyList props;
		resolve( propertyPath.begin(), propertyPath.end(), props );
		itfor( PropertyList, props, it )
		{
			(*it)->replaceScalar( value );
		}
	}

	void Document::write( ostream &out, const char *indent )
	{
		itfor( PropertyMap, props(), it )
		{
			it->second->write( out, indent );
		}
	}

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS Parser
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	
	Document *Parser::parseFile( const char *path )
	{
		Document *doc = new Document( path, path );
		DocumentLocation loc( doc, 0 );
		NodeStack nodeStack;

		nodeStack.push( doc );

		ifstream in( path );
		char *line;
		int inMultiLineComment = 0;

		while( (line = readline(in,
								loc,
								inMultiLineComment)) != NULL )
		{
#if DEBUG_INPUT
			cout << "readline='" << line << "'" << endl;
#endif

			CStringList tokens;
			tokenize( loc, line, tokens );

			processLine( doc,
						 loc,
						 nodeStack,
						 tokens );

			itfor( CStringList, tokens, it )
			{
				free( *it );
			}
			tokens.clear();
			free( line );
		}

		if( nodeStack.size() > 1 )
		{
			loc.err( "Unexpected end of document. Likely missing '}' or ']'" );
		}

		return doc;
	}

	bool Parser::isValidIdentifier( const string &_text )
	{
		bool valid = false;

		if( (_text != "if") && (_text != "else") && (_text != "elif") )
		{
			const char *text = _text.c_str();
			if( isalpha(*text) )
			{
				valid = true;

				for( const char *c = text; *c && valid; c++ )
				{
					valid = isalnum(*c);
				}
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

	StringList Parser::parsePropertyPath( const string &path )
	{
		StringList result;

		char copy[1024];
		assert( path.length() < sizeof(copy) );
		strcpy( copy, path.c_str() );

		for( char *str = copy, *token, *save; 
			 (token = strtok_r(str, ".[]", &save)) != NULL;
			 str = NULL )
		{
			result.push_back( token );
		}

		return result;
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

	bool Parser::scalarValuesEqual( const string &a, const string &b )
	{
		if( a == b )
			return true;

		{
			float x, y;

			if( parseFloat(a, &x) && parseFloat(b, &y) )
			{
				return x == y;
			}
		}

		return false;
	}

	char *Parser::readline( istream &in,
							DocumentLocation &loc,
							int &inMultiLineComment )
	{
		char buf[1024 * 4];
		char *result = NULL;

		do
		{
			in.getline( buf, sizeof(buf) );
			if( !in.good() )
			{
				if( in.eof() )
				{
					if( in.gcount() == 0 )
						break;
				}
				else
					loc.err( "Failed reading file!" );
			}

#if DEBUG_INPUT
			cout << "getline='" << buf << "'" << endl;
#endif

			char *line = stripComments( buf, inMultiLineComment );

			loc.lineno++;
			
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

	char *Parser::stripComments( char *line,
								 int &inMultiLineComment )
	{
		// process multi-line comments
		{
			typedef list< pair<int,int> > SegmentList;

			SegmentList contentSegments;
			char prevChar = '\0';
			int index;
			int contentStart = inMultiLineComment == 0 ? 0 : -1;
				
			for( index = 0; line[index] != '\0'; index++ )
			{
				char c = line[index];

				if( (prevChar == '#') && (c == '*') )
				{
					inMultiLineComment++;
					if( inMultiLineComment == 1 )
					{
						int contentEnd = index - 2;

						if( (contentStart > -1) && (contentEnd >= contentStart) )
							contentSegments.push_back( make_pair(contentStart, contentEnd) );

						contentStart = -1;
					}
				}
				else if( (prevChar == '*') && (c == '#') )
				{
					inMultiLineComment--;
					if( inMultiLineComment == 0 )
						contentStart = index + 1;
				}

				prevChar = c;
			}

			if( contentStart > -1 )
				contentSegments.push_back( make_pair(contentStart, index) );

			char *tail = line;

			itfor( SegmentList, contentSegments, it )
			{
				int n = (it->second - it->first) + 1;
				char segment[n];
				memcpy( segment, line + it->first, n );
				memcpy( tail, segment, n );
				tail += n;
			}

			*tail = '\0';
		}

		// remove single-line comments
		{
			char *comment = strchr( line, '#' );
			if( comment != NULL )
			{
				*comment = '\0';
			}
		}

		return line;
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
			list.push_back( _strndup(start, (END) - start) );	\
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
							  NodeStack &nodeStack,
							  CStringList &tokens )
	{
		size_t ntokens = tokens.size();
		string nameToken = tokens.front();
		string valueToken = tokens.back();
		

		if( valueToken == "]" )
		{
			if( !nodeStack.top()->isProp() )
			{
				loc.err( "Expecting ']' to terminate property, not condition" );
			}
			else if( ntokens != 1 )
			{
				loc.err( "Expecting only ']'" );
			}
			if( nodeStack.size() < 2 )
			{
				loc.err( "Extraneous ']'" );
			}
			
			if( !nodeStack.top()->toProp()->isArray() )
			{
				// close out the object for the last element.
				nodeStack.pop();

				if( !nodeStack.top()->toProp()->isArray() )
				{
					loc.err( "Unexpected ']'." );
				}

				if( nodeStack.size() < 2 )
				{
					loc.err( "Extraneous ']'" );
				}
			}
			else
			{
				Property *propArray = nodeStack.top()->toProp();
				int nelements = (int)propArray->props().size();

				if( nelements > 0 )
				{
					if( propArray->get(0).isContainer() )
					{
						// we must have had a hanging ','. We force an empty object as a final element.
						propArray->add( new Property( loc, nelements ) );
					}
				}
			}

			nodeStack.pop();
		}
		else
		{
			bool isScalarArray = false;

			if( nodeStack.top()->isProp()
				&& nodeStack.top()->toProp()->isArray() )
			{
				Property *propArray = nodeStack.top()->toProp();

				// If this is the first element.
				if( propArray->props().size() == 0 )
				{
					if( (ntokens == 1) && (valueToken != ",") )
					{
						isScalarArray = true;
					}
				}
				else
				{
					isScalarArray = propArray->get(0).isScalar();
				}

				if( !isScalarArray )
				{
					// we need a container for the coming element.
					size_t index = nodeStack.top()->toProp()->props().size();
					Property *prop = new Property( loc, index );
					nodeStack.top()->add( prop );
					nodeStack.push( prop );
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

				size_t index = nodeStack.top()->toProp()->props().size();
				nodeStack.top()->add( new Property(loc, index, valueToken.c_str()) );
			}
			else
			{
				if( valueToken == "{" )
				{
					if( nameToken == "if" )
					{
						if( ntokens != 3 )
						{
							loc.err( "Expecting 'if EXPR {'" );
						}

						Condition *cond = new Condition( loc );
						nodeStack.top()->add( cond );
						nodeStack.push( cond );

						Clause *clause = new Clause( loc, Clause::IF, tokens[1] );
						cond->add( clause );
					}
					else if( nameToken == "}" )
					{
						string clauseType = tokens[1];

						if( clauseType == "elif" )
						{
							if( ntokens != 4 )
							{
								loc.err( "Expecting '} elif EXPR {'" );
							}
							if( !nodeStack.top()->isCond() )
							{
								loc.err( "Unexpected 'elif'" );
							}

							Clause *clause = new Clause( loc, Clause::ELIF, tokens[2] );
							nodeStack.top()->add( clause );
						}
						else if( clauseType == "else" )
						{
							if( ntokens != 3 )
							{
								loc.err( "Expecting '} else {'" );
							}
							if( !nodeStack.top()->isCond() )
							{
								loc.err( "Unexpected 'else'" );
							}

							Clause *clause = new Clause( loc, Clause::ELSE );
							nodeStack.top()->add( clause );
						}
						else
						{
							loc.err( string("Unexpected '") + tokens[1] + "'" );
						}
					}
					else
					{
						if( (ntokens != 2) || !isValidIdentifier(nameToken) )
						{
							loc.err( "Expecting 'NAME {'" );
						}

						string &objectName = nameToken;
			
						Property *prop = new Property( loc, objectName );
						nodeStack.top()->add( prop );
						nodeStack.push( prop );
					}
				}
				else if( valueToken == "}" )
				{
					if( ntokens != 1 )
					{
						loc.err( "Expecting '}' only." );
					}

					if( nodeStack.size() < 2 )
					{
						loc.err( "Extraneous '}'" );
					}

					nodeStack.pop();
				}
				else if( valueToken == "[" )
				{
					if( ntokens != 2 )
					{
						loc.err( "Expecting 'NAME ['" );
					}

					string &arrayName = nameToken;
			
					Property *propArray = new Property( loc, arrayName, true );
					nodeStack.top()->add( propArray );
					nodeStack.push( propArray );
				}
				else if( valueToken == "," )
				{
					if( ntokens != 1 )
					{
						loc.err( "Expecting comma only." );
					}

					nodeStack.pop();
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
			
					nodeStack.top()->add( prop );
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
		normalize( *docSchema, *docValues, isLegacyMode(docValues) );
		validateChildren( *docSchema, *docValues );
	}

	void Schema::reduce( Document *docSchema, Document *docValues, bool apply )
	{
		if( apply )
			Schema::apply( docSchema, docValues );
		reduceChildren( *docSchema, *docValues, isLegacyMode(docValues) );
	}

	void Schema::normalize( Property &propSchema, Property &propValue, bool legacyMode )
	{
		assert( propSchema.isContainer() );

		injectDefaults( propSchema, propValue, legacyMode );

		itfor( ConditionList, propSchema.conds(), it )
		{
			Condition *cond = *it;

			Clause *clause = cond->selectClause( &propValue );
			if( clause )
			{
				Property *bodyProp = clause->bodyProp;
				clause->bodyProp = NULL;
				itfor( PropertyMap, bodyProp->props(), itprop )
				{
					propSchema.add( itprop->second );
				}
				bodyProp->props().clear();

				itfor( ConditionList, bodyProp->conds(), itcond )
				{
					propSchema.add( *itcond );
				}
				bodyProp->conds().clear();

				injectDefaults( propSchema, propValue, legacyMode );
			}
		}

		itfor( ConditionList, propSchema.conds(), it )
		{
			delete *it;
		}
		propSchema.conds().clear();

		itfor( PropertyMap, propSchema.props(), it )
		{
			Property *childSchema = it->second;
			string childSchemaName = childSchema->getName();

			if( !childSchema->isScalar() )
			{
				if( childSchemaName == "element"  )
				{
					assert( string(propSchema.getName()) == propValue.getName() );

					if( !propValue.isArray() )
					{
						propValue.err( "Expecting ARRAY" );
					}

					PropertyMap schemaElements;

					itfor( PropertyMap, propValue.props(), itelem )
					{
						Property *elementValue = itelem->second;
						Property *elementSchema = childSchema->clone();

						normalize( *elementSchema, *elementValue, legacyMode );

						elementSchema->id = elementValue->id;
						schemaElements[ elementValue->id ] = elementSchema;
					}

					childSchema->replace( schemaElements, true );
				}
				else if( childSchemaName != "default" && childSchemaName != "values" )
				{
					Property *childValue = NULL;
					if( !propValue.isScalar() )
					{
						childValue = propValue.getp( childSchema->id );
					}
					if( childValue == NULL )
					{
						childValue = &propValue;
					}

					normalize( *childSchema, *childValue, legacyMode );
				}
			}
		}
	}

	void Schema::injectDefaults( Property &propSchema, Property &propValue, bool legacyMode )
	{
		assert( propSchema.isContainer() );

		// need to be within <root>, properties, element
		if( propSchema.getp( "type" ) != NULL )
		{
			return;
		}

		if( !propValue.isContainer() )
		{
			propValue.err( "Expecting OBJECT" );
		}

		itfor( PropertyMap, propSchema.props(), it )
		{
			Property *childSchema = it->second;
			if( !childSchema->isContainer() )
			{
				childSchema->err( string("Unexpected attribute for ") + propSchema.getName() );
			}

			Property *childValue = propValue.getp( childSchema->id );

			if( childValue == NULL )
			{
				Property *propDefault = getDefaultProperty( childSchema, legacyMode );

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
		assert( propSchema.isContainer() );
		assert( propValue.isContainer() );

		if( propValue.conds().size() > 0 )
		{
			propValue.conds().front()->err( "'if' only legal in schema files." );
		}

		itfor( PropertyMap, propSchema.props(), it )
		{
			Property *childSchema = it->second;

			if( !childSchema->isContainer() )
			{
				childSchema->err( "Unexpected attribute." );
			}
		}

		itfor( PropertyMap, propValue.props(), it )
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
		if( propValue.isContainer() && (propValue.conds().size() > 0) )
		{
			propValue.conds().front()->err( "'if' only legal in schema files." );
		}

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
		else if( type == "STRING" )
		{
			(string)propValue;
		}
		else if( type == "ARRAY" )
		{
			if( !propValue.isArray() )
			{
				propValue.err( string("Expecting ARRAY value for property '") + propValue.getName() + "'" );
			}

			// verify required attributes exist
			propSchema.get( "element" );
		}
		else if( type == "OBJECT" )
		{
			if( !propValue.isContainer() || propValue.isArray() )
			{
				propValue.err( string("Expecting OBJECT value for property '") + propValue.getName() + "'" );
			}

			// verify required attributes exist
			propSchema.get( "properties" );
		}
		else if( type == "ENUM" )
		{
			Property &propEnumValues = propSchema.get( "values" );
			if( !propEnumValues.isArray() )
			{
				propEnumValues.err( "Expecting array of enum values." );
			}

			bool valid = false;

			itfor( PropertyMap, propEnumValues.props(), it )
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
					if( scalarType == "INT"
						|| scalarType == "FLOAT"
						|| scalarType == "BOOL"
						|| scalarType == "STRING" )
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
		itfor( PropertyMap, propSchema.props(), it )
		{
			string attrName = it->first.getName();
			Property &attrVal = *(it->second);

			attrVal.symbolSource = &propValue;

			// --
			// -- CONSTRAINT
			// --
#define CONSTRAINT( NAME, OP, ERRDESC )									\
			if( type == "ARRAY" )										\
			{															\
				bool valid = propValue.isArray()						\
					&& (int)propValue.props().size() OP (int)attrVal;	\
																		\
				if( !valid )											\
				{														\
					propValue.err( name + " element count "ERRDESC" "NAME" " + (string)attrVal ); \
				}														\
			}															\
			else														\
			{															\
				bool valid = true;										\
																		\
				if( type == "INT" )										\
				{														\
					valid = (int)propValue OP (int)attrVal;				\
				}														\
				else if( type == "FLOAT" )								\
				{														\
					valid = (float)propValue OP (float)attrVal;			\
				}														\
				else													\
				{														\
					propSchema.err( string("'" NAME "' is not valid for type ") + type ); \
				}														\
																		\
				if( !valid )											\
				{														\
					propValue.err( (string)propValue + " "ERRDESC" "NAME" " + (string)attrVal ); \
				}														\
			}

			// --
			// -- min
			// --
			if( attrName == "min" )
			{
				CONSTRAINT( "min", >=, "<" );
			}

			// --
			// -- exmin
			// --
			else if( attrName == "exmin" )
			{
				CONSTRAINT( "exmin", >, "<=" );
			}

			// --
			// -- max
			// --
			else if( attrName == "max" )
			{
				CONSTRAINT( "max", <=, ">" );
			}

			// --
			// -- exmax
			// --
			else if( attrName == "exmax" )
			{
				CONSTRAINT( "exmax", <, ">=" );
			}

#undef CONSTRAINT

			// --
			// -- assert
			// --
			else if( attrName == "assert" )
			{
				bool result = (bool)attrVal;

				if( !result )
				{
					propValue.err( string("Failed assertion '") + (string)attrVal + "' at " + attrVal.loc.getDescription());
				}
			}

			// --
			// -- warn
			// --
			else if( attrName == "warn" )
			{
				propValue.warn( (string)attrVal );
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

				assert( attrVal.isArray() );
				assert( propValue.isArray() );

				itfor( PropertyMap, propValue.props(), itelem )
				{
					Property *elementValue = itelem->second;

					validateProperty( attrVal.get(elementValue->getName()), *elementValue );
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
			else if( (attrName != "type") && (attrName != "default") && (attrName != "legacy") )
			{
				attrVal.err( string("Invalid schema attribute '") + attrName + "'" );
			}
		}
	}

	bool Schema::reduceChildren( Property &propSchema, Property &propValue, bool legacyMode )
	{
		assert( propSchema.isContainer() );
		assert( propValue.isContainer() );

		bool reduced = true;
		PropertyList reducedProperties;

		itfor( PropertyMap, propValue.props(), it )
		{
			Property *childValue = it->second;
			Property *childSchema = propSchema.getp( childValue->id );

			// it has to be non-null because we apply() prior to reduce()
			assert( childSchema );

			if( reduceProperty(*childSchema, *childValue, legacyMode) )
			{
				reducedProperties.push_back( childValue );
			}
			else
			{
				reduced = false;
			}
		}

		itfor( PropertyList, reducedProperties, it )
		{
			propValue.props().erase( (*it)->id );
			delete *it;
		}

		return reduced;
	}

	bool Schema::reduceProperty(  Property &propSchema, Property &propValue, bool legacyMode )
	{
		bool reduced = false;
		Property *propDefault = getDefaultProperty( &propSchema, legacyMode );

		if( propDefault && propDefault->equals(&propValue) )
		{
			reduced = true;
		}
		else if( !propValue.isScalar() )
		{
			if( propValue.isArray() )
			{
				if( (propValue.size() > 0) && !propValue.get(0).isScalar() )
				{
					// it's an array of objects. it doesn't match the default, but we
					// need to try to reduce individual properties, which could even
					// reduce the whole array.
					reduced = true;
				
					Property &schemaElements = propSchema.get( "element" );
					for( size_t i = 0, n = propValue.size(); i < n; i++ )
					{
						if( !reduceChildren(schemaElements.get(i).get( "properties" ),
											propValue.get(i),
											legacyMode) )
						{
							reduced = false;
						}
					}
				}
			}
			else
			{
				// it's an object
				reduced = reduceChildren( propSchema.get("properties"), propValue, legacyMode );
			}
		}

		return reduced;
	}

	bool Schema::isLegacyMode( Document *docValues )
	{
		bool legacyMode = false;
		Property *propLegacyMode = docValues->getp( "LegacyMode" );
		if( propLegacyMode != NULL )
		{
			legacyMode = (bool)*propLegacyMode;
		}
		return legacyMode;
	}

	Property *Schema::getDefaultProperty( Property *propSchema, bool legacyMode )
	{
		Property *propDefault = NULL;

		if( legacyMode )
		{
			propDefault = propSchema->getp( "legacy" );
		}
		if( propDefault == NULL )
		{
			propDefault = propSchema->getp( "default" );
		}

		return propDefault;
	}

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS Overlay
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------

	bool Overlay::hasOverlay( Document *docOverlay, const string &selector )
	{
		return findOverlay( docOverlay, selector ) != NULL;
	}

	void Overlay::overlay( Document *docOverlay, Document *docValues, const string &selector )
	{
		Property *propOverlay = findOverlay( docOverlay, selector );
		if( propOverlay )
		{
			overlay( propOverlay, docValues->toProp() );
		}
	}

	Property *Overlay::findOverlay( Document *docOverlay, const string &selector )
	{
		validateOverlayDocument( docOverlay );

		itfor( PropertyMap, docOverlay->props(), it )
		{
			if( selector == it->second->getName() )
			{
				return it->second;
			}
		}

		return NULL;
	}

	void Overlay::validateOverlayDocument( Document *docOverlay )
	{
		itfor( PropertyMap, docOverlay->props(), it )
		{
			if( !it->second->isContainer() )
			{
				it->second->err( "Illegal top-level element." );
			}
		}
	}

	void Overlay::overlay( Property *propOverlay, Property *propOriginal )
	{
		assert( propOriginal->isContainer() && propOverlay->isContainer() );

		if( propOverlay->isArray() || propOriginal->isArray() ) { cerr << "array overlay not implemented." << endl; exit(1); }

		itfor( PropertyMap, propOverlay->props(), it )
		{
			Property *subpropOverlay = it->second;
			Property *subpropOriginal = propOriginal->getp( subpropOverlay->getName() );

			if( subpropOverlay->isArray() ) { cerr << "array overlay not implemented." << endl; exit(1); }

			if( subpropOriginal )
			{
				if( subpropOriginal->isArray() ) { cerr << "array overlay not implemented." << endl; exit(1); }

				if( subpropOriginal->isScalar() != subpropOverlay->isScalar() )
				{
					subpropOverlay->err( string("Incompatible with property at ") + subpropOriginal->getLocation().getDescription() );
				}

				if( subpropOverlay->isScalar() )
				{
					propOriginal->replaceScalar( subpropOriginal, (string)*subpropOverlay );
					delete subpropOriginal;
				}
				else
				{
					overlay( subpropOverlay, subpropOriginal );
				}
			}
			else
			{
				propOriginal->add( subpropOverlay->clone() );
			}
		}
	}
}

#ifdef PROPDEV
using namespace proplib;

int main( int argc, char **argv )
{
	if( false )
	{
		Parser::parseFile( "v101.wfs" )->dump( cout );
	}
	else
	{
#if false
		Document *docValues = Parser::parseFile( "values.txt" );
		Document *docSchema = Parser::parseFile( "schema.txt" );
#else
		Document *docValues = Parser::parseFile( "v101.wf" );
		Document *docSchema = Parser::parseFile( "v101.wfs" );
#endif
		Schema::apply( docSchema, docValues );

		docValues->dump( cout );

		delete docValues;
		delete docSchema;
	}

	return 0;
}
#endif
