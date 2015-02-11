#pragma once

#include <list>
#include <map>
#include <string>

#include "datalib.h"

class TSimulation;

// Putting this macro in a class declaration provides the dynamic properties evaluation function access
// to private members.
#define PROPLIB_CPP_PROPERTIES \
	friend void proplib::CppProperties_Init( proplib::CppProperties::UpdateContext *context ); \
	friend void proplib::CppProperties_Update( proplib::CppProperties::UpdateContext *context );

namespace proplib
{
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS CppProperties
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	class CppProperties
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
			enum Type
			{
				Dynamic,
				Runtime
			};
			std::string name;
			Type type;
			datalib::Type valueType;
			void *value;
			void *state;

			const char *toString();

			char __toStringBuf[256];
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
		struct CppPropertyInfo
		{
			int metadataIndex;
			int stage;
		};

		typedef std::list<class __ScalarProperty *> CppPropertyList;
		typedef std::list<class DynamicScalarProperty *> DynamicPropertyList;
		typedef std::list<class RuntimeScalarProperty *> RuntimePropertyList;
		typedef std::map<class Property *, CppPropertyInfo> CppPropertyInfoMap;

		static void generateLibrarySource();
		static void generateStateStructs( std::ofstream &out, DynamicPropertyList &dynamicProperties );
		static void generateMetadata( std::ofstream &out,
									  CppPropertyList &cppProperties, 
									  CppPropertyInfoMap &infoMap );
		static void generateInitSource( std::ofstream &out,
										CppPropertyList &cppProperties,
										DynamicPropertyList &dynamicProperties,
										CppPropertyInfoMap &infoMap );
		static std::string generateInitFunctionBody( class DynamicScalarProperty *prop,
													 DynamicPropertyList &antecedents,
													 CppPropertyInfoMap &infoMap );
		static void generateUpdateSource( std::ofstream &out,
										  DynamicPropertyList &dynamicProperties,
										  CppPropertyInfoMap &infoMap );
		static std::string generateUpdateFunctionBody( class DynamicScalarProperty *prop,
													   DynamicPropertyList &antecedents,
													   CppPropertyInfoMap &infoMap );

		static std::string getStateStructName( class DynamicScalarProperty *prop );
		static std::string getDataLibType( class __ScalarProperty *prop );
		static std::string getCppType( class Property *prop );
		static std::string getCppSymbol( class Property *prop );
		static std::string getMetadataLValue( class Property *prop, CppPropertyInfoMap &infoMap );

		static void getCppProperties( class Property *container,
									  CppPropertyList &result_all,
									  DynamicPropertyList &result_dynamic,
									  RuntimePropertyList &result_runtime );
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
	void CppProperties_Init( CppProperties::UpdateContext *context );
	void CppProperties_Update( CppProperties::UpdateContext *context );
}
