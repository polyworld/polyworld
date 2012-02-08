#pragma once

#include <istream>
#include <list>
#include <map>
#include <stack>
#include <string>
#include <vector>

namespace proplib
{
	// forward decl
	class Document;

	typedef std::vector<char *> CStringList;
	typedef std::list<std::string> StringList;

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS Interpreter
	// ---
	// --- Manages python interpreter.
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	class Interpreter
	{
	public:
		static void init();
		static void dispose();

	private:
		friend class Property;

		typedef std::map<std::string, std::string> SymbolTable;

		static bool eval( const char *expr,
						  SymbolTable &symbols,
						  char *result, size_t result_size );

	private:
		static bool alive;
	};

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS Identifier
	// ---
	// --- This represents either a property name or an array index. The API
	// --- uses this so it doesn't have to define separate methods for
	// --- strings and integers.
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	class Identifier
	{
	public:
		Identifier( const std::string &name );
		Identifier( const char *name);
		Identifier( int index );
		Identifier( size_t index );
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
	// ---
	// --- Primarily used for giving the user error messages that include
	// --- file and line number.
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	class DocumentLocation
	{
	public:
		DocumentLocation( Document *doc, unsigned int lineno );

		std::string getPath();
		std::string getDescription();

		void err( std::string msg );
		void warn( std::string msg );

	private:
		friend class Parser;

		Document *doc;
		unsigned int lineno;
	};

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS Node
	// ---
	// --- The heart of the proplib Document Object Model (DOM). The primary
	// --- utilty of this common base class is that it makes parsing
	// --- convenient. Note the use of a "NodeStack" in Parser::parseLine().
	// --- This allows for the parser to create a context to which nodes
	// --- are added, where the nodes might be properties or condtions.
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	class Node
	{
	public:
		enum Type
		{
			CONDITION,
			CLAUSE,
			PROPERTY
		};

		Node( DocumentLocation loc,
			  Type type );
		virtual ~Node();

		Type getNodeType();
		DocumentLocation &getLocation();
		

		bool isProp();
		bool isCond();
		bool isClause();

		class Property *toProp();
		class Condition *toCond();
		class Clause *toClause();

		void err( std::string message );
		void warn( std::string message );

		virtual void add( Node *node ) = 0;

		virtual void dump( std::ostream &out, const char *indent = "" ) = 0;
		virtual void write( std::ostream &out, const char *indent = "" ) = 0;

	protected:
		friend class Parser;
		friend class Schema;

		DocumentLocation loc;

	private:
		Type type;
	};

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS Property
	// ---
	// --- This is the most important class for clients of proplib. It is
	// --- through this class that clients get values from a file. A property
	// --- can be a "SCALAR" (e.g. integer, boolean) or a "CONTAINER"
	// --- (Array or Object). While it doesn't seem very object-oriented to
	// --- have this one class representing both scalars and containers, it
	// --- provides the client with convenient fetching of properties within
	// --- a path, for example:
	// ---
	// ---   int val = doc.get("obj1").get("array1").get("prop1");
	// ---
	// --- Note the cast operators that facilitate statements like the
	// --- example above.
	// ---
	// --- Note that $() expression are stored in this class, but that they
	// --- stored with the '$(' and ')' stripped.
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	typedef std::map<Identifier, class Property *> PropertyMap;
	typedef std::list<class Property *> PropertyList;

	typedef std::list<Condition *> ConditionList;

	class Property : public Node
	{
	public:
		enum Type
		{
			SCALAR,
			CONTAINER
		};

		Property( DocumentLocation loc, Identifier id, bool _isArray = false );
		Property( DocumentLocation loc, Identifier id, const char *val );
		virtual ~Property();

		Property *clone();
		bool equals( Property *other );

		const char *getName();
		std::string getFullName( int minDepth = 0 );

		bool isContainer();
		bool isArray();
		bool isScalar();

		Property &get( Identifier id );
		Property *getp( Identifier id );

		PropertyMap &props();
		PropertyMap &elements(); // just an alias for props() to make code clearer with arrays
		size_t size(); // number of properties/elements
		ConditionList &conds();

		Property &replaceScalar( const std::string &newValue );
		Property &replaceScalar( Property *oldChild, const std::string &newValue );
		void replace( PropertyMap &newprops, bool isArray );

		operator short();
		operator int();
		operator long();
		operator float();
		operator bool();
		operator std::string();

		std::string evalScalar();

		virtual void add( Node *node );

		virtual void dump( std::ostream &out, const char *indent = "" );
		virtual void write( std::ostream &out, const char *indent = "" );
		virtual void writeScalarNames( std::ostream &out, int depth_start = 0 );

	protected:
		void resolve( StringList::iterator curr, StringList::iterator end, PropertyList &result );

	private:
		// We don't allow the copy constructor
		Property( const Property &copy ) : Node(DocumentLocation(NULL,-1), Node::PROPERTY), id("")
		{ throw "Property copy not supported."; }

		std::string getDecoratedScalar();
		Property *findSymbol( Identifier id );
		int getDepth();

		friend class Parser;
		friend class Schema;
		friend class Clause;

		Property *parent;
		Property *symbolSource;
		Type type;
		Identifier id;
		bool _isArray;
		bool isExpr;
		bool isEvaling;

		union
		{
			char *sval;
			struct
			{
				PropertyMap *props;
				ConditionList *conds;
			} oval;
		};
	};

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS Condition
	// ---
	// --- This represents an if/elif/else statement. It can contain one or
	// --- more "clauses". Where the following example code contains 4
	// --- clauses:
	// ---
	// --- if $( foo ) {
	// --- } elif $( bar ) {
	// --- } elif $( baz ) {
	// --- } else {
	// --- }
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	class Condition : public Node
	{
	public:
		Condition( DocumentLocation loc );
		virtual ~Condition();

		Condition *clone();

		Clause *selectClause( Property *symbolSource );

		virtual void add( Node *node );

		virtual void dump( std::ostream &out, const char *indent = "" );
		virtual void write( std::ostream &out, const char *indent = "" );

	private:
		typedef std::list<Clause *> ClauseList;

		ClauseList clauses;
	};

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS Clause
	// ---
	// --- Exists within an if/elif/else "Condition". All clauses have an
	// --- "expression", but an else clause always has the expression "True".
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	class Clause : public Node
	{
	public:
		enum Type
		{
			IF,
			ELIF,
			ELSE
		};

		Clause( DocumentLocation loc, Type type, std::string expr = "True" );
	public:
		virtual ~Clause();

		Clause *clone();

		bool isIf();
		bool isElif();
		bool isElse();

		bool evalExpr( Property *symbolSource );

		virtual void add( Node *node );

		virtual void dump( std::ostream &out, const char *indent = "" );
		virtual void write( std::ostream &out, const char *indent = "" );

	private:
		Clause( DocumentLocation loc );

		friend class Condition;
		friend class Schema;

		Type type;
		Property *exprProp;
		Property *bodyProp;
	};

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS Document
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	class Document : public Property
	{
	public:
		Document( const char *name, const char *path );
		virtual ~Document();

		std::string getPath();

		void set( StringList &propertyPath, const std::string &value );

		virtual void write( std::ostream &out, const char *indent = "" );

	private:
		std::string path;
	};

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS Parser
	// ---
	// --- A stupid line-based parser. One day we'll do better.
	// ---
	// --- If you want to use proplib, you need the parseFile() function.
	// --- The other public functions are for proplib. Not a great API.
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	class Parser
	{
	public:
		static Document *parseFile( const char *path );

		static bool isValidIdentifier( const std::string &text );
		static void scanIdentifiers( const std::string &expr, StringList &ids );

		static StringList parsePropertyPath( const std::string &path );

		static bool parseInt( const std::string &text, int *result = NULL );
		static bool parseFloat( const std::string &text, float *result = NULL );
		static bool parseBool( const std::string &text, bool *result = NULL );

		static bool scalarValuesEqual( const std::string &a, const std::string &b );

	private:
		typedef std::stack<Node *> NodeStack;

		static char *readline( std::istream &in,
							   DocumentLocation &loc,
							   int &inMultiLineComment );
		static char *stripComments( char *line,
									int &inMultiLineComment );
		static void tokenize( DocumentLocation &loc, char *line, CStringList &list );
		static void processLine( Document *doc,
								 DocumentLocation &loc,
								 NodeStack &nodeStack,
								 CStringList &tokens );
	};

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS Schema
	// ---
	// --- This is how you normalize/validate/reduce a values file.
	// ---
	// --- The "normalize" procedure does two important things:
	// ---
	// ---   1) Evaluate/resolve conditions in the schema
	// ---   2) Insert default values into the values file if needed.
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	class Schema
	{
	public:
		static void apply( Document *docSchema, Document *docValues );
		static void reduce( Document *docSchema, Document *docValues, bool apply = true );

	private:
		static void normalize( Property &propSchema, Property &propValues, bool legacyMode );
		static void injectDefaults( Property &propSchema, Property &propValues, bool legacyMode );
		static void validateChildren( Property &propSchema, Property &propValues );
		static void validateProperty(  Property &propSchema, Property &propValues );
		static bool reduceChildren( Property &propSchema, Property &propValues, bool legacyMode );
		static bool reduceProperty(  Property &propSchema, Property &propValues, bool legacyMode );

		static bool isLegacyMode( Document *docValues );
		static Property *getDefaultProperty( Property *propSchema, bool legacyMode );
	};

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS Overlay
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	class Overlay
	{
	public:
		static void overlay( Document *docOverlay, Document *docValues, int index );

	private:
		static Property *findOverlay( Document *docOverlay, const std::string &selector );
		static void validateOverlayDocument( Document *docOverlay );
		static void overlay( Property *propOverlay, Property *propOriginal );
	};

};
