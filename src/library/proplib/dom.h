#pragma once

#include <iostream>
#include <list>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "interpreter.h"
#include "cppprops.h"
#include "expression.h"

namespace proplib
{
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
		bool isIndex() const;

	private:
		std::string _name;
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
		DocumentLocation( class Document *doc,
						  unsigned int lineno = -1,
						  class Token *beginToken = NULL,
						  class Token *endToken = NULL );

		class Document *getDocument();
		std::string getPath() const;
		std::string getDescription();

		class Token *getBeginToken();
		class Token *getEndToken();

		void err( std::string msg );
		void warn( std::string msg );

	private:
		friend bool operator<( const DocumentLocation &a, const DocumentLocation &b );

		class Document *_doc;
		unsigned int _lineno;
		class Token *_beginToken;
		class Token *_endToken;
	};

	bool operator<( const DocumentLocation &a, const DocumentLocation &b );


	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS Symbol
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	class Symbol
	{
	public:
		enum Type
		{
			Class,
			Property,
			EnumValue
		};

		Type type;
		union
		{
			class Property *prop;
			class Class *klass;
		};
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
			Enum,
			Class,
			Scalar,
			Object,
			Array,
			Attr,
			Meta
		};

		enum Subtype
		{
			None,
			Const,
			Dynamic,
			Runtime,
			Document
		};

	protected:
		Node( const Node &other );
		Node( Type type, Subtype subtype, DocumentLocation loc );

	public:
		virtual ~Node();

		Type getType();
		Subtype getSubtype();

		DocumentLocation getLocation();

		void err( std::string msg );
		void warn( std::string msg );
		virtual void dump( std::ostream &out, std::string = "" );

		bool findSymbol( SymbolPath *name, Symbol &sym );
		virtual bool __findLocalSymbol( SymbolPath::Element *name, Symbol &sym );

	protected:
		void add( Node *node );

		Node *_parent;
		Node *_symbolSource;

	private:
		bool findSymbol( SymbolPath::Element *name, Symbol &sym );

		Type _type;
		Subtype _subtype;
		DocumentLocation _loc;
	};

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS Enum
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	typedef std::map<Identifier, class Enum *> EnumMap;

	class Enum : public Node
	{
	public:
		Enum( DocumentLocation loc, Identifier id );
		virtual ~Enum();

		const Identifier &getId();
		void addValue( const std::string &name );
		bool contains( const std::string &name );

		Enum *clone();
		virtual bool __findLocalSymbol( SymbolPath::Element *name, Symbol &sym );

	private:
		Identifier _id;
		std::set<std::string> _values;
	};

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS Class
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	typedef std::map<Identifier, class Class *> ClassMap;

	class Class : public Node
	{
	public:
		Class( DocumentLocation loc,
			   Identifier id,
			   class ObjectProperty *definition );
		virtual ~Class();

		const Identifier &getId();
		class ObjectProperty *getDefinition();

		Class *clone();

	protected:
		friend class Property;

	private:
		Identifier _id;
		class ObjectProperty *_definition;
	};

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS Property
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	typedef std::map<Identifier, class Property *> PropertyMap;
	typedef std::list<class Property *> PropertyList;

	class Property : public Node
	{
	protected:
		Property( const Property &other );

	protected:
		Property( Node::Type type, Node::Subtype subtype, DocumentLocation loc, Identifier id );

	public:
		virtual ~Property();

		const Identifier &getId();
		const char *getName();
		std::string getFullName( int minDepth = 0, const char *delimiter = NULL );

		Property *getParent();
		bool hasProperty( Identifier id );
		virtual Property &get( Identifier id ) = 0;
		virtual Property *getp( Identifier id ) = 0;

		virtual PropertyMap &props() = 0;
		PropertyMap &elements(); // just an alias for props() to make code clearer with arrays
		size_t size(); // number of properties/elements

		virtual operator short() = 0;
		virtual operator int() = 0;
		virtual operator long() = 0;
		virtual operator float() = 0;
		virtual operator bool() = 0;
		virtual operator std::string() = 0;

		virtual Property *clone( Identifier cloneId ) = 0;

		virtual void addEnum( class Enum *enum_ );
		class Enum *getEnum( const std::string &name );
		bool isEnumValue( const std::string &value );

		bool isString();

		void setSchema( class ObjectProperty *schema );
		class ObjectProperty *getSchema();

		virtual bool hasBinding();

		virtual bool __findLocalSymbol( SymbolPath::Element *name, Symbol &sym );

	protected:
		int getDepth();
		Property *baseClone( Property *clone );

		EnumMap _enums;

	private:
		friend class DocumentEditor;

		Identifier _id;
		class ObjectProperty *_schema;
	};

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS __ScalarProperty
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	class __ScalarProperty : public Property
	{
	protected:
		__ScalarProperty( const __ScalarProperty &other );
		__ScalarProperty( Node::Subtype subtype, DocumentLocation loc, Identifier id );

	public:
		virtual ~__ScalarProperty();

		virtual Property &get( Identifier id );
		virtual Property *getp( Identifier id );

		virtual PropertyMap &props();
		
		virtual operator short();
		virtual operator int();
		virtual operator long();
		virtual operator float();
		virtual operator bool();
		virtual operator std::string();

		virtual void dump( std::ostream &out, std::string indent = "" );

	protected:
		virtual std::string getEvaledString() = 0;

	private:
		int toInt();
		float toFloat();
		bool toBool();
	};

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS ConstScalarProperty
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	class ConstScalarProperty : public __ScalarProperty
	{
	private:
		ConstScalarProperty( const ConstScalarProperty &other );

	public:
		ConstScalarProperty( DocumentLocation loc, Identifier id, Expression *expr );
		virtual ~ConstScalarProperty();

		virtual Property *clone( Identifier cloneId );

		Expression *getExpression();

	protected:
		virtual std::string getEvaledString();

	private:
		Interpreter::ExpressionEvaluator *_expr;
	};


	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS RuntimeScalarProperty
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	class RuntimeScalarProperty : public __ScalarProperty
	{
	public:
		RuntimeScalarProperty( DocumentLocation loc, Identifier id );
		virtual ~RuntimeScalarProperty();

		virtual Property *clone( Identifier cloneId );

	protected:
		virtual std::string getEvaledString();
	};


	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS DynamicScalarAttribute
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	class DynamicScalarAttribute : public Node
	{
	public:
		DynamicScalarAttribute( DocumentLocation loc,
								Identifier id,
								Expression *expr );
		virtual ~DynamicScalarAttribute();

		const char *getName();

		DynamicScalarAttribute *clone();

		Expression *getExpression();

	private:
		Identifier _id;
		Expression *_expr;
	};

	typedef std::map<Identifier, DynamicScalarAttribute *> DynamicScalarAttributeMap;

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS DynamicScalarProperty
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	class DynamicScalarProperty : public __ScalarProperty
	{
	public:
		DynamicScalarProperty( DocumentLocation loc,
							   Identifier id,
							   Expression *initExpr );
		virtual ~DynamicScalarProperty();

		virtual Property *clone( Identifier cloneId );

		Expression *getInitExpression();

		void add( DynamicScalarAttribute *attr );
		DynamicScalarAttributeMap &attrs();
		DynamicScalarAttribute *getAttr( std::string name );

	protected:
		virtual std::string getEvaledString();

	private:
		Interpreter::ExpressionEvaluator *_initExpr;
		DynamicScalarAttributeMap _attrs;
	};


	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS __ContainerProperty
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	class __ContainerProperty : public Property
	{
	protected:
		__ContainerProperty( Node::Type type, Node::Subtype subtype, DocumentLocation loc, Identifier id );
		virtual ~__ContainerProperty();

	public:
		virtual void add( Property *prop );
		virtual void replace( Property *propNew );

		virtual Property &get( Identifier id );
		virtual Property *getp( Identifier id );

		virtual PropertyMap &props();
		
		virtual operator short();
		virtual operator int();
		virtual operator long();
		virtual operator float();
		virtual operator bool();
		virtual operator std::string();

		virtual void dump( std::ostream &out, std::string indent = "" );

		virtual bool __findLocalSymbol( SymbolPath::Element *name, Symbol &sym );

	private:
		PropertyMap _props;
	};

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS ObjectProperty
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	class ObjectProperty : public __ContainerProperty
	{
	protected:
		ObjectProperty( Node::Subtype subtype, DocumentLocation loc, Identifier id );

	public:
		ObjectProperty( DocumentLocation loc, Identifier id );
		virtual ~ObjectProperty();

		virtual void add( Property *prop );

		virtual Property *clone( Identifier cloneId );
		virtual bool __findLocalSymbol( SymbolPath::Element *name, Symbol &sym );

		void addClass( class Class *class_ );
		class Class *getClass( const std::string &name );

	private:
		ClassMap _classes;
	};

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS ArrayProperty
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	class ArrayProperty : public __ContainerProperty
	{
	public:
		ArrayProperty( DocumentLocation loc, Identifier id );
		virtual ~ArrayProperty();

		virtual void add( Property *prop );

		virtual Property *clone( Identifier cloneId );
	};

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS MetaProperty
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	class MetaProperty : public Node
	{
	public:
		MetaProperty( DocumentLocation loc,
					  Identifier id,
					  std::string value );
		virtual ~MetaProperty();

		Identifier getId();
		std::string getValue();

	private:
		Identifier _id;
		std::string _value;
	};

	typedef std::map<Identifier, MetaProperty *> MetaPropertyMap;

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS Document
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	class Document : public ObjectProperty
	{
	public:
		Document( std::string name, std::string path );
		virtual ~Document();

		std::string getPath();

		bool hasMeta( std::string name );
		MetaProperty *getMeta( std::string name );
		void addMeta( MetaProperty *prop );
		MetaPropertyMap &metaprops();

	private:
		std::string _path;
		MetaPropertyMap _metaprops;
	};
};
