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
	static void parseWorldFile( proplib::Document **ret_docWorldFile,
								proplib::Document **ret_docSchema,
								const char *argWorldfilePath);

 private:
	static std::string find( std::string name );
	static bool exists( std::string path );
};
