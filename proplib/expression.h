#pragma once

#include <iostream>
#include <list>
#include <map>
#include <string>

namespace proplib
{
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS SymbolPath
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	class SymbolPath
	{
	public:
		class Element
		{
		public:
			std::string getText();

			class Token *token;
			Element *next;
			Element *prev;
		};

		SymbolPath();
		virtual ~SymbolPath();

		void add( class Token *token );
		std::string toString( bool leadingDecoration = false );

		Element *head;
		Element *tail;
	};


	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS ExpressionElement
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	class ExpressionElement
	{
	public:
		enum Type
		{
			Symbol,
			Misc
		};

		ExpressionElement( Type type );
		virtual ~ExpressionElement();

		Type type;
	};

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS SymbolExpressionElement
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	class SymbolExpressionElement : public ExpressionElement
	{
	public:
		SymbolExpressionElement( SymbolPath *name );
		virtual ~SymbolExpressionElement();

		SymbolPath *symbolPath;
	};

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS MiscExpressionElement
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	class MiscExpressionElement : public ExpressionElement
	{
	public:
		MiscExpressionElement( class Token *tok );
		virtual ~MiscExpressionElement();

		class Token *token;
	};

	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	// --- CLASS Expression
	// ----------------------------------------------------------------------
	// ----------------------------------------------------------------------
	class Expression
	{
	public:
		Expression();
		virtual ~Expression();

		Expression *clone();

		void write( std::ostream &out, bool leadingDecoration = true );
		std::string toString( bool leadingDecoration = true );

		bool isCppClause();

		std::list<ExpressionElement *> elements;
	};
}
