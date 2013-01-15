#include "schema.h"

#include <assert.h>

#include <string>
#include <sstream>

#include "builder.h"
#include "editor.h"
#include "misc.h"
#include "overlay.h"

using namespace std;
using namespace proplib;

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// --- CLASS SchemaDocument
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
set<string> SchemaDocument::standardTypes;

static class SchemaDocumentInit
{
 public:
	SchemaDocumentInit()
	{
		SchemaDocument::standardTypes.insert( "Int" );
		SchemaDocument::standardTypes.insert( "Float" );
		SchemaDocument::standardTypes.insert( "Bool" );
		SchemaDocument::standardTypes.insert( "Object" );
		SchemaDocument::standardTypes.insert( "Array" );
		SchemaDocument::standardTypes.insert( "String" );
		SchemaDocument::standardTypes.insert( "Enum" );
	}
} _schemaDocumentInit;

SchemaDocument::SchemaDocument( std::string name, std::string path )
: Document( name, path )
{
}

SchemaDocument::~SchemaDocument()
{
}

void SchemaDocument::init()
{
	injectClasses( this );
}

void SchemaDocument::apply( Document *doc )
{
	parseDefaults( doc );

	if( doc->getp("overlay") )
	{
		DocumentEditor editor( this, doc );
		Overlay overlay;
		overlay.applyEmbedded( doc, &editor );
	}

	normalize( *this, *doc );
	validate( *doc );
}

void SchemaDocument::makePathDefaults( Document *values, SymbolPath *symbolPath )
{
	parseDefaults( values );
	makePathDefaults( *this, *values, symbolPath->head );
}

void SchemaDocument::injectClasses( Property *prop_ )
{
	if( prop_->getType() == Node::Object )
	{
		ObjectProperty *prop = dynamic_cast<ObjectProperty *>( prop_ );
		Property *propType = prop->getp( "type" );
		if( propType )
		{
			string type = *propType;
			if( standardTypes.find(type) == standardTypes.end() )
			{
				DocumentBuilder builder;
				SymbolPath *symbolPath = builder.buildSymbolPath( type );
				Symbol sym;
				if( !prop->findSymbol(symbolPath, sym) )
					propType->err( "Invalid type name." );

				if( sym.type != Symbol::Class )
					propType->err( "Expecting class name." );

				DocumentEditor editor( this );
				
				editor.set( propType, "Object" );
				prop->add( sym.klass->getDefinition()->clone("properties") );
			}
		}

		itfor( PropertyMap, prop->props(), it )
			injectClasses( it->second );
	}
}

void SchemaDocument::normalize( ObjectProperty &propertySchema, Property &propertyValue )
{
	propertyValue.setSchema( &propertySchema );

	string type = propertySchema.get( "type" );

	if( type == "Object" )
	{
		if( propertyValue.getType() != Node::Object )
			propertyValue.err( "Schema specifies this property as an Object." );

		normalizeObject( propertySchema,
						 dynamic_cast<ObjectProperty &>(propertyValue) );
	}
	else if( type == "Array" )
	{
		if( propertyValue.getType() != Node::Array )
			propertyValue.err( "Schema specifies this property as an Array." );

		normalizeArray( propertySchema,
						dynamic_cast<ArrayProperty &>(propertyValue) );
	}
	else if( type == "Enum" )
	{
		propertyValue.addEnum( propertySchema.getEnum("Values") );
	}
}

void SchemaDocument::normalizeObject( ObjectProperty &objectSchema, ObjectProperty &objectValue )
{
	Property &propertiesSchema = objectSchema.get( "properties" );
	if( propertiesSchema.getType() != Node::Object )
		propertiesSchema.err( "Invalid schema. 'properties' must be an Object." );

	// ---
	// --- Inject Default & Runtime Properties
	// ---
	itfor( PropertyMap, propertiesSchema.props(), it )
	{
		Property &propertySchema_ = *it->second;
		if( propertySchema_.getType() != Node::Object )
			propertySchema_.err( "Invalid schema. Expecting Object describing a property." );

		ObjectProperty &propertySchema = dynamic_cast<ObjectProperty &>(propertySchema_);

		Property *propRuntime = propertySchema.getp( "runtime" );
		if( propRuntime && (bool)*propRuntime )
		{
			if( objectValue.getp(propertySchema.getName()) )
				objectValue.get(propertySchema.getName()).err( "Cannot assign value to runtime property." );

			objectValue.add( new RuntimeScalarProperty(propertySchema.getLocation(),
													   propertySchema.getId()) );
		}
		else
		{
			if( objectValue.getp(propertySchema.getName()) == NULL )
			{
				Property *propDefault = createDefault( propertySchema );

				if( propDefault == NULL )
				{
					Property *propOptional = propertySchema.getp( "optional" );
					if( !propOptional || !(bool)*propOptional )
						objectValue.err( string("Missing property ") + propertySchema.getName() );
				}
				else
				{
					objectValue.add( propDefault );
				}
			}
		}
	}

	// ---
	// --- Normalize Children
	// ---
	itfor( PropertyMap, propertiesSchema.props(), it )
	{
		Property &childSchema_ = *it->second;

		if( childSchema_.getType() != Node::Object )
			childSchema_.err( "Expecting property schema definition, which should be an object." );

		ObjectProperty &childSchema = dynamic_cast<ObjectProperty &>( childSchema_ );
		Property *childValue = objectValue.getp( childSchema.getName() );

		if( childValue )
			normalize( childSchema, *childValue );
		else
			assert( (bool)childSchema.get("optional") );
	}
}

void SchemaDocument::normalizeArray( ObjectProperty &propertySchema, ArrayProperty &propertyValue )
{
	Property &elementSchema_ = propertySchema.get( "element" );
	if( !elementSchema_.getType() == Node::Object )
		elementSchema_.err( "Illegal schema. 'element' should be an Object." );

	ObjectProperty &elementSchema = dynamic_cast<ObjectProperty &>(elementSchema_);

	// ---
	// --- Normalize Children
	// ---
	itfor( PropertyMap, propertyValue.elements(), it )
	{
		normalize( elementSchema, *it->second );
	}
}

void SchemaDocument::validate( Property &propertyValue )
{
	if( (0 == strcmp(propertyValue.getName(), "overlay"))
		&& (propertyValue.getParent()->getSubtype() == Node::Document) )
	{
		return;
	}

	if( propertyValue.getSchema() == NULL )
		propertyValue.err( "No definition in schema." );

	if( propertyValue.getSubtype() == Node::Dynamic )
	{
		if( !propertyValue.hasBinding() )
			propertyValue.err( "Properties without 'cppsym' may not use dyn expressions." );
	}

	if( propertyValue.getSubtype() == Node::Runtime )
	{
		if( !propertyValue.hasBinding() )
			propertyValue.err( "Runtime properties must define 'cppsym'." );

		// Runtime properties are controlled by the simulation.
		return;
	}

	string type = propertyValue.getSchema()->get( "type" );

	if( type == "Array" )
	{
		validateArray( propertyValue );
	}
	else if( type == "Object" )
	{
		validateObject( propertyValue );
	}
	else if( type == "Enum" )
	{
		validateEnum( propertyValue );
	}
	else
	{
		validateScalar( propertyValue );
	}
}

void SchemaDocument::validateScalar( Property &propertyValue )
{
	validateScalar( *propertyValue.getSchema(), propertyValue );
}

void SchemaDocument::validateScalar( ObjectProperty &schema, Property &value )
{
	string type = schema.get( "type" );

	if( type == "Int" )
	{
		(int)value;
	}
	else if( type == "Float" )
	{
		(float)value;
	}
	else if( type == "Bool" )
	{
		(bool)value;
	}
	else if( type == "String" )
	{
		(string)value;
	}
	else
	{
		schema.get("type").err( "Invalid scalar type." );
	}

	itfor( PropertyMap, schema.props(), it )
	{
		Property &attr = *it->second;
		string attrName = attr.getName();

		// --
		// -- CONSTRAINT
		// --
		#define CONSTRAINT( OP, ERRDESC )								\
		{																\
			bool valid = true;											\
																		\
			if( type == "Int" )											\
				valid = (int)value OP (int)attr;						\
			else if( type == "Float" )									\
				valid = (float)value OP (float)attr;					\
			else														\
				attr.err( string("'") + attrName + "' is not valid for type " + type ); \
																		\
			if( !valid )												\
				value.err( (string)value + " " ERRDESC " " + attrName + " " + (string)attr ); \
		}

		if( attrName == "min" )
			CONSTRAINT( >=, "<" )

		else if( attrName == "exmin" )
			CONSTRAINT( >, "<=" )

		else if( attrName == "max" )
			CONSTRAINT( <=, ">" )

		else if( attrName == "exmax" )
			CONSTRAINT( <, ">=" )

		else
			validateCommonAttribute( attr, value );

		#undef CONSTRAINT
	}
}

void SchemaDocument::validateEnum( Property &propertyValue )
{
	ObjectProperty &propertySchema = *propertyValue.getSchema();

	proplib::Enum *enum_ = propertySchema.getEnum( "Values" );

	if( !enum_->contains((string)propertyValue) )
	{
		Property *scalarSchema = propertySchema.getp( "scalar" );
		if( !scalarSchema )
			propertyValue.err( "Invalid enum value." );
		if( scalarSchema->getType() != Node::Object )
			scalarSchema->err( "Invalid schema. Scalar definition must be object." );

		validateScalar( dynamic_cast<ObjectProperty &>(*scalarSchema),
						propertyValue );
	}

	itfor( PropertyMap, propertySchema.props(), it )
	{
		Property &attr = *it->second;
		string attrName = attr.getName();

		if( attrName == "scalar" )
		{
			// no-op
		}
		else
		{
			validateCommonAttribute( *it->second, propertyValue );
		}
	}
}

void SchemaDocument::validateObject( Property &propertyValue )
{
	if( propertyValue.getType() != Node::Object )
		propertyValue.err( "Expecting Object." );

	itfor( PropertyMap, propertyValue.getSchema()->props(), it )
	{
		Property &attr = *it->second;
		string attrName = attr.getName();

		if( attrName == "properties" )
		{
			// no-op
		}
		else
		{
			validateCommonAttribute( *it->second, propertyValue );
		}
	}

	itfor( PropertyMap, propertyValue.props(), it )
	{
		validate( *it->second );
	}
}

void SchemaDocument::validateArray( Property &propertyValue )
{
	if( propertyValue.getType() != Node::Array )
		propertyValue.err( "Expecting Array." );

	itfor( PropertyMap, propertyValue.getSchema()->props(), it )
	{
		Property &attr = *it->second;
		string attrName = attr.getName();

		// --
		// -- CONSTRAINT
		// --
		#define CONSTRAINT( OP, ERRDESC )										\
		{															            \
			bool valid = (int)propertyValue.props().size() OP (int)attr;        \
																		        \
			if( !valid )												        \
				propertyValue.err( string("Element count ")+ERRDESC+" "+attrName+" " + (string)attr ); \
		}

		if( attrName == "min" )
			CONSTRAINT( >=, "<" )

		else if( attrName == "exmin" )
			CONSTRAINT( >, "<=" )

		else if( attrName == "max" )
			CONSTRAINT( <=, ">" )

		else if( attrName == "exmax" )
			CONSTRAINT( <, ">=" )

		else if( attrName == "element" )
		{
			// no-op
		}
		else
			validateCommonAttribute( attr, propertyValue );

		#undef CONSTRAINT
	}

	itfor( PropertyMap, propertyValue.elements(), it )
	{
		validate( *it->second );
	}
}

void SchemaDocument::validateCommonAttribute( Property &attr, Property &value )
{
	string attrName = attr.getName();

	if( attrName == "assert" )
	{
		if( !(bool)attr )
			value.err( string("Failed assertion at ") + attr.getLocation().getDescription() );
	}
	else if( attrName == "cpptype"
			 || attrName == "cppsym"
			 || attrName == "type"
			 || attrName == "default"
			 || attrName == "defaults"
			 || attrName == "optional" )
	{
		// no-op
	}
	else
	{
		attr.err( "Invalid schema attribute." );
	}
}

void SchemaDocument::makePathDefaults( ObjectProperty &schema, __ContainerProperty &value, SymbolPath::Element *pathElement )
{
	string type = schema.get( "type" );
	string childName = pathElement->getText();

	if( type == "Array" )
	{
		if( pathElement->next == NULL )
			schema.err( "Unexpected array index as end of symbol path" );
		
		if( value.getType() != Node::Array )
			value.err( "Expecting array" );

		Property &schemaChild = schema.get( "element" );
		Property &valueChild = value.get( childName );

		if( valueChild.getType() == Node::Scalar )
			valueChild.err( "Unexpected scalar" );

		makePathDefaults( dynamic_cast<ObjectProperty &>(schemaChild),
						  dynamic_cast<__ContainerProperty &>(valueChild),
						  pathElement->next );
	}
	else if( type == "Object" )
	{
		Property &schemaChild = schema.get( "properties" ).get( childName );

		if( value.getp(childName) == NULL )
		{
			Property *defaultProperty = createDefault( schemaChild );
			if( defaultProperty == NULL )
				schema.err( string("No default for ") + childName );

			value.add( defaultProperty );
		}

		if( pathElement->next == NULL )
			return;

		Property &valueChild = value.get( childName );

		if( valueChild.getType() == Node::Scalar )
			valueChild.err( "Unexpected scalar" );

		makePathDefaults( dynamic_cast<ObjectProperty &>(schemaChild),
						  dynamic_cast<__ContainerProperty &>(valueChild),
						  pathElement->next );
		
	}
	else
	{
		value.err( "Expecting Object or Array" );
	}
}

void SchemaDocument::parseDefaults( Document *doc )
{
	_defaults.clear();

	if( doc->hasMeta("@defaults") )
	{
		string defaults = doc->getMeta("@defaults")->getValue();
		istringstream in( defaults );

		while( !in.eof() )
		{
			string value;
			in >> value;
			assert( !value.empty() );

			char last = value[ value.length() - 1 ];
			if( last == '.' )
				_defaults.push_back( value.substr(0, value.length() - 1) );
			else if( isdigit(last) )
			{
				const char *numstr;
				for( numstr = value.c_str() + value.length() - 1;
					 isdigit(*numstr);
					 numstr-- )
				{}
				numstr++;

				string prefix = value.substr( 0, numstr - value.c_str() );
				
				int num = atoi( numstr );
				assert( num < 200 ); // sanity check.
				for( ; num >= 0; num-- )
				{
					char buf[ 128 ];
					sprintf( buf, "%s%d", prefix.c_str(), num );

					_defaults.push_back( buf );
				}
			}
			else
				_defaults.push_back( value );
		}
	}
}

Property *SchemaDocument::createDefault( Property &schema )
{
	Property *propDefault = schema.getp( "default" );
	if( !propDefault )
	{
		Property *propDefaults = schema.getp( "defaults" );
		if( propDefaults )
		{
			itfor( list<string>, _defaults, it )
			{
				propDefault = propDefaults->getp( *it );
				if( propDefault )
					break;
			}

			if( !propDefault )
				propDefault = propDefaults->getp( "default" );
		}
	}
	else if( schema.getp("defaults") )
		propDefault->err( "If 'defaults' exists, 'default' must be a child of it." );

	if( !propDefault )
		return NULL;
	else
		return propDefault->clone( schema.getId() );
}
