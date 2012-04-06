#pragma once

#include <iostream>
#include <vector>

#include "parser.h"

namespace proplib
{
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS DocumentWriter
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	class DocumentWriter
	{
	public:
		DocumentWriter( std::ostream &out );
		void write( class Document *doc );
		void writeScalarNames( class Document *doc, int depthStart );

	private:
		Token *findPrevToken( class Node *node, Token::Type type, bool search = true );
		Token *findBeginToken( class Node *node, Token::Type type, bool search = true );
		Token *findEndToken( class Node *node, Token::Type type );
		Token *findNextToken( class Node *node, Token::Type type, bool search = true );

		void writeToken( Token *tok );

		void sortMeta( class Document *doc, std::vector<class MetaProperty *> &result );
		void sortChildren( class __ContainerProperty *prop, std::vector<class Property *> &result );
		void sortAttributes( class DynamicScalarProperty *prop, std::vector<class DynamicScalarAttribute *> &result );
		

		void writeMetaProperties( class Document *doc );
		void writeProperties( class ObjectProperty *object );
		void writeProperty( class Property *prop );
		void writeConst( class ConstScalarProperty *prop );
		void writeDynamic( class DynamicScalarProperty *prop );
		void writeArray( class ArrayProperty *prop );
		void writeObject( class ObjectProperty *prop );

		void writeExpression( class Expression *expr );

		void writeScalarNames( class Property *prop, int depthStart );

		Document *_prevPropertyDoc;
		Document *_doc;
		std::ostream &_out;
	};

}
