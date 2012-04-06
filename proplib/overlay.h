#pragma once

#include "dom.h"

namespace proplib
{
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS Overlay
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	class Overlay
	{
	public:
		void applyDocument( Document *overlay, class DocumentEditor *editor );
		void applyDocument( Document *overlay, int overlayIndex, class DocumentEditor *editor );
		void applyEmbedded( Document *doc, class DocumentEditor *editor );

	private:
		void apply( Property &overlayProp, class DocumentEditor *editor );

		std::string getDocumentPropertyName( Property &overlayProperty );

		int _depth;
	};
}
