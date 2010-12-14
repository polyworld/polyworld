#pragma once

#include <istream>
#include <list>
#include <map>
#include <stack>
#include <string>

namespace PropertyFile
{
	// forward decl
	class Document;

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS Identifier
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	class Identifier
	{
	public:
		Identifier( const std::string &name );
		Identifier( int index );
		Identifier( unsigned int index );
		Identifier( size_t index );
		Identifier( const char *name);
		Identifier( const Identifier &other );

		~Identifier();

		const char *getName() const;

	private:
		std::string name;
	};

	bool operator<( const Identifier &a, const Identifier &b );

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS DocumentLocation
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	class DocumentLocation
	{
	public:
		DocumentLocation( Document *doc, unsigned int lineno );

		Document *getDocument();
		std::string getDescription();

		void err( std::string msg );

	private:
		friend class Parser;

		Document *doc;
		unsigned int lineno;
	};

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS Property
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	class Property
	{
	public:
		enum Type
		{
			SCALAR,
			OBJECT
		};

		Property( DocumentLocation loc, Identifier id, bool _isArray = false );
		Property( DocumentLocation loc, Identifier id, const char *val );
		virtual ~Property();

		const char *getName();

		Property &get( Identifier id );
		Property *getp( Identifier id );

		Property *find( Identifier id );

		operator int();
		operator float();
		operator bool();
		operator std::string();

		void add( Property *prop );

		void err( std::string message );
		void dump( std::ostream &out, const char *indent = "" );

	private:
		// We don't allow the copy constructor
		Property( const Property &copy ) :loc(NULL,-1), id("") { throw "copy not supported."; }

		std::string getScalarValue();

		friend class Parser;
		friend class Schema;

		Property *parent;
		Property *symbolSource;
		DocumentLocation loc;
		Type type;
		Identifier id;
		bool isArray;
		bool isExpr;
		bool isEval;

		typedef std::map<Identifier, Property *> PropertyMap;
		union
		{
			char *sval;
			PropertyMap *oval;
		};
	};

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS Document
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	class Document : public Property
	{
	public:
		Document( const char *name );
		virtual ~Document();
	};

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS Parser
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	class Parser
	{
	public:
		typedef std::list<char *> CStringList;
		typedef std::list<std::string> StringList;

		static Document *parse( const char *path );

		static bool isValidIdentifier( const std::string &text );
		static void scanIdentifiers( const std::string &expr, StringList &ids );

		static bool parseInt( const std::string &text, int *result );
		static bool parseFloat( const std::string &text, float *result );
		static bool parseBool( const std::string &text, bool *result );

	private:
		typedef std::stack<Property *> PropertyStack;

		static char *readline( std::istream &in,
							   DocumentLocation &loc );
		static void tokenize( DocumentLocation &loc, char *line, CStringList &list );
		static void processLine( Document *doc,
								 DocumentLocation &loc,
								 PropertyStack &propertyStack,
								 CStringList &tokens );
	};

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS Schema
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	class Schema
	{
	public:
		static void apply( Document *docSchema, Document *docValues );

	private:
		static void validateChildren( Property &propSchema, Property &propValues );
		static void validateProperty(  Property &propSchema, Property &propValues );
	};
};
