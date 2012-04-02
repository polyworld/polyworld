#pragma once

#include "dom.h"

namespace proplib
{
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS OverlayDocument
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	class OverlayDocument : public Document
	{
	public:
		OverlayDocument( std::string name, std::string path );
		virtual ~OverlayDocument();

		void validate();
		void apply( int index, class DocumentEditor *editor );
		Property &getClause( int index );
		std::string getDocumentPropertyName( Property &overlayProperty );

	private:
		void apply( Property &overlayProp, class DocumentEditor *editor );
	};
}
