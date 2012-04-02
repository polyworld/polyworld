#pragma once

#include <fstream>
#include <string>

namespace proplib { class Document; class SchemaDocument; }

class Resources
{
 public:
	static bool loadPolygons( class gpolyobj *poly,
							  std::string name );

 private:
	static std::string find( std::string name );
};
