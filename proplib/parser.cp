#include "parser.h"

#include <assert.h>
#include <stdlib.h>

#include <sstream>

#include "misc.h"

using namespace std;
using namespace proplib;

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// --- CLASS Token
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
#define NEW_TOKEN( TYPE, TEXT ) new Token( Token::TYPE, #TYPE, TEXT )

Token::Token( Type type_, const char *typeName_, const string &text_ )
: type( type_ )
, typeName( typeName_ )
	, text( text_ )
	, next( NULL )
	, prev( NULL )
	, decoration( NULL )
	, lineno( -1 )
	, number( -1 )
{
}

bool Token::isDecoration()
{
	return type == Whitespace || type == Comment;
}

string Token::getDecorationString()
{
	assert( !isDecoration() );

	stringstream out;

	for( Token *dec = decoration; dec; dec = dec->next )
		out << dec->text;

	return out.str();
}

bool Token::hasNewline()
{
	for( Token *dec = decoration; dec; dec = dec->next )
	{
		if( (dec->type == Whitespace) && (dec->text.find('\n') != string::npos) )
			return true;
	}

	return false;
}

string Token::toString( Token *start, Token *end, bool leadingDecoration )
{
	stringstream out;

	for( Token *tok = start; tok != NULL; tok = (tok == end ? NULL : tok->next) )
	{
		if( leadingDecoration || tok != start )
		{
			out << tok->getDecorationString();
		}
		out << tok->text;
	}

	return out.str();
}

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// --- CLASS Tokenizer
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
Tokenizer::Tokenizer( string sourceName, istream *in )
: _sourceName( sourceName )
, _in( in )
, _lineno( 1 )
, _prev( NULL )

{
}

Tokenizer::~Tokenizer()
{
	delete _in;
}

Token *Tokenizer::next()
{
	if( _prev == NULL )
	{
		_prev = NEW_TOKEN( Bof, "" );
		_prev->lineno = 0;
		return _prev;
	}

	Token *decorationHead = NULL;
	Token *decoration = NULL;

	while( true )
	{
		int lineno = _lineno;

		Token *tok = __next();
		tok->lineno = lineno;
		tok->number = ++_number;

		if( tok->isDecoration() )
		{
			if( decoration )
				decoration->next = tok;
			else
				decorationHead = tok;

			tok->prev = decoration;
			decoration = tok;
		}
		else
		{
			tok->decoration = decorationHead;

			tok->prev = _prev;
			if( _prev )
				_prev->next = tok;
			_prev = tok;

			return tok;
		}
	}
}

char Tokenizer::get()
{
	char c = _in->get();
	if( c == '\n' )
		_lineno++;
	return c;
}

bool isspace(char c) {
	if (c == ' ' || c == '\n' || c == '\t' || c == '\r' || c == '\v' || c == '\f')
		return true;
	else
		return false;
}

bool isalpha(char c) {
	if ((c >= 65 && c <= 90) || (c >= 97 && c <= 122))
		return true;
	else
		return false;
}

bool isdigit(char c) {
	if (c >= 48 && c <= 57)
		return true;
	else
		return false;
}

bool isalnum(char c) {
	if (isdigit(c) || isalpha(c))
		return true;
	else
		return false;
}

Token *Tokenizer::__next()
{
	char c = get();

	if( !_in->good() )
	{
		if( _in->eof() )
			return NEW_TOKEN( Eof, "" );
		else
		{
			cerr << "Failed reading from " << _sourceName << endl;
			exit( 1 );
		}
	}

	if( c == '#' )
		return parseComment();
	else if( isspace(c) )
		return parseWhitespace();
	else if( c == '@' )
		return parseMetaId();
	else if( (c == '_') || isalpha(c) )
		return parseWord();
	else if( c == '"' )
		return parseString();
	else if( c == '\\' )
		return parseEscape();
	else if( isdigit(c) )
		return parseNumber();
	else if( c == '{' )
		return NEW_TOKEN( LeftCurly, "{" );
	else if( c == '}' )
		return NEW_TOKEN( RightCurly, "}" );
	else if( c == '[' )
		return NEW_TOKEN( LeftSquare, "[" );
	else if( c == ']' )
		return NEW_TOKEN( RightSquare, "]" );
	else if( c == '(' )
		return NEW_TOKEN( LeftParen, "(" );
	else if( c == ')' )
		return NEW_TOKEN( RightParen, ")" );
	else if( c == ',' )
		return NEW_TOKEN( Comma, "," );
	else if( c == ';' )
		return NEW_TOKEN( Semicolon, ";" );
	else if( c == '.' )
		return NEW_TOKEN( Dot, "." );
	else
		return NEW_TOKEN( Misc, string("") + c );
}

Token *Tokenizer::parseComment()
{
	int linenoStart = _lineno;
	stringstream buf;
	buf << '#';

	if( _in->peek() == '*' )
	{
		// Multi-Line Comment
		buf << '*';
		_in->ignore();

		enum {
			Init,
			Splat,
			Complete
		} state = Init;

		while( _in->good() && (state != Complete) )
		{
			char c = get();
			buf << c;

			if( c == '*' )
				state = Splat;
			else if( (c == '#') && (state == Splat) )
				state = Complete;
			else
				state = Init;
		}

		if( state != Complete )
		{
			cerr << _sourceName << ":" << linenoStart << ": Unterminated multi-line comment." << endl;
			exit( 1 );
		}

		return NEW_TOKEN( Comment, buf.str() );
	}
	else
	{
		// Single-line comment
		stringstream buf;
		buf << '#';

		while( _in->good() && (_in->peek() != '\n') )
			buf << get();

		return NEW_TOKEN( Comment, buf.str() );
	}
}

Token *Tokenizer::parseWhitespace()
{
	_in->unget();
	if( _in->peek() == '\n' )
		_lineno--;

	stringstream buf;

	while( _in->good() )
	{
		char c = _in->peek();
		if( !isspace(c) )
		{
			break;
		}
		buf << get();
	}

	return NEW_TOKEN( Whitespace, buf.str() );
}

Token *Tokenizer::parseMetaId()
{
	stringstream buf;

	buf << '@';

	while( _in->good() )
	{
		char c = _in->peek();
		if( isspace(c) )
			break;
		buf << get();
	}

	return NEW_TOKEN( MetaId, buf.str() );
}

Token *Tokenizer::parseWord()
{
	_in->unget();

	stringstream buf;
	buf << get();

	while( _in->good() )
	{
		char c = _in->peek();
		if( !isalnum(c) && (c != '_') )
			break;
		buf << get();
	}

	string text = buf.str();

	if( text == "enum" )
		return NEW_TOKEN( Enum, "enum" );
	if( text == "class" )
		return NEW_TOKEN( Class, "class" );
	if( text == "dyn" )
		return NEW_TOKEN( Dyn, "dyn" );
	if( text == "attrs" )
		return NEW_TOKEN( Attrs, "attrs" );

	return NEW_TOKEN( Id, buf.str() );
}

Token *Tokenizer::parseEscape()
{
	char buf[3] = { '\\', get(), 0 };

	return NEW_TOKEN( Misc, buf );
}

Token *Tokenizer::parseString()
{
	int lineno = _lineno;

	stringstream buf;
	buf << '"';

	enum State {
		Init,
		Escape,
		Complete
	} state = Init;

	while( _in->good() && (state != Complete) )
	{
		char c = get();
		buf << c;

		if( state == Escape )
			state = Init;
		else if( c == '\\' )
			state = Escape;
		else if( c == '"' )
			state = Complete;
		else if( c == '\n' )
			break;
	}

	if( state != Complete )
	{
		cerr << _sourceName << ":" << lineno << ": Unterminated string literal." << endl;
		exit( 1 );
	}

	return NEW_TOKEN( String, buf.str() );
}

Token *Tokenizer::parseNumber()
{
	_in->unget();

	stringstream buf;

	enum State {
		Init,
		Dot,
		Complete
	} state = Init;

	while( _in->good() && (state != Complete) )
	{
		char c = _in->peek();

		if( c == '.' )
		{
			if( state == Dot )
			{
				cerr << _sourceName << ":" << _lineno << ": Unexpected '.'" << endl;
				exit( 1 );
			}

			state = Dot;
		}
		else if( c == 'l' )
		{
			if( state == Dot )
			{
				cerr << _sourceName << ":" << _lineno << ": Illegal combination of '.' and 'l' suffix" << endl;
				exit( 1 );
			}
			state = Complete;
		}
		else if( c == 'f' || c == 'd' )
			state = Complete;
		else if( !isdigit(c) )
		{
			state = Complete;
			break;
		}

		buf << get();
	}

	return NEW_TOKEN( Number, buf.str() );
}

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// --- CLASS SyntaxNode
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
void SyntaxNode::dump( ostream &out, string indent )
{
	out << indent << typeName;

	if( children.empty() )
	{
		out << "('" << Token::toString( beginToken, endToken ) << "')" << endl;
	}
	else
	{
		out << "('" << beginToken->text << "' --> '" << endToken->text << "')" << endl;

		indent += "  ";
		itfor( vector<SyntaxNode *>, children, it )
		{
			(*it)->dump( out, indent );
		}
	}
}

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// --- CLASS Parser
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
//#define ptrc( x...) cout << x << endl;
#define ptrc( x...)

#define PUSH_TOK(TYPE,TOK) pushNode( SyntaxNode::TYPE, #TYPE, TOK )
#define PUSH(TYPE) PUSH_TOK( TYPE, _currTok )
#define PUSH_PEEK(TYPE) pushNode( SyntaxNode::TYPE, #TYPE, peek() )
#define PUSH_POP(TYPE) {PUSH(TYPE); POP(TYPE);}
#define POP_TOK(TYPE,TOK) popNode( SyntaxNode::TYPE, #TYPE, TOK )
#define POP(TYPE) POP_TOK( TYPE, _currTok )
#define POP_PEEK(TYPE) POP_TOK( TYPE, peek() )

SyntaxNode *Parser::parseDocument( std::string path, istream *in )
{
	init( path, in );

	next( Token::Bof );
	PUSH( Document );

	parseMetaProperties();
	parseObject();

	next( Token::Eof );
	SyntaxNode *result = POP( Document );
	//result->dump( cout );

	delete _tokenizer;

	return result;
}

SyntaxNode *Parser::parseSymbolPath( string text )
{
	init( "<string>", new istringstream(text) );

	next( Token::Bof );

	SyntaxNode *result = parseSymbolPath();

	next( Token::Eof );

	delete _tokenizer;

	return result;
}

SyntaxNode *Parser::parsePropertyValue( string text )
{
	init( "<string>", new istringstream(text) );

	next( Token::Bof );

	SyntaxNode *result = parsePropertyValue();

	next( Token::Eof );

	delete _tokenizer;

	return result;
}

SyntaxNode *Parser::parseMetaPropertyValue( string text )
{
	init( "<string>", new istringstream(text) );

	next( Token::Bof );

	SyntaxNode *result = parseMetaPropertyValue();

	next( Token::Eof );

	delete _tokenizer;

	return result;
}

void Parser::init( string path, istream *in )
{
	_path = path;
	_tokenizer = new Tokenizer( path, in );
	_currTok = NULL;
	_syntaxNode = NULL;
}

void Parser::err( Token *tok, string message )
{
	cerr << _path << ":" << tok->lineno << " " << message << endl;
	exit( 1 );
}

Token *Parser::peek()
{
	if( _currTok->next )
		return _currTok->next;
	else
		return _tokenizer->next();
}

Token *Parser::next( Token::Type required )
{
	Token *tok = next();
	if( tok->type != required )
	{
		err( tok, string("Unexpected '") + tok->text + "'" );
	}
	return tok;
}

Token *Parser::next()
{
	if( _currTok && _currTok->next )
		_currTok = _currTok->next;
	else
		_currTok = _tokenizer->next();

	return _currTok;
}

void Parser::pushNode( SyntaxNode::Type type, const char *typeName, Token *begin )
{
	SyntaxNode *node = new SyntaxNode();
	node->type = type;
	node->typeName = typeName;
	node->beginToken = begin;
	node->parent = _syntaxNode;
	if( _syntaxNode )
		_syntaxNode->children.push_back( node );
	_syntaxNode = node;

	ptrc( "begin " << typeName << ", tok=" << begin->text );
}

SyntaxNode *Parser::popNode( SyntaxNode::Type type, const char *typeName, Token *end )
{
	ptrc( "end " << typeName << ", tok=" << end->text );

	SyntaxNode *node = _syntaxNode;
	assert( node->type == type );

	node->endToken = end;

	_syntaxNode = node->parent;
	return node;
}

void Parser::parseMetaProperties()
{
	while( peek()->type == Token::MetaId )
	{
		next( Token::MetaId );
		PUSH( MetaProperty );

		PUSH_POP( Id );

		parseMetaPropertyValue();

		POP( MetaProperty );
	}
}

SyntaxNode *Parser::parseMetaPropertyValue()
{
	PUSH_PEEK( MetaPropertyValue );
	while( !peek()->hasNewline() && (peek()->type != Token::Eof) )
	{
		next();
		PUSH_POP( Misc );
	}
	return POP( MetaPropertyValue );
}

void Parser::parseObject()
{
	PUSH( Object );

	bool containerComplete = false;

	while( !containerComplete )
	{
		switch( peek()->type )
		{
		case Token::Id:
			parseProperty();
			break;
		case Token::Enum:
			parseEnum();
			break;
		case Token::Class:
			parseClass();
			break;
		default:
			containerComplete = true;
			break;
		}
	}

	POP_PEEK( Object );
}

void Parser::parseProperty()
{
	next( Token::Id );
	PUSH( Property );

	PUSH_POP( Id );

	parsePropertyValue();

	POP( Property );
}

SyntaxNode *Parser::parsePropertyValue()
{
	PUSH_PEEK( PropertyValue );

	switch( peek()->type )
	{
	case Token::LeftCurly:
		next( Token::LeftCurly );
		parseObject();
		next( Token::RightCurly );
		break;
	case Token::LeftSquare:
		parseArray();
		break;
	case Token::Dyn:
		parseDyn();
		break;
	default:
		parseExpression();
		break;
	}

	return POP( PropertyValue );
}

void Parser::parseArray()
{
	next( Token::LeftSquare );
	PUSH( Array );

	switch( peek()->type )
	{
	case Token::RightSquare:
		// empty array
		break;
	case Token::LeftCurly:
		while( true )
		{
			next( Token::LeftCurly );
			PUSH( PropertyValue );
			parseObject();
			next( Token::RightCurly );
			POP( PropertyValue );

			if( peek()->type == Token::Comma )
				next( Token::Comma );
			else
				break;
		}
		break;
	default:
		while( true )
		{
			PUSH_PEEK( PropertyValue );
			if( peek()->type == Token::Dyn )
			{
				parseDyn();
			}
			else
			{
				parseExpression();
			}
			POP( PropertyValue );

			if( peek()->type == Token::Comma )
				next( Token::Comma );
			else
				break;
		}
		break;
	}

	next( Token::RightSquare );
	POP( Array );
}

void Parser::parseDyn()
{
	next( Token::Dyn );
	PUSH( Dyn );

	parseExpression();

	switch( peek()->type )
	{
	case Token::Attrs:
		parseDynAttrs();
		break;
	case Token::LeftCurly:
		parseCppClause();
		break;
	default:
		// no-op
		break;
	}

	POP( Dyn );
}

void Parser::parseDynAttrs()
{
	next( Token::Attrs );
	next( Token::LeftCurly );

	while( peek()->type != Token::RightCurly )
	{
		if( peek()->type != Token::Id )
			err( peek(), "Expecting identifier" );

		next( Token::Id );
		PUSH( DynAttr );

		PUSH_POP( Id );

		if( peek()->type == Token::LeftCurly )
			parseCppClause();
		else
			parseExpression();

		POP( DynAttr );
	}

	next( Token::RightCurly );
}

void Parser::parseCppClause()
{
	if( peek()->type != Token::LeftCurly )
		err( peek(), "Expecting '{'" );

	{
		PUSH_PEEK( CppClause );

		int depth = 0;
		Token *tokStart = peek();
		do
		{
			if( peek()->type == Token::Id )
			{
				parseSymbolPath();
			}
			else
			{
				switch( next()->type )
				{
				case Token::LeftCurly:
					depth++;
					break;
				case Token::RightCurly:
					depth--;
					break;
				case Token::Eof:
					err( tokStart, "Missing '}'" );
					break;
				default:
					// no-op
					break;
				}

				PUSH_POP( Misc );
			}
		} while( depth );

		POP( CppClause );
	}
}

SyntaxNode *Parser::parseExpression( bool parenDelimited )
{
	PUSH_PEEK( Expression );

	Token *tokStart = NULL;
	int parenDepth = 0;
	bool complete = false;

	while( !complete )
	{
		if( !parenDepth && tokStart && peek()->hasNewline() )
		{
			complete = true;
		}
		else
		{
			Token *tok = NULL;

			if( peek()->type == Token::Id )
			{
				tok = peek();
				parseSymbolPath();
			}
			else
			{
				if( parenDepth )
				{
					tok = next();
					if( tok->type == Token::Eof )
						complete = true;
				}
				else
				{
					switch( peek()->type )
					{
					case Token::Semicolon:
						complete = true;
						tok = next();
						break;
					case Token::Comma:
					case Token::RightSquare:
					case Token::LeftCurly:
					case Token::RightCurly:
					case Token::Eof:
						complete = true;
						break;
					default:
						tok = next();
						break;
					}
				}

				if( tok )
					PUSH_POP( Misc );
			}

			if( tok )
			{
				if( tokStart == NULL )
				{
					if( parenDelimited && (tok->type != Token::LeftParen) )
						err( tok, "Expecting '('" );
					tokStart = tok;
				}

				switch( tok->type )
				{
				case Token::LeftParen:
					parenDepth++;
					break;
				case Token::RightParen:
					parenDepth--;
					if( parenDepth < 0 )
						err( tok, "Unexpected ')'" );
					if( parenDelimited && parenDepth == 0 )
						complete = true;
					break;
				default:
					//no-op
					break;
				}
			}
		}
	}

	if( tokStart == NULL )
		err( _currTok, string("Expecting scalar after '") + _currTok->text + "'" );
	else if( parenDepth > 0 )
		err( tokStart, "Missing ')'" );

	return POP( Expression );
}

SyntaxNode *Parser::parseSymbolPath()
{
	PUSH_PEEK( SymbolPath );

	bool complete = false;
	Token *beginArrayIndex = NULL;

	while( !complete )
	{
		if( beginArrayIndex )
		{
			switch( next()->type )
			{
			case Token::RightSquare:
				if( _currTok->prev == beginArrayIndex )
				{
					// empty []
					complete = true;
				}
				else
				{
					PUSH_TOK( SymbolPathElement, beginArrayIndex->next );
					POP_TOK( SymbolPathElement, _currTok->prev );
					if( peek()->type != Token::Dot )
						complete = true;
				}
				beginArrayIndex = NULL;
				break;
			case Token::Eof:
				// This will yield an error message
				complete = true;
				break;
			default:
				// no-op
				break;
			}
		}
		else
		{
			switch( peek()->type )
			{
			case Token::Id:
				next();
				PUSH_POP( SymbolPathElement );
				if( (peek()->type != Token::Dot) && (peek()->type != Token::LeftSquare) )
					complete = true;
				break;
			case Token::LeftSquare:
				next();
				beginArrayIndex = _currTok;
				break;
			case Token::Dot:
				next();
				break;
			default:
				complete = true;
				break;
			}
		}
	}

	if( beginArrayIndex )
		err( beginArrayIndex, "Missing ']'" );

	return POP( SymbolPath );
}

void Parser::parseEnum()
{
	next( Token::Enum );
	PUSH( Enum );

	next( Token::Id );
	PUSH( Id ); POP( Id );

	next( Token::LeftCurly );
	PUSH( EnumValues );

	while( true )
	{
		next( Token::Id );
		PUSH_POP( Id );

		if( peek()->type == Token::Comma )
			next( Token::Comma );
		else
			break;
	}

	next( Token::RightCurly );
	POP( EnumValues );
	POP( Enum );
}

void Parser::parseClass()
{
	next( Token::Class );
	PUSH( Class );

	next( Token::Id );
	PUSH( Id ); POP( Id );

	next( Token::LeftCurly );
	parseObject();
	next( Token::RightCurly );

	POP( Class );
}

#undef ptrc
#undef PUSH
#undef PUSH_PEEK
#undef PUSH_POP
#undef POP
