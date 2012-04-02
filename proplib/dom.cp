#include "dom.h"

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>

#include "interpreter.h"
#include "misc.h"

using namespace std;
using namespace proplib;

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// --- CLASS Identifier
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------

Identifier::Identifier( const string &name )
: _name( name )
{
}

Identifier::Identifier( const char *name)
: _name( name )
{
}

Identifier::Identifier( int index )
{
	char buf[32];
	sprintf( buf, "%d", index );
	_name = buf;
}

Identifier::Identifier( size_t index )
{
	char buf[32];
	sprintf( buf, "%lu", index );
	_name = buf;
}

Identifier::Identifier( const Identifier &other )
: _name( other._name )
{
}

Identifier::~Identifier()
{
}

const char *Identifier::getName() const
{
	return _name.c_str();
}

bool Identifier::isIndex() const
{
	for( size_t i = 0; i < _name.size(); i++ )
		if( !isdigit(_name[i]) )
			return false;

	return true;
}

namespace proplib
{
	bool operator<( const Identifier &a, const Identifier &b )
	{
		return strcmp( a.getName(), b.getName() ) < 0;
	}
}

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// --- CLASS DocumentLocation
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
DocumentLocation::DocumentLocation( Document *doc,
									unsigned int lineno,
									Token *beginToken,
									Token *endToken )
: _doc( doc )
, _lineno( lineno )
, _beginToken( beginToken )
, _endToken( endToken )
{
}

Document *DocumentLocation::getDocument()
{
	return _doc;
}

string DocumentLocation::getPath() const
{
	return _doc->getName();
}

string DocumentLocation::getDescription()
{
	char buf[1024 * 4];
	if( _lineno > 0 )
		sprintf( buf, "%s:%u", _doc->getName(), _lineno );
	else
		sprintf( buf, "%s", _doc->getName() );
	return buf;
}

Token *DocumentLocation::getBeginToken()
{
	return _beginToken;
}

Token *DocumentLocation::getEndToken()
{
	return _endToken;
}

void DocumentLocation::err( string msg )
{
	fprintf( stderr, "%s: ERROR! %s\n", getDescription().c_str(), msg.c_str() );
	exit( 1 );
}

void DocumentLocation::warn( string msg )
{
	fprintf( stderr, "%s: WARNING! %s\n", getDescription().c_str(), msg.c_str() );
}

namespace proplib
{
	bool operator<( const DocumentLocation &a, const DocumentLocation &b )
	{
		int cmp = strcmp( a.getPath().c_str(), b.getPath().c_str() );
		if( cmp != 0 )
			return cmp < 0;

		return a._lineno < b._lineno;
	}
}


// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// --- CLASS Node
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------

Node::Node( const Node &other )
: _loc( DocumentLocation(NULL) )
{
	assert( false );
}

Node::Node( Type type, Subtype subtype, DocumentLocation loc )
: _parent( NULL )
, _symbolSource( NULL )
, _type( type )
, _subtype( subtype )
, _loc( loc )
{
}

Node::~Node()
{
}

Node::Type Node::getType()
{
	return _type;
}

Node::Subtype Node::getSubtype()
{
	return _subtype;
}

DocumentLocation Node::getLocation()
{
	return _loc;
}

void Node::err( string message )
{
	_loc.err( message );
}

void Node::warn( string message )
{
	_loc.warn( message );
}

void Node::dump( ostream &out, string indent )
{
}

bool Node::findSymbol( SymbolPath *name, Symbol &sym )
{
	return findSymbol( name->head, sym );
}

bool Node::findSymbol( SymbolPath::Element *name, Symbol &sym )
{
	if( (_symbolSource != NULL) && _symbolSource->findSymbol(name, sym) )
		return true;
	else if( findLocalSymbol(name, sym) )
		return true;
	else if( _parent )
		return _parent->findSymbol( name, sym );
	else
		return false;
}

bool Node::findLocalSymbol( SymbolPath::Element *name, Symbol &sym )
{
	return false;
}

void Node::add( Node *node )
{
	node->_parent = this;
}


// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// --- CLASS Enum
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
Enum::Enum( DocumentLocation loc, Identifier id )
: Node( Node::Enum, Node::None, loc )
, _id( id )
{
}

Enum::~Enum()
{
}

const Identifier &Enum::getId()
{
	return _id;
}

void Enum::addValue( const string &name )
{
	if( contains(name) )
		err( string("Duplicate name: ") + name );

	_values.insert( name );
}

bool Enum::contains( const string &name )
{
	return _values.find(name) != _values.end();
}

bool Enum::findLocalSymbol( SymbolPath::Element *name, Symbol &sym )
{
	if( (name->next == NULL) && contains(name->getText()) )
	{
		sym.type = Symbol::EnumValue;
		sym.prop = NULL;
		return true;
	}
	else
		return false;
}

Enum *Enum::clone()
{
	Enum *clone = new Enum( getLocation(), _id );
	clone->_values = _values;
	return clone;
}


// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// --- CLASS Property
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------

Property::Property( const Property &other )
: Node( other )
, _id( -1 )
{
	assert( false );
}

Property::Property( Node::Type type, Node::Subtype subtype, DocumentLocation loc, Identifier id )
: Node( type, subtype, loc )
, _id( id )
, _schema( NULL )
{
}

Property::~Property()
{
}

const Identifier &Property::getId()
{
	return _id;
}

const char *Property::getName()
{
	return _id.getName();
}

std::string Property::getFullName( int minDepth, const char *delimiter )
{
	if( getDepth() < minDepth )
	{
		return "";
	}
	else if( _parent == NULL )
	{
		return getName();
	}
	else
	{
		string parentFullName = "";
		if( getParent() )
		{
			parentFullName = getParent()->getFullName( minDepth, delimiter );
		}

		if( parentFullName == "" )
		{
			return getName();
		}
		else
		{
			if( delimiter )
			{
				return parentFullName + delimiter + getName();
			}
			else
			{
				if( _id.isIndex() )
					return parentFullName + "[" + getName() + "]";
				else
					return parentFullName + "." + getName();
			}
		}
	}
}

Property *Property::getParent()
{
	return dynamic_cast<Property *>( _parent );
}

PropertyMap &Property::elements()
{
	return props();
}

size_t Property::size()
{
	return props().size();
}

bool Property::findLocalSymbol( SymbolPath::Element *name, Symbol &sym )
{
	itfor( EnumMap, _enums, it )
	{
		if( it->second->findLocalSymbol(name, sym) )
			return true;
	}

	if( (name->getText() == "parent") && getParent() )
	{
		if( name->next )
			return getParent()->findLocalSymbol( name->next, sym );

		sym.type = Symbol::Property;
		sym.prop = getParent();
		return true;
	}

	return false;
}

int Property::getDepth()
{
	if( getParent() == NULL )
		return 0;
	else
		return getParent()->getDepth() + 1;
}

void Property::addEnum( class Enum *enum_ )
{
	if( _enums.find(enum_->getId()) != _enums.end() )
	{
		enum_->err( "Duplicate enum" );
	}

	_enums[ enum_->getId() ] = enum_;
	Node::add( enum_ );
}

Enum *Property::getEnum( const string &name )
{
	EnumMap::iterator it = _enums.find( name );
	if( it == _enums.end() )
		return NULL;
	else
		return it->second;
}

bool Property::isEnumValue( const string &name )
{
	itfor( EnumMap, _enums, it )
		if( it->second->contains(name) )
			return true;

	return false;
}

bool Property::isString()
{
	return _schema && (string)_schema->get("type") == "String";
}

void Property::setSchema( ObjectProperty *schema )
{
	_schema = schema;
	_schema->_symbolSource = this;
}

ObjectProperty *Property::getSchema()
{
	return _schema;
}

bool Property::hasBinding()
{
	return _schema && _schema->getp( "cppsym" );
}

Property *Property::baseClone( Property *clone )
{
	itfor( EnumMap, _enums, it )
		clone->_enums[it->first] = it->second->clone();

	return clone;
}

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// --- CLASS __ScalarProperty
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
__ScalarProperty::__ScalarProperty( const __ScalarProperty &other )
: Property( other )
{
	assert( false );
}

__ScalarProperty::__ScalarProperty( Node::Subtype subtype, DocumentLocation loc, Identifier id )
: Property( Node::Scalar, subtype, loc, id )
{
}

__ScalarProperty::~__ScalarProperty()
{
}

Property &__ScalarProperty::get( Identifier id )
{
	err( string("Invalid request for property: '") + id.getName() );
	return *this;
}

Property *__ScalarProperty::getp( Identifier id )
{
	err( string("Invalid request for property: '") + id.getName() );
	return this;
}

PropertyMap &__ScalarProperty::props()
{
	err( "Invalid request for properties." );
	return *(new PropertyMap());
}

__ScalarProperty::operator short()
{
	return toInt();
}

__ScalarProperty::operator int()
{
	return toInt();
}

__ScalarProperty::operator long()
{
	return toInt();
}

__ScalarProperty::operator float()
{
	return toFloat();
}

__ScalarProperty::operator bool()
{
	return toBool();
}

__ScalarProperty::operator string()
{
	return getEvaledString();
}

void __ScalarProperty::dump( ostream &out, string indent )
{
	out << indent << getName() << " (schema=" << getSchema() << ")" << endl;
}

int __ScalarProperty::toInt()
{
	char *end;
	int result = (int)strtol( getEvaledString().c_str(), &end, 10 );

	if( *end != '\0' )
		err( "Expecting integer." );

	return result;
}

float __ScalarProperty::toFloat()
{
	char *end;
	float result = (float)strtof( getEvaledString().c_str(), &end );

	if( *end != '\0' )
		err( "Expecting float." );

	return result;
}

bool __ScalarProperty::toBool()
{
	string value = getEvaledString();

	if( (value == "1") || (value == "True") )
		return true;
	else if( (value == "0") || (value == "False") )
		return false;
	else
	{
		err( "Expecting bool." );
		return false;
	}
}


// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// --- CLASS ConstScalarProperty
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
ConstScalarProperty::ConstScalarProperty( const ConstScalarProperty &other )
: __ScalarProperty( other )
{
	assert( false );
}

ConstScalarProperty::ConstScalarProperty( DocumentLocation loc, Identifier id, Expression *expr )
: __ScalarProperty( Node::Const, loc, id )
, _expr( new Interpreter::ExpressionEvaluator(expr) )
{
}

ConstScalarProperty::~ConstScalarProperty()
{
}

Property *ConstScalarProperty::clone( Identifier cloneId )
{
	return baseClone( new ConstScalarProperty(getLocation(),
											  cloneId,
											  getExpression()->clone()) );
}

Expression *ConstScalarProperty::getExpression()
{
	return _expr->getExpression();
}

string ConstScalarProperty::getEvaledString()
{
	return _expr->evaluate( this );
}


// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// --- CLASS RuntimeScalarProperty
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
RuntimeScalarProperty::RuntimeScalarProperty( DocumentLocation loc, Identifier id )
: __ScalarProperty( Node::Runtime, loc, id )
{
}

RuntimeScalarProperty::~RuntimeScalarProperty()
{
}

Property *RuntimeScalarProperty::clone( Identifier cloneId )
{
	// Clone runtime makes no sense.
	assert( false );
}

string RuntimeScalarProperty::getEvaledString()
{
	err( "Illegal request for value of runtime property." );
	return "";
}


// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// --- CLASS DynamicScalarAttribute
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
DynamicScalarAttribute::DynamicScalarAttribute( DocumentLocation loc,
												Identifier id,
												Expression *expr )
: Node( Node::Attr, Node::Dynamic, loc )
, _id( id )
, _expr( expr )
{
}

DynamicScalarAttribute::~DynamicScalarAttribute()
{
}

const char *DynamicScalarAttribute::getName()
{
	return _id.getName();
}

DynamicScalarAttribute *DynamicScalarAttribute::clone()
{
	return new DynamicScalarAttribute( getLocation(), _id, _expr->clone() );
}

Expression *DynamicScalarAttribute::getExpression()
{
	return _expr;
}


// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// --- CLASS DynamicScalarProperty
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
DynamicScalarProperty::DynamicScalarProperty( DocumentLocation loc,
											  Identifier id,
											  Expression *initExpr )
: __ScalarProperty( Node::Dynamic, loc, id )
, _initExpr( new Interpreter::ExpressionEvaluator(initExpr) )
{
}

DynamicScalarProperty::~DynamicScalarProperty()
{
}

Property *DynamicScalarProperty::clone( Identifier cloneId )
{
	DynamicScalarProperty *clone = new DynamicScalarProperty( getLocation(),
															  cloneId,
															  getInitExpression()->clone() );
	itfor( DynamicScalarAttributeMap, _attrs, it )
		clone->add( it->second->clone() );

	return baseClone( clone );
}

Expression *DynamicScalarProperty::getInitExpression()
{
	return _initExpr->getExpression();
}

void DynamicScalarProperty::add( DynamicScalarAttribute *attr )
{
	string name = attr->getName();
	if( getAttr(name) )
		attr->err( "Duplicate attribute." );

	_attrs[name] = attr;
	Node::add( attr );
}

DynamicScalarAttributeMap &DynamicScalarProperty::attrs()
{
	return _attrs;
}

DynamicScalarAttribute *DynamicScalarProperty::getAttr( std::string name )
{
	DynamicScalarAttributeMap::iterator it = _attrs.find( name );
	if( it == _attrs.end() )
		return NULL;
	return it->second;
}

string DynamicScalarProperty::getEvaledString()
{
	return _initExpr->evaluate( this );
}


// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// --- CLASS __ContainerProperty
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
__ContainerProperty::__ContainerProperty( Node::Type type, Node::Subtype subtype, DocumentLocation loc, Identifier id )
: Property( type, subtype, loc, id )
{
}

__ContainerProperty::~__ContainerProperty()
{
}

void __ContainerProperty::add( Property *prop )
{
	if( getp(prop->getName()) != NULL )
	{
		
		prop->err( string("Duplicate property name '") + prop->getName() + "' (see " + getp(prop->getName())->getLocation().getDescription() + ")" );
	}

	_props[ prop->getName() ] = prop;
	Node::add( prop );
}

void __ContainerProperty::replace( Property *newProp )
{
	Property *oldProp = getp( newProp->getName() );
	assert( oldProp != NULL );
	delete oldProp;

	_props[ newProp->getName() ] = newProp;
	Node::add( newProp );
}

Property &__ContainerProperty::get( Identifier id )
{
	Property *prop = getp( id );
	if( !prop )
	{
		err( string("No such property: '") + id.getName() + "'" );
	}
	return *prop;
}

Property *__ContainerProperty::getp( Identifier id )
{
	PropertyMap::iterator it = _props.find( id );
	if( it == _props.end() )
		return NULL;
	else
		return it->second;
}

PropertyMap &__ContainerProperty::props()
{
	return _props;
}

__ContainerProperty::operator short()
{
	err( "Expecting Short" ); return -1;
}

__ContainerProperty::operator int()
{
	err( "Expecting Int" ); return -1;
}

__ContainerProperty::operator long()
{
	err( "Expecting Long" ); return -1;
}

__ContainerProperty::operator float()
{
	err( "Expecting Float" ); return -1;
}

__ContainerProperty::operator bool()
{
	err( "Expecting Bool" ); return false;
}

__ContainerProperty::operator string()
{
	err( "Expecting String" ); return "";
}

void __ContainerProperty::dump( ostream &out, string indent )
{
	out << indent << getName() << " (schema=" << getSchema() << ")" << endl;
	out << indent << "{" << endl;
	itfor( PropertyMap, props(), it )
		it->second->dump( out, indent + "  " );
	out << indent << "}" << endl;
}

bool __ContainerProperty::findLocalSymbol( SymbolPath::Element *name, Symbol &sym )
{
	Property *child = getp( name->getText() );

	if( child )
	{
		if( name->next )
		{
			return child->findLocalSymbol( name->next, sym );
		}
		else
		{
			sym.type = Symbol::Property;
			sym.prop = child;

			return true;
		}
	}

	return Property::findLocalSymbol( name, sym );
}


// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// --- CLASS ObjectProperty
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
ObjectProperty::ObjectProperty( Node::Subtype subtype, DocumentLocation loc, Identifier id )
: __ContainerProperty( Node::Object, subtype, loc, id )
{
}

ObjectProperty::ObjectProperty( DocumentLocation loc, Identifier id )
: __ContainerProperty( Node::Object, Node::None, loc, id )
{
}

ObjectProperty::~ObjectProperty()
{
}

void ObjectProperty::add( Property *prop )
{
	assert( !prop->getId().isIndex() );

	__ContainerProperty::add( prop );
}

Property *ObjectProperty::clone( Identifier cloneId )
{
	ObjectProperty *prop = new ObjectProperty( getLocation(),
											   cloneId );

	itfor( PropertyMap, props(), it )
	{
		prop->add( it->second->clone(it->second->getId()) );
	}

	return baseClone( prop );
}


// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// --- CLASS ArrayProperty
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
ArrayProperty::ArrayProperty( DocumentLocation loc, Identifier id )
: __ContainerProperty( Node::Array, Node::None, loc, id )
{
}

ArrayProperty::~ArrayProperty()
{
}

void ArrayProperty::add( Property *prop )
{
	assert( prop->getId().isIndex() );

	__ContainerProperty::add( prop );
}

Property *ArrayProperty::clone( Identifier cloneId )
{
	ArrayProperty *prop = new ArrayProperty( getLocation(),
											 cloneId );

	itfor( PropertyMap, props(), it )
	{
		prop->add( it->second->clone(it->second->getId()) );
	}

	return baseClone( prop );
}


// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// --- CLASS MetaProperty
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
MetaProperty::MetaProperty( DocumentLocation loc,
							Identifier id,
							Expression *expr )
: Node( Node::Attr, Node::Dynamic, loc )
, _id( id )
, _expr( expr )
{
}

MetaProperty::~MetaProperty()
{
}

Identifier MetaProperty::getId()
{
	return _id;
}

Expression *MetaProperty::getExpression()
{
	return _expr;
}


// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// --- CLASS Document
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------

Document::Document( string name, string path )
: ObjectProperty( Node::Document, DocumentLocation(this,0), name )
, _path( path )
{
}

Document::~Document()
{
}

string Document::getPath()
{
	return _path;
}

void Document::addMeta( MetaProperty *prop )
{
	if( _metaprops.find( prop->getId() ) != _metaprops.end() )
		prop->err( "Duplicate meta property" );

	_metaprops[prop->getId()] = prop;
}

MetaPropertyMap &Document::metaprops()
{
	return _metaprops;
}
