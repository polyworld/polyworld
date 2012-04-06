#pragma once

#include <string>

namespace proplib
{

	class WorldfileConverter
	{
	public:
		static bool isV1( std::string path );
		static void convertV1SyntaxToV2( std::string pathIn, std::string pathOut );
		static void convertV1PropertiesToV2( class DocumentEditor *editor, class Document *doc );
		static void convertDeprecatedV2Properties( class DocumentEditor *editor, class Document *doc );
	};

}
