#pragma once

#include <list>
#include <map>
#include <string>

#include "datalib.h"

class TSimulation;

// Putting this macro in a class declaration provides the dynamic properties evaluation function access
// to private members.
#define PROPLIB_DYNAMIC_PROPERTIES \
	friend void proplib::DynamicProperties_Init( proplib::DynamicProperties::UpdateContext *context ); \
	friend void proplib::DynamicProperties_Update( proplib::DynamicProperties::UpdateContext *context );

namespace proplib
{
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS DynamicProperties
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	class DynamicProperties
	{
	public:
		// ----------------------------------------------------------------------
		// ----------------------------------------------------------------------
		// --- CLASS UpdateContext
		// ----------------------------------------------------------------------
		// ----------------------------------------------------------------------
		class UpdateContext
		{
		public:
			UpdateContext( TSimulation *sim ) { this->sim = sim; }
			
			TSimulation *sim;
		};

		// ----------------------------------------------------------------------
		// ----------------------------------------------------------------------
		// --- CLASS PropertyMetadata
		// ----------------------------------------------------------------------
		// ----------------------------------------------------------------------
		struct PropertyMetadata
		{
			std::string name;
			datalib::Type type;
			void *value;
			bool valueChanged;
			void *state;
		};

		// ----------------------------------------------------------------------
		// ----------------------------------------------------------------------
		// --- API
		// ----------------------------------------------------------------------
		// ----------------------------------------------------------------------
		static void init( class Document *doc, UpdateContext *context );
		static void update();
		static void getMetadata( PropertyMetadata **metadata, int *count );

	private:
		typedef std::list<class DynamicScalarProperty *> DynamicPropertyList;
		typedef std::map<class DynamicScalarProperty *, int> DynamicPropertyIndexMap;

		static void generateLibrarySource();
		static void generateStateStructs( std::ofstream &out, DynamicPropertyList &dynamicProperties );
		static void generateMetadata( std::ofstream &out,
									  DynamicPropertyList &dynamicProperties, 
									  DynamicPropertyIndexMap &indexMap );
		static void generateInitSource( std::ofstream &out,
										DynamicPropertyList &dynamicProperties,
										DynamicPropertyIndexMap &indexMap );
		static std::string generateInitFunctionBody( class DynamicScalarProperty *prop,
													 DynamicPropertyList &antecedents,
													 DynamicPropertyIndexMap &indexMap );
		static void generateUpdateSource( std::ofstream &out,
										  DynamicPropertyList &dynamicProperties,
										  DynamicPropertyIndexMap &indexMap );
		static std::string generateUpdateFunctionBody( class DynamicScalarProperty *prop,
													   DynamicPropertyList &antecedents,
													   DynamicPropertyIndexMap &indexMap );

		static std::string getStateStructName( DynamicScalarProperty *prop );
		static std::string getDataLibType( DynamicScalarProperty *prop );
		static std::string getCppType( class Property *prop );
		static std::string getCppSymbol( class Property *prop );
		static std::string getMetadataLValue( class Property *prop, DynamicPropertyIndexMap &indexMap );

		static void getDynamicProperties( Property *container, DynamicPropertyList &result );
		static void sortDynamicProperties( DynamicPropertyList &properties,
										   std::map<DynamicScalarProperty *, DynamicPropertyList> &prop2antecedents );


		typedef void (*LibraryUpdate)( UpdateContext * );
		static LibraryUpdate _update;
		typedef void (*LibraryGetMetadata)( PropertyMetadata **, int * );
		static LibraryGetMetadata _getMetadata;
		static class Document *_doc;

		friend class __StateObject;
		static UpdateContext *_context;
	};

	// These functions are implemented by the generated code.
	void DynamicProperties_Init( DynamicProperties::UpdateContext *context );
	void DynamicProperties_Update( DynamicProperties::UpdateContext *context );
}
