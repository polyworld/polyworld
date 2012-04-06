#include "overlay.h"

#include "editor.h"
#include "misc.h"


using namespace std;
using namespace proplib;


// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// --- CLASS Overlay
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------

void Overlay::applyDocument( Document *overlay, DocumentEditor *editor )
{
	_depth = 1;
	apply( *overlay, editor );
}

void Overlay::applyDocument( Document *overlay, int overlayIndex, DocumentEditor *editor )
{
	Property &overlays = overlay->get( "overlays" );
	if( overlay->props().size() > 1 )
		overlay->err( "Expecting only 'overlays' at top-level." );

	_depth = 3;

	Property &overlayClause = overlays.get( overlayIndex );
	if( overlayClause.getType() != Node::Object )
		overlayClause.err( "Expecting Object" );
	
	apply( overlayClause, editor );
}

void Overlay::applyEmbedded( Document *doc, DocumentEditor *editor )
{
	Property &overlay = doc->get( "overlay" );
	_depth = 2;
	apply( overlay, editor );
}

string Overlay::getDocumentPropertyName( Property &overlayProperty )
{
	return overlayProperty.getFullName( _depth );
}

void Overlay::apply( Property &overlayProp, DocumentEditor *editor )
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
