#pragma once

#include <list>
#include <set>

#include "dom.h"

namespace proplib
{
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS SchemaDocument
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	class SchemaDocument : public Document
	{
	private:
		friend class DocumentBuilder;
		SchemaDocument( std::string name, std::string path );
		void init();

	public:
		static std::set<std::string> standardTypes;

		virtual ~SchemaDocument();

		void apply( Document *doc );
		void makePathDefaults( Document *values, SymbolPath *symbolPath );

	private:
		void injectClasses( Property *prop );

		void normalize( ObjectProperty &propertySchema, Property &propertyValue );
		void normalizeObject( ObjectProperty &propertySchema, ObjectProperty &propertyValue );
		void normalizeArray( ObjectProperty &propertySchema, ArrayProperty &propertyValue );

		void validate( Property &propertyValue );
		void validateScalar( Property &propertyValue );
		void validateScalar( ObjectProperty &schema, Property &value );
		void validateEnum( Property &propertyValue );
		void validateObject( Property &propertyValue );
		void validateArray( Property &propertyValue );
		void validateCommonAttribute( Property &attr, Property &value );

		void makePathDefaults( ObjectProperty &schema, __ContainerProperty &value, SymbolPath::Element *pathElement );

		void parseDefaults( Document *doc );
		Property *createDefault( Property &schema );

		std::list<std::string> _defaults;
	};
}
