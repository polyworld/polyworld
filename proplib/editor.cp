#include "editor.h"

#include <assert.h>
#include <stdlib.h>

#include <iostream>

#include "builder.h"
#include "dom.h"
#include "schema.h"

using namespace std;
using namespace proplib;

static void err( string msg )
{
	cerr << msg << endl;
	exit( 1 );
}

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// --- CLASS DocumentEditor
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
DocumentEditor::DocumentEditor( SchemaDocument *schema, Document *doc )
: _schema( schema )
, _doc( doc )
{
}

DocumentEditor::DocumentEditor( Document *doc )
: _schema( NULL )
, _doc( doc )
{
}

DocumentEditor::~DocumentEditor()
{
}

void DocumentEditor::setMeta( string name, string value )
{
	proplib::DocumentBuilder builder;
	_doc->metaprops()[Identifier(name)] = builder.buildMetaProperty( DocumentLocation(_doc),
																	 Identifier(name),
																	 " " + value );
}

void DocumentEditor::set( string symbolPathString, string valueString )
{
	proplib::DocumentBuilder builder;
	proplib::SymbolPath *symbolPath = builder.buildSymbolPath( symbolPathString );	

	if( _schema )
		_schema->makePathDefaults( _doc, symbolPath );

	Symbol sym;
	if( !_doc->findSymbol(symbolPath, sym) )
	{
		if( _schema )
			err( string("Cannot find ") + symbolPathString + " in " + _doc->getPath() + " or " + _schema->getPath() );
		else
			err( string("Cannot find ") + symbolPathString + " in " + _doc->getPath() );
	}

	if( sym.type != Symbol::Property )
	{
		err( string("Only properties can be edited. (") + symbolPathString + ")" );
	}

	set( sym.prop, valueString );
}

void DocumentEditor::set( Property *prop, std::string valueString )
{
	if( prop->getSubtype() == proplib::Node::Runtime )
	{
		err( string("Runtime properties cannot be edited. (") + prop->getFullName(1) + ")" );
	}

	proplib::DocumentBuilder builder;
	Property *newProp = builder.buildProperty( prop->getLocation(),
											   prop->getName(),
											   " " + valueString );
	dynamic_cast<__ContainerProperty *>( prop->getParent() )->replace( newProp );
}

void DocumentEditor::move( Property *prop, Property *newParent_, bool modifyOldParent )
{
	assert( !modifyOldParent ); // must be implemented

	__ContainerProperty *newParent = dynamic_cast<__ContainerProperty *>( newParent_ );
	if( !newParent )
		newParent_->err( "[Edit] Expecting Object or Array" );

	newParent->add( prop );
}

void DocumentEditor::remove( Property *prop )
{
	if( prop->getParent() )
		prop->getParent()->props().erase( prop->getId() );
}

void DocumentEditor::removeChildren( Property *prop )
{
	prop->props().clear();
}

void DocumentEditor::rename( Property *prop, const char *newName, bool modifyParent )
{
	assert( !modifyParent );
	
	prop->_id = newName;
}
