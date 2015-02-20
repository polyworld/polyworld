#include "expression.h"

#include <assert.h>

#include <sstream>

#include "misc.h"
#include "parser.h"

using namespace std;
using namespace proplib;

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// --- CLASS SymbolPath::Element
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
string SymbolPath::Element::getText()
{
	return token->text;
}

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// --- CLASS SymbolPath
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
SymbolPath::SymbolPath()
{
	head = NULL;
	tail = NULL;
}

SymbolPath::~SymbolPath()
{
	for( Element *e = head; e;  )
	{
		Element *del = e;
		e = e->next;
		delete del;
	}
}

void SymbolPath::add( Token *token )
{
	Element *e = new Element();
	e->token = token;
	e->next = NULL;

	e->prev = tail;
	if( tail )
		tail->next = e;
	tail = e;
	if( !head )
		head = e;
}

string SymbolPath::toString( bool leadingDecoration )
{
	return Token::toString( head->token,
							tail->token,
							leadingDecoration );
}


// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// --- CLASS ExpressionElement
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
ExpressionElement::ExpressionElement( Type type_ )
: type( type_ )
{
}

ExpressionElement::~ExpressionElement()
{
}


// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// --- CLASS SymbolExpressionElement
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
SymbolExpressionElement::SymbolExpressionElement( SymbolPath *name )
: ExpressionElement( Symbol )
, symbolPath( name )
{
}

SymbolExpressionElement::~SymbolExpressionElement()
{
}

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// --- CLASS MiscExpressionElement
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
MiscExpressionElement::MiscExpressionElement( Token *tok )
: ExpressionElement( Misc )
, token( tok )
{
}

MiscExpressionElement::~MiscExpressionElement()
{
}

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// --- CLASS Expression
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
Expression::Expression()
{
}

Expression::~Expression()
{
	itfor( list<ExpressionElement *>, elements, it )
		delete *it;
}

Expression *Expression::clone()
{
	// todo
	return this;
}

string Expression::toString( bool leadingDecoration )
{
	stringstream out;
	write( out, leadingDecoration );
	return out.str();
}

void Expression::write( ostream &out, bool leadingDecoration )
{
	Token *beginToken;
	Token *endToken;

	switch( elements.front()->type )
	{
	case ExpressionElement::Symbol:
		beginToken = dynamic_cast<SymbolExpressionElement *>( elements.front() )->symbolPath->head->token;
		break;
	case ExpressionElement::Misc:
		beginToken = dynamic_cast<MiscExpressionElement *>( elements.front() )->token;
		break;
	default:
		assert(false);
	}

	switch( elements.back()->type )
	{
	case ExpressionElement::Symbol:
		endToken = dynamic_cast<SymbolExpressionElement *>( elements.back() )->symbolPath->tail->token;
		break;
	case ExpressionElement::Misc:
		endToken = dynamic_cast<MiscExpressionElement *>( elements.back() )->token;
		break;
	default:
		assert(false);
	}

	out << Token::toString( beginToken, endToken, leadingDecoration );
}

bool Expression::isCppClause()
{
	return (elements.front()->type == ExpressionElement::Misc)
		&& dynamic_cast<MiscExpressionElement *>( elements.front() )->token->type == Token::LeftCurly;
}
