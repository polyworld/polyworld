#pragma once

#include <string>

namespace proplib
{
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS DocumentEditor
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	class DocumentEditor
	{
	public:
		DocumentEditor( class SchemaDocument *schema, class Document *doc );
		DocumentEditor( class Document *doc );

		virtual ~DocumentEditor();

		void setMeta( std::string name, std::string value );
		void set( std::string symbolPath, std::string value );
		void set( class Property *prop, std::string value );
		void move( class Property *prop, class Property *newParent, bool modifyOldParent );
		void remove( class Property *prop );
		void removeChildren( class Property *prop );
		void rename( class Property *prop, const char *newName, bool modifyParent ); 

	private:
		class SchemaDocument *_schema;
		class Document *_doc;
	};
}
