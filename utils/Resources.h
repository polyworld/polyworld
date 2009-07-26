#pragma once

#include <fstream>
#include <string>

class Resources
{
 public:
	static bool loadPolygons( class gpolyobj *poly,
							  std::string name );
	static bool openWorldFile( std::filebuf *fb,
							   std::string filename );

 private:
	static std::string find( std::string name );
};
