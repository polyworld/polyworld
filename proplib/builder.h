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

		class SymbolPath *buildSymbolPath( const std::string &text );
		MetaProperty *buildMetaProperty( DocumentLocation loc, Identifier id, const std::string &value );
		Property *buildProperty( DocumentLocation loc, Identifier id, const std::string &value );

	private:
		DocumentLocation createLocation( SyntaxNode *node );
		Identifier createId( SyntaxNode *node );

		void buildDocument( SyntaxNode *node, ObjectProperty *rootContainer = NULL );
		MetaProperty *buildMetaProperty( DocumentLocation loc, Identifier id, SyntaxNode *nodeValue );
		Property *buildProperty( SyntaxNode *node );
		Property *buildProperty( DocumentLocation loc, Identifier id, SyntaxNode *nodeValue );
		Property *buildConstScalarProperty( DocumentLocation loc, Identifier id, SyntaxNode *nodeValue );
		Property *buildDynamicScalarProperty( DocumentLocation loc, Identifier id, SyntaxNode *nodeValue );
		ObjectProperty *buildObjectProperty( DocumentLocation loc, Identifier id, SyntaxNode *nodeValue );
		ObjectProperty *buildObjectProperty( ObjectProperty *obj, SyntaxNode *nodeObject );
		Property *buildArrayProperty( DocumentLocation loc, Identifier id, SyntaxNode *nodeValue );
		DynamicScalarAttribute *buildDynamicScalarAttribute( SyntaxNode *node );
		Enum *buildEnum( SyntaxNode *node );
		Class *buildClass( SyntaxNode *node );
		class Expression *buildExpression( SyntaxNode *node );
		class SymbolPath *buildSymbolPath( SyntaxNode *node );

		std::string _path;
		Document *_doc;
	};
}
