#pragma once

#include <list>
#include <map>
#include <string>

#include "expression.h"

namespace proplib
{
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
		// ----------------------------------------------------------------------
		// ----------------------------------------------------------------------
		// --- CLASS ExpressionEvaluator
		// ----------------------------------------------------------------------
		// ----------------------------------------------------------------------
		class ExpressionEvaluator
		{
		public:
			ExpressionEvaluator( Expression *expr );
			virtual ~ExpressionEvaluator();

			Expression *getExpression();

			std::string evaluate( class Property *prop );

		private:
			Expression *_expr;
			bool _isEvaluating;
		};

		// ----------------------------------------------------------------------
		// ----------------------------------------------------------------------
		// --- API
		// ----------------------------------------------------------------------
		// ----------------------------------------------------------------------
		static void init();
		static void dispose();

	private:
		friend class ExpressionEvaluator;
		static bool eval( const std::string &expr,
						  char *result, size_t result_size );

		static bool alive;
	};
}
