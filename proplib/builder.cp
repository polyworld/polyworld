#include "builder.h"

#include <assert.h>

#include <sstream>

#include "convert.h"
#include "dynamic.h"
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

	return _doc;
}

SchemaDocument *DocumentBuilder::buildSchemaDocument( const string &path )
{
	proplib::Parser parser;
	SyntaxNode *node = parser.parseDocument( path, new ifstream(path.c_str()) );

	SchemaDocument *doc = new SchemaDocument( path, path );

	// schema file does not have a 'type' attribute at the top-level, so inject one.
	doc->add( buildConstScalarProperty(DocumentLocation(doc),
									   "type",
									   "Object") );
	

	// schema file does not have a 'properties' attribute at the top-level, so inject one.
	ObjectProperty *properties = new ObjectProperty( DocumentLocation(doc),
													 "properties" );
	doc->add( properties );

	// schema file does not declare this enum of types, so inject it.
	proplib::Enum *enum_ = new proplib::Enum( DocumentLocation(doc), "__types" );
	enum_->addValue( "Int" );
	enum_->addValue( "Float" );
	enum_->addValue( "Bool" );
	enum_->addValue( "Object" );
	enum_->addValue( "Array" );
	enum_->addValue( "String" );
	enum_->addValue( "Enum" );

	doc->addEnum( enum_ );

	// build up the document, with top-level schema properties placed in 'properties'
	_path = path;
	_doc = doc;
	buildDocument( node, properties );

	return doc;
}

OverlayDocument *DocumentBuilder::buildOverlayDocument( const string &path )
{
	proplib::Parser parser;
	SyntaxNode *node = parser.parseDocument( path, new ifstream(path.c_str()) );

	OverlayDocument *overlay = new OverlayDocument( path, path );
	_path = path;
	_doc = overlay;

	buildDocument( node );

	overlay->validate();

	return overlay;
}

SymbolPath *DocumentBuilder::buildSymbolPath( const string &text )
{
	proplib::Parser parser;
	SyntaxNode *node = parser.parseSymbolPath( text );

	_path = "<string>";
	_doc = NULL;

	return buildSymbolPath( node );
}

ConstScalarProperty *DocumentBuilder::buildConstScalarProperty( DocumentLocation loc, Identifier id, const string &value )
{
	_doc = loc.getDocument();
	_path = _doc->getPath();

	proplib::Parser parser;
	SyntaxNode *node = parser.parseExpression( value );
	Expression *expression = buildExpression( node );

	return new ConstScalarProperty( loc, id, expression );
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

void DocumentBuilder::buildDocument( SyntaxNode *node, ObjectProperty *obj )
{
	for( int i = 0; i < (int)node->children.size() - 1; i++ )
		_doc->addMeta( buildMetaProperty(node->children[i]) );

	buildObjectProperty( node->children[node->children.size() - 1], -1, obj ? obj : _doc );
}

MetaProperty *DocumentBuilder::buildMetaProperty( SyntaxNode *node )
{
	assert( node->type == SyntaxNode::MetaProperty );

	assert( node->children[0]->type == SyntaxNode::Id );
	assert( node->children[1]->type == SyntaxNode::Expression );

	return new MetaProperty( createLocation(node),
							 createId(node->children[0]),
							 buildExpression(node->children[1]) );
}

Property *DocumentBuilder::buildObjectProperty( SyntaxNode *node,
												Identifier id,
												ObjectProperty *obj )
{
	if( obj == NULL )
	{
		switch( node->type )
		{
		case SyntaxNode::Property:
			{
				obj = new ObjectProperty( createLocation(node),
										  createId(node->children[0]) );
				node = node->children[1];
			}
			break;
		case SyntaxNode::Object: // Array Element
			{
				obj = new ObjectProperty( createLocation(node),
										  id );
			}
			break;
		default:
			assert( false );
		}
	}

	assert( node->type == SyntaxNode::Object );

	itfor( vector<SyntaxNode *>, node->children, it )
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
		default:
			assert( false );
			break;
		}
	}

	return obj;
}

Property *DocumentBuilder::buildProperty( SyntaxNode *node )
{
	assert( node->children[0]->type == SyntaxNode::Id );

	switch( node->children[1]->type )
	{
	case SyntaxNode::Expression:
		return buildConstScalarProperty( node );
	case SyntaxNode::Dyn:
		return buildDynamicScalarProperty( node );
	case SyntaxNode::Object:
		return buildObjectProperty( node );
	case SyntaxNode::Array:
		return buildArrayProperty( node );
	default:
		assert( false );
		return NULL;
	}
}

ConstScalarProperty *DocumentBuilder::buildConstScalarProperty( SyntaxNode *node, Identifier id )
{
	Expression *expr;

	switch( node->type )
	{
	case SyntaxNode::Property:
		id = createId( node->children[0] );
		expr = buildExpression( node->children[1] );
		break;
	case SyntaxNode::Expression: // Array Element
		expr = buildExpression( node );
		break;
	default:
		assert( false );
		return NULL;
	}

	return new ConstScalarProperty( createLocation(node),
									id,
									expr );
}

Property *DocumentBuilder::buildDynamicScalarProperty( SyntaxNode *node, Identifier id )
{
	SyntaxNode *nodeDyn;

	switch( node->type )
	{
	case SyntaxNode::Property:
		id = createId(node->children[0]);
		nodeDyn = node->children[1];
		break;
	case SyntaxNode::Dyn: // Array Element
		nodeDyn = node;
		break;
	default:
		assert( false );
		return NULL;
	}

	Expression *initExpr = buildExpression( nodeDyn->children[0] );
	DynamicScalarProperty *prop = new DynamicScalarProperty( createLocation(node),
															 id,
															 initExpr );

	for( int i = 1; i < (int)nodeDyn->children.size(); i++ )
		prop->add( buildDynamicScalarAttribute(nodeDyn->children[i]) );

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

Property *DocumentBuilder::buildArrayProperty( SyntaxNode *node )
{
	ArrayProperty *prop = new ArrayProperty( createLocation(node),
											 createId(node->children[0]) );

	int index = 0;

	itfor( vector<SyntaxNode *>, node->children[1]->children, it )
	{
		SyntaxNode *childNode = *it;

		switch( childNode->type )
		{
		case SyntaxNode::Expression:
			prop->add( buildConstScalarProperty(childNode, Identifier(index)) );
			break;
		case SyntaxNode::Dyn:
			prop->add( buildDynamicScalarProperty(childNode, Identifier(index)) );
			break;
		case SyntaxNode::Object:
			prop->add( buildObjectProperty(childNode, Identifier(index)) );
			break;
		default:
			assert( false );
			break;
		}

		index++;
	}

	return prop;
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
