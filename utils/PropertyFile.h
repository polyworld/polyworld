#pragma once

#include <istream>
#include <list>
#include <map>
#include <stack>
#include <string>
#include <vector>

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

		std::string getPath();
		std::string getDescription();

		void err( std::string msg );

	private:
		friend class Parser;

		Document *doc;
		unsigned int lineno;
	};

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS Node
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

		bool isProp();
		bool isCond();
		bool isClause();

		class Property *toProp();
		class Condition *toCond();
		class Clause *toClause();

		void err( std::string message );

		virtual void add( Node *node ) = 0;
		virtual void dump( std::ostream &out, const char *indent = "" ) = 0;

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
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	typedef std::map<Identifier, class Property *> PropertyMap;
	typedef std::list<Condition *> ConditionList;

	class Property : Node
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

		bool isObj();
		bool isArray();
		bool isScalar();

		Property &get( Identifier id );
		Property *getp( Identifier id );

		PropertyMap &props();
		ConditionList &conds();

		operator int();
		operator float();
		operator bool();
		operator std::string();

		virtual void add( Node *node );

		Property *clone();
		virtual void dump( std::ostream &out, const char *indent = "" );

	private:
		// We don't allow the copy constructor
		Property( const Property &copy ) : Node(DocumentLocation(NULL,-1), Node::PROPERTY), id("")
		{ throw "Property copy not supported."; }

		std::string evalScalar();
		Property *findSymbol( Identifier id );

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
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	class Condition : public Node
	{
	public:
		Condition( DocumentLocation loc );
		virtual ~Condition();

		Clause *selectClause( Property *symbolSource );

		virtual void add( Node *node );

		virtual void dump( std::ostream &out, const char *indent = "" );
		

	private:
		typedef std::list<Clause *> ClauseList;

		ClauseList clauses;
	};

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS Clause
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
		virtual ~Clause();

		bool isIf();
		bool isElif();
		bool isElse();

		bool evalExpr( Property *symbolSource );

		virtual void add( Node *node );

		virtual void dump( std::ostream &out, const char *indent = "" );

	private:
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
		typedef std::vector<char *> CStringList;
		typedef std::list<std::string> StringList;

		static Document *parse( const char *path );

		static bool isValidIdentifier( const std::string &text );
		static void scanIdentifiers( const std::string &expr, StringList &ids );

		static bool parseInt( const std::string &text, int *result );
		static bool parseFloat( const std::string &text, float *result );
		static bool parseBool( const std::string &text, bool *result );

	private:
		typedef std::stack<Node *> NodeStack;

		static char *readline( std::istream &in,
							   DocumentLocation &loc,
							   bool &inMultiLineComment );
		static char *stripComments( char *line,
									bool &inMultiLineComment );
		static void tokenize( DocumentLocation &loc, char *line, CStringList &list );
		static void processLine( Document *doc,
								 DocumentLocation &loc,
								 NodeStack &nodeStack,
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
		static void normalize( Property &propSchema,
							   Property &propValues,
							   Property *conditionSymbolSource = NULL );
		static void injectDefaults( Property &propSchema, Property &propValues );
		static void validateChildren( Property &propSchema, Property &propValues );
		static void validateProperty(  Property &propSchema, Property &propValues );
	};
};
