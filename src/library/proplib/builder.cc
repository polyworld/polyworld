#include "builder.h"

#include <assert.h>

#include <sstream>

#include "convert.h"
#include "cppprops.h"
#include "editor.h"
#include "expression.h"
#include "interpreter.h"
#include "misc.h"
#include "overlay.h"
#include "parser.h"
#include "schema.h"


using namespace std;
using namespace proplib;


// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// --- CLASS DocumentBuilder
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
Document *DocumentBuilder::buildDocument( const string &path )
{
	proplib::Parser parser;
	SyntaxNode *node = parser.parseDocument( path, new ifstream(path.c_str()) );

	_path = path;
	_doc = new Document( path, path );

	buildDocument( node );

	return _doc;
}

Document *DocumentBuilder::buildWorldfileDocument( SchemaDocument *schema, const std::string &path )
{
	if( !exists(path) )
	{
		cerr << "No such file: " << path << endl;
		exit( 1 );
	}

	if( WorldfileConverter::isV1(path) )
	{
		string pathv2 = path + ".v2";
		WorldfileConverter::convertV1SyntaxToV2( path, pathv2 );
			
		proplib::Parser parser;
		SyntaxNode *node = parser.parseDocument( pathv2, new ifstream(pathv2.c_str()) );

		_path = pathv2;
		_doc = new Document( _path, _path );
		buildDocument( node );

		DocumentEditor editor( schema, _doc );
		WorldfileConverter::convertV1PropertiesToV2( &editor, _doc );
	}
	else
	{
		proplib::Parser parser;
		SyntaxNode *node = parser.parseDocument( path, new ifstream(path.c_str()) );
		_path = path;
		_doc = new Document( _path, _path );

		buildDocument( node );
	}

	DocumentEditor editor( schema, _doc );
	WorldfileConverter::convertDeprecatedV2Properties( &editor, _doc );

	return _doc;
}

SchemaDocument *DocumentBuilder::buildSchemaDocument( const string &path )
{
	proplib::Parser parser;
	SyntaxNode *node = parser.parseDocument( path, new ifstream(path.c_str()) );

	SchemaDocument *doc = new SchemaDocument( path, path );

	// schema file does not have a 'type' attribute at the top-level, so inject one.
	doc->add( buildProperty(DocumentLocation(doc),
							"type",
							"Object") );
	

	// schema file does not have a 'properties' attribute at the top-level, so inject one.
	ObjectProperty *properties = new ObjectProperty( DocumentLocation(doc),
													 "properties" );
	doc->add( properties );

	// schema file does not declare this enum of types, so inject it.
	proplib::Enum *enum_ = new proplib::Enum( DocumentLocation(doc), "__types" );
	itfor( set<string>, SchemaDocument::standardTypes, it )
		enum_->addValue( *it );

	doc->addEnum( enum_ );

	// build up the document, with top-level schema properties placed in 'properties'
	_path = path;
	_doc = doc;
	buildDocument( node, properties );

	doc->init();

	return doc;
}

SymbolPath *DocumentBuilder::buildSymbolPath( const string &text )
{
	proplib::Parser parser;
	SyntaxNode *node = parser.parseSymbolPath( text );

	_path = "<string>";
	_doc = NULL;

	return buildSymbolPath( node );
}

MetaProperty *DocumentBuilder::buildMetaProperty( DocumentLocation loc, Identifier id, const string &value )
{
	_doc = loc.getDocument();
	_path = _doc->getPath();

	proplib::Parser parser;
	SyntaxNode *node = parser.parseMetaPropertyValue( value );

	return buildMetaProperty( loc, id, node );
}

Property *DocumentBuilder::buildProperty( DocumentLocation loc, Identifier id, const string &value )
{
	_doc = loc.getDocument();
	_path = _doc->getPath();

	proplib::Parser parser;
	SyntaxNode *node = parser.parsePropertyValue( value );

	return buildProperty( loc, id, node );
}

DocumentLocation DocumentBuilder::createLocation( SyntaxNode *node )
{
	return DocumentLocation( _doc, node->beginToken->lineno, node->beginToken, node->endToken );
}

Identifier DocumentBuilder::createId( SyntaxNode *node )
{
	assert( node->type == SyntaxNode::Id );
	assert( node->beginToken == node->endToken );

	return Identifier( node->beginToken->text );
}

void DocumentBuilder::buildDocument( SyntaxNode *node, ObjectProperty *rootContainer )
{
	for( int i = 0; i < (int)node->children.size() - 1; i++ )
	{
		SyntaxNode *nodeProp = node->children[i];
		assert( nodeProp->type == SyntaxNode::MetaProperty );

		_doc->addMeta( buildMetaProperty(createLocation(nodeProp),
										 createId(nodeProp->children[0]),
										 nodeProp->children[1]) );
	}

	// client may not want properties added to root of doc (e.g. schema wants them added to 'properties')
	buildObjectProperty( rootContainer ? rootContainer : _doc,
						 node->children[node->children.size() - 1] );
}

MetaProperty *DocumentBuilder::buildMetaProperty( DocumentLocation loc, Identifier id, SyntaxNode *nodeValue )
{
	assert( nodeValue->type == SyntaxNode::MetaPropertyValue );

	string value;
	if( nodeValue->children.empty() )
		value = "";
	else
		value = Token::toString( nodeValue->beginToken, nodeValue->endToken );

	return new MetaProperty( loc, id, value );
}

Property *DocumentBuilder::buildProperty( SyntaxNode *node )
{
	assert( node->children[0]->type == SyntaxNode::Id );
	assert( node->children[1]->type == SyntaxNode::PropertyValue );

	return buildProperty( createLocation(node),
						  createId(node->children[0]),
						  node->children[1] );
}

Property *DocumentBuilder::buildProperty( DocumentLocation loc,
										  Identifier id,
										  SyntaxNode *nodeValue )
{
	assert( nodeValue->type == SyntaxNode::PropertyValue );

	switch( nodeValue->children[0]->type )
	{
	case SyntaxNode::Expression:
		return buildConstScalarProperty( loc, id, nodeValue );
	case SyntaxNode::Dyn:
		return buildDynamicScalarProperty( loc, id, nodeValue );
	case SyntaxNode::Object:
		return buildObjectProperty( loc, id, nodeValue );
	case SyntaxNode::Array:
		return buildArrayProperty( loc, id, nodeValue );
	default:
		assert( false );
		return NULL;
	}
}

Property *DocumentBuilder::buildConstScalarProperty( DocumentLocation loc,
													 Identifier id,
													 SyntaxNode *nodeValue )
{
	assert( nodeValue->children[0]->type == SyntaxNode::Expression );

	return new ConstScalarProperty( loc,
									id,
									buildExpression(nodeValue->children[0]) );
}

Property *DocumentBuilder::buildDynamicScalarProperty( DocumentLocation loc,
													   Identifier id,
													   SyntaxNode *nodeValue )
{
	SyntaxNode *nodeDyn = nodeValue->children[0];
	assert( nodeDyn->type == SyntaxNode::Dyn );

	Expression *initExpr = buildExpression( nodeDyn->children[0] );
	DynamicScalarProperty *prop = new DynamicScalarProperty( loc,
															 id,
															 initExpr );

	for( int i = 1; i < (int)nodeDyn->children.size(); i++ )
		prop->add( buildDynamicScalarAttribute(nodeDyn->children[i]) );

	return prop;
}

ObjectProperty *DocumentBuilder::buildObjectProperty( DocumentLocation loc,
													  Identifier id,
													  SyntaxNode *nodeValue )
{
	assert( nodeValue->type == SyntaxNode::PropertyValue );
	ObjectProperty *obj = new ObjectProperty( loc, id );

	return buildObjectProperty( obj, nodeValue->children[0] );
}

ObjectProperty *DocumentBuilder::buildObjectProperty( ObjectProperty *obj,
													  SyntaxNode *nodeObject )
{
	assert( nodeObject->type == SyntaxNode::Object );

	itfor( vector<SyntaxNode *>, nodeObject->children, it )
	{
		SyntaxNode *childNode = *it;

		switch( childNode->type )
		{
		case SyntaxNode::Property:
			obj->add( buildProperty(childNode) );
			break;
		case SyntaxNode::Enum:
			obj->addEnum( buildEnum(childNode) );
			break;
		case SyntaxNode::Class:
			obj->addClass( buildClass(childNode) );
			break;
		default:
			assert( false );
			break;
		}
	}

	return obj;
}

Property *DocumentBuilder::buildArrayProperty( DocumentLocation loc, Identifier id, SyntaxNode *nodeValue )
{
	assert( nodeValue->type == SyntaxNode::PropertyValue );

	ArrayProperty *prop = new ArrayProperty( loc, id );

	int index = 0;

	itfor( vector<SyntaxNode *>, nodeValue->children[0]->children, it )
	{
		SyntaxNode *childNode = *it;

		Property *element = buildProperty( createLocation(childNode),
										   Identifier(index++),
										   childNode );
		prop->add( element );
	}

	return prop;
}

DynamicScalarAttribute *DocumentBuilder::buildDynamicScalarAttribute( SyntaxNode *node )
{
	switch( node->type )
	{
	case SyntaxNode::DynAttr:
		return new DynamicScalarAttribute( createLocation(node),
										   createId(node->children[0]),
										   buildExpression(node->children[1]) );
	case SyntaxNode::CppClause:
		return new DynamicScalarAttribute( createLocation(node),
										   "update",
										   buildExpression(node) );
	default:
		assert( false );
	}
}

Enum *DocumentBuilder::buildEnum( SyntaxNode *node )
{
	Enum *enum_ = new Enum( createLocation(node),
							createId(node->children[0]) );

	SyntaxNode *nodeValues = node->children[1];
	assert( nodeValues->type == SyntaxNode::EnumValues );
	
	itfor( vector<SyntaxNode *>, nodeValues->children, it )
	{
		enum_->addValue( (*it)->beginToken->text );
	}

	return enum_;
}

Class *DocumentBuilder::buildClass( SyntaxNode *node )
{
	ObjectProperty *definition = new ObjectProperty( createLocation(node), Identifier("definition") );
	buildObjectProperty( definition, node->children[1] );

	Class *klass = new Class( createLocation(node),
							  createId(node->children[0]),
							  definition );

	return klass;
}

Expression *DocumentBuilder::buildExpression( SyntaxNode *node )
{
	Expression *expression = new Expression();

	itfor( vector<SyntaxNode *>, node->children, it )
	{
		SyntaxNode *nodeElement = *it;

		switch( nodeElement->type )
		{
		case SyntaxNode::SymbolPath:
			expression->elements.push_back( new SymbolExpressionElement(buildSymbolPath(nodeElement)) );
			break;
		case SyntaxNode::Misc:
			expression->elements.push_back( new MiscExpressionElement(nodeElement->beginToken) );
			break;
		default:
			assert( false );
		}
	}

	return expression;
}

SymbolPath *DocumentBuilder::buildSymbolPath( SyntaxNode *node )
{
	SymbolPath *name = new SymbolPath();

	itfor( vector<SyntaxNode *>, node->children, it )
	{
		SyntaxNode *nodeElement = *it;
		assert( nodeElement->type == SyntaxNode::SymbolPathElement );

		name->add( nodeElement->beginToken );
	}

	return name;
}
