#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

namespace proplib
{
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS Token
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	class Token
	{
	public:
		enum Type
		{
			Comment,
			Whitespace,
			Enum,
			Class,
			Dyn,
			Attrs,
			MetaId,
			Id,
			String,
			Number,
			LeftCurly,
			RightCurly,
			LeftSquare,
			RightSquare,
			LeftParen,
			RightParen,
			Comma,
			Semicolon,
			Dot,
			Bof,
			Eof,
			Misc
		};

		Token( Type type, const char *typeName, const std::string &text );

		bool isDecoration();
		std::string getDecorationString();

		bool hasNewline();

		static std::string toString( Token *start, Token *end, bool leadingDecoration = false );

		Type type;
		const char *typeName;
		std::string text;

		Token *next;
		Token *prev;
		Token *decoration;
		int lineno;
		int number;
	};

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS Tokenizer
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	class Tokenizer
	{
	public:
		Tokenizer( std::string sourceName, std::istream *in );
		virtual ~Tokenizer();

		Token *next();

	private:
		char get();
		Token *__next();
		Token *parseComment();
		Token *parseWhitespace();
		Token *parseMetaId();
		Token *parseWord();
		Token *parseString();
		Token *parseEscape();
		Token *parseNumber();

		std::string _sourceName;
		std::istream *_in;
		int _lineno;
		int _number;

		Token *_prev;
	};

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS SyntaxNode
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	class SyntaxNode
	{
	public:
		enum Type
		{
			Document,
			MetaProperty,
			MetaPropertyValue,
			Object,
			Property,
			PropertyValue,
			Id,
			Array,
			Enum,
			EnumValues,
			Class,
			Dyn,
			DynAttr,
			CppClause,
			Expression,
			SymbolPath,
			SymbolPathElement,
			Misc
		};

		void dump( std::ostream &out, std::string indent = "" );

		Type type;
		const char *typeName;
		Token *beginToken;
		Token *endToken;
		SyntaxNode *parent;
		std::vector<SyntaxNode *> children;
	};

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS Parser
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	class Parser
	{
	public:
		SyntaxNode *parseDocument( std::string path, std::istream *in );
		SyntaxNode *parseSymbolPath( std::string text );
		SyntaxNode *parsePropertyValue( std::string text );
		SyntaxNode *parseMetaPropertyValue( std::string text );

	private:
		void init( std::string path, std::istream *in );
		void err( Token *tok, std::string message );
		Token *peek();
		Token *next( Token::Type required );
		Token *next();
		void pushNode( SyntaxNode::Type type, const char *typeName, Token *begin );
		SyntaxNode *popNode( SyntaxNode::Type type, const char *typeName, Token *end );

		void parseMetaProperties();
		SyntaxNode *parseMetaPropertyValue();
		void parseObject();
		void parseProperty();
		SyntaxNode *parsePropertyValue();
		void parseArray();
		void parseDyn();
		void parseDynAttrs();
		void parseDynAttr();
		void parseCppClause();
		SyntaxNode *parseExpression( bool parenDelimited = false);
		SyntaxNode *parseSymbolPath();
		void parseEnum();
		void parseClass();

		std::string _path;
		Tokenizer *_tokenizer;
		Token *_currTok;
		SyntaxNode *_syntaxNode;
	};
}
