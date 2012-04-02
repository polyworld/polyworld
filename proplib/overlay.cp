#include "overlay.h"

#include "editor.h"
#include "misc.h"


using namespace std;
using namespace proplib;


// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// --- CLASS OverlayDocument
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------

OverlayDocument::OverlayDocument( string name, string path )
: Document( name, path )
{
}

OverlayDocument::~OverlayDocument()
{
}

void OverlayDocument::validate()
{
	if( size() != 1 )
		err( "Overlay document should consist only of 'overlays' at top-level." );

	Property &overlays = get( "overlays" );
	if( overlays.getType() != Node::Array )
		overlays.err( "Should be an array." );

	itfor( PropertyMap, overlays.elements(), it )
	{
		if( it->second->getType() != Node::Object )
			it->second->err( "Expecting Object" );
	}
}

void OverlayDocument::apply( int index, DocumentEditor *editor )
{
	apply( getClause(index), editor );
}

Property &OverlayDocument::getClause( int index )
{
	return get( "overlays" ).get( index );
}

string OverlayDocument::getDocumentPropertyName( Property &overlayProperty )
{
	return overlayProperty.getFullName( 3 );
}

void OverlayDocument::apply( Property &overlayProp, DocumentEditor *editor )
{
	if( overlayProp.getType() == Node::Scalar )
	{
		if( overlayProp.getSubtype() != Node::Const )
			overlayProp.err( "Only const scalars currently supported for overlays" );

		string name = getDocumentPropertyName( overlayProp );

		ConstScalarProperty &prop = dynamic_cast<ConstScalarProperty &>( overlayProp );
		string value = prop.getExpression()->toString();

		editor->set( name, value );
	}
	else
	{
		itfor( PropertyMap, overlayProp.props(), it )
			apply( *it->second, editor );
	}
}
