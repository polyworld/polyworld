#pragma once

#include <fstream>
#include <string>

namespace proplib
{
	class Document;
}

class Resources
{
 public:
	static bool loadPolygons( class gpolyobj *poly,
							  std::string name );
	static void parseConfiguration( proplib::Document **ret_docValus,
									proplib::Document **ret_docSchema,
									std::string valuesPath,
									std::string schemaPath );

 private:
	static std::string find( std::string name );
	static bool exists( std::string path );
};
