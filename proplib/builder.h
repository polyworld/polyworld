#pragma once

#include <string>

#include "dom.h"

namespace proplib
{
	class SyntaxNode;

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS DocumentBuilder
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	class DocumentBuilder
	{
	public:
		Document *buildDocument( const std::string &path );
		class Document *buildWorldfileDocument( class SchemaDocument *schema, const std::string &path );
		class SchemaDocument *buildSchemaDocument( const std::string &path );
		class OverlayDocument *buildOverlayDocument( const std::string &path );

		class SymbolPath *buildSymbolPath( const std::string &text );
		ConstScalarProperty *buildConstScalarProperty( DocumentLocation loc, Identifier id, const std::string &value );

	private:
		DocumentLocation createLocation( SyntaxNode *node );
		Identifier createId( SyntaxNode *node );

		void buildDocument( SyntaxNode *node, ObjectProperty *obj = NULL );
		MetaProperty *buildMetaProperty( SyntaxNode *node );
		Property *buildProperty( SyntaxNode *node );
		ConstScalarProperty *buildConstScalarProperty( SyntaxNode *node, Identifier id = -1 );
		Property *buildDynamicScalarProperty( SyntaxNode *node, Identifier id = -1 );
		DynamicScalarAttribute *buildDynamicScalarAttribute( SyntaxNode *node );
		ObjectProperty *buildObjectProperty( SyntaxNode *node, Identifier id = -1, ObjectProperty *obj = NULL );
		Property *buildArrayProperty( SyntaxNode *node );
		Enum *buildEnum( SyntaxNode *node );
		Class *buildClass( SyntaxNode *node );
		class Expression *buildExpression( SyntaxNode *node );
		class SymbolPath *buildSymbolPath( SyntaxNode *node );

		std::string _path;
		Document *_doc;
	};
}
