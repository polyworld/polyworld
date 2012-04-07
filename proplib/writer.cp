#include "writer.h"

#include <assert.h>

#include <algorithm>
#include <vector>

#include "dom.h"
#include "misc.h"

using namespace std;
using namespace proplib;

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// --- CLASS DocumentWriter
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------

DocumentWriter::DocumentWriter( ostream &out )
: _out( out )
{
}

void DocumentWriter::write( Document *doc )
{
	_doc = doc;
	_prevPropertyDoc = doc;
	writeMetaProperties( doc );
	writeProperties( doc );
}

void DocumentWriter::writeScalarNames( Document *doc, int depthStart )
{
	writeScalarNames( (Property *)doc, depthStart );
}

Token *DocumentWriter::findPrevToken( Node *node, Token::Type type, bool search )
{
	for( Token *tok = node->getLocation().getBeginToken()->prev;
		 tok;
		 tok = tok->prev )
	{
		if( tok->type == type )
			return tok;

		if( !search )
			break;
	}

	return NULL;
}

Token *DocumentWriter::findBeginToken( Node *node, Token::Type type, bool search )
{
	Token *tokEnd = node->getLocation().getEndToken();

	for( Token *tok = node->getLocation().getBeginToken();
		 tok;
		 tok = tok->next )
	{
		if( tok->type == type )
			return tok;

		if( tok == tokEnd || !search)
			break;
	}

	return NULL;
}

Token *DocumentWriter::findEndToken( Node *node, Token::Type type )
{
	Token *tokBegin = node->getLocation().getBeginToken();

	for( Token *tok = node->getLocation().getEndToken();
		 tok;
		 tok = tok->prev )
	{
		if( tok->type == type )
			return tok;

		if( tok == tokBegin )
			break;
	}

	return NULL;
}

Token *DocumentWriter::findNextToken( Node *node, Token::Type type, bool search )
{
	for( Token *tok = node->getLocation().getEndToken()->next;
		 tok;
		 tok = tok->next )
	{
		if( tok->type == type )
			return tok;

		if( !search )
			break;
	}

	return NULL;
}


void DocumentWriter::writeToken( Token *tok )
{
	_out << tok->getDecorationString();
	_out << tok->text;
}

static bool lessthan( Node *a, Node *b )
{
	return a->getLocation() < b->getLocation();
}

void DocumentWriter::sortMeta( Document *doc, vector<MetaProperty *> &result )
{
	itfor( MetaPropertyMap, doc->metaprops(), it )
		result.push_back( it->second );

	sort( result.begin(), result.end(), lessthan );
}

void DocumentWriter::sortChildren( __ContainerProperty *prop, vector<Property *> &result )
{
	itfor( PropertyMap, prop->props(), it )
		result.push_back( it->second );

	sort( result.begin(), result.end(), lessthan );
}

void DocumentWriter::sortAttributes( DynamicScalarProperty *prop, vector<DynamicScalarAttribute *> &result )
{
	itfor( DynamicScalarAttributeMap, prop->attrs(), it )
		result.push_back( it->second );

	sort( result.begin(), result.end(), lessthan );
}

void DocumentWriter::writeMetaProperties( Document *doc )
{
	vector<MetaProperty *> sorted;
	sortMeta( doc, sorted );

	itfor( vector<MetaProperty *>, sorted, it )
	{
		MetaProperty *prop = *it;
		Token *tokId = findBeginToken(prop, Token::MetaId);
		if( tokId )
		{
			if( (prop != sorted.front()) && !tokId->hasNewline() )
				_out << "\n";
			writeToken( tokId );
		}
		else
		{
			if( prop != sorted.front() )
				_out << "\n";
			
			_out << prop->getId().getName();
		}

		_out << " " << prop->getValue();
	}
}

void DocumentWriter::writeProperties( ObjectProperty *object )
{
	vector<Property *> sorted;
	sortChildren( object, sorted );

	itfor( vector<Property *>, sorted, it )
		writeProperty( *it );
}

void DocumentWriter::writeProperty( Property *prop )
{
	if( prop->getSubtype() == Node::Runtime )
		return;

	if( prop->getParent()->getType() != Node::Array )
	{
		Token *tokId = findBeginToken( prop, Token::Id );
		string decoration = tokId->getDecorationString();

		Document *doc = prop->getLocation().getDocument();
		if( ( (doc != _doc) || (doc != _prevPropertyDoc) ) && !tokId->hasNewline() )
			decoration = "\n" + decoration;
		_prevPropertyDoc = doc;

		_out << decoration;
		_out << prop->getName();
	}

	switch( prop->getType() )
	{
	case Node::Scalar:
		switch( prop->getSubtype() )
		{
		case Node::Const:
			writeConst( dynamic_cast<ConstScalarProperty *>(prop) );
			break;
		case Node::Dynamic:
			writeDynamic( dynamic_cast<DynamicScalarProperty *>(prop) );
			break;
		default:
			assert( false );
			break;
		}
		break;
	case Node::Array:
		writeArray( dynamic_cast<ArrayProperty *>(prop) );
		break;
	case Node::Object:
		writeObject( dynamic_cast<ObjectProperty *>(prop) );
		break;
	default:
		assert( false );
	}
}

void DocumentWriter::writeConst( ConstScalarProperty *prop )
{
	writeExpression( prop->getExpression() );
}

void DocumentWriter::writeDynamic( DynamicScalarProperty *prop )
{
	writeToken( findBeginToken(prop, Token::Dyn) );
	writeExpression( prop->getInitExpression() );

	vector<DynamicScalarAttribute *> attrs;
	sortAttributes( prop, attrs );

	if( (attrs.size() == 1) && findBeginToken(attrs.front(), Token::LeftCurly, false) )
	{
		writeExpression( attrs.front()->getExpression() );
	}
	else if( !attrs.empty() )
	{
		writeToken( findPrevToken(attrs.front(), Token::Attrs) );
		writeToken( findPrevToken(attrs.front(), Token::LeftCurly, false) );

		itfor( vector<DynamicScalarAttribute *>, attrs, it )
		{
			writeToken( findBeginToken(*it, Token::Id) );
			writeExpression( (*it)->getExpression() );
		}

		writeToken( findNextToken(attrs.back(), Token::RightCurly, false) );
	}
}

void DocumentWriter::writeArray( ArrayProperty *prop )
{
	writeToken( findBeginToken(prop, Token::LeftSquare) );

	int nelements = prop->size();
	for( int i = 0; i < nelements; i++ )
	{
		writeProperty( prop->getp(i) );

		if( i != (nelements - 1) )
			writeToken( findNextToken(prop->getp(i), Token::Comma) );
	}

	writeToken( findEndToken(prop, Token::RightSquare) );
}

void DocumentWriter::writeObject( ObjectProperty *prop )
{
	writeToken( findBeginToken(prop, Token::LeftCurly) );

	writeProperties( prop );

	writeToken( findEndToken(prop, Token::RightCurly) );
}

void DocumentWriter::writeExpression( Expression *expr )
{
	expr->write( _out );
}

void DocumentWriter::writeScalarNames( Property *prop, int depthStart )
{
	switch( prop->getType() )
	{
	case Node::Array:
	case Node::Object:
		{
			vector<Property *> sorted;
			sortChildren( dynamic_cast<__ContainerProperty *>(prop), sorted );
			itfor( vector<Property *>, sorted, it )
			{
				writeScalarNames( *it, depthStart );
			}
		}
		break;
	case Node::Scalar:
		_out << prop->getFullName( depthStart ) << endl;
		break;
	default:
		assert( false );
	}
}
