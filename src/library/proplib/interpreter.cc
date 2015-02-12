#include <Python.h>

#include "interpreter.h"

#include <assert.h>
#include <stdio.h>

#include <sstream>

#include "dom.h"
#include "misc.h"
#include "parser.h"

using namespace std;
using namespace proplib;


// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// --- CLASS ExpressionEvaluator
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
Interpreter::ExpressionEvaluator::ExpressionEvaluator( Expression *expr )
: _expr( expr )
, _isEvaluating( false )
{
}

Interpreter::ExpressionEvaluator::~ExpressionEvaluator()
{
}

Expression *Interpreter::ExpressionEvaluator::getExpression()
{
	return _expr;
}

string Interpreter::ExpressionEvaluator::evaluate( Property *prop )
{
	if( _isEvaluating )
		prop->err( "Dependency cycle" );

	_isEvaluating = true;

	// ---
	// --- Generate Python Code
	// ---
	stringstream exprbuf;

	itfor( list<ExpressionElement *>, _expr->elements, it )
	{
		switch( (*it)->type )
		{
		case ExpressionElement::Misc:
			{
				MiscExpressionElement *element = dynamic_cast<MiscExpressionElement *>( *it );

				// Add leading whitespace if not first element.
				if( it != _expr->elements.begin() )
					exprbuf << element->token->getDecorationString();

				// Don't add trailing semicolon
				if( (element != _expr->elements.back()) || (element->token->type != Token::Semicolon) )
					exprbuf << element->token->text;
			}
			break;
		case ExpressionElement::Symbol:
			{
				SymbolExpressionElement *element = dynamic_cast<SymbolExpressionElement *>( *it );
				SymbolPath *symbolPath = element->symbolPath;
				
				// Add leading whitespace if not first element.
				if( it != _expr->elements.begin() )
					exprbuf << symbolPath->head->token->getDecorationString();
				
				Symbol sym;
				if( prop->findSymbol(symbolPath, sym) )
				{
					switch( sym.type )
					{
					case Symbol::EnumValue:
					case Symbol::Class:
						exprbuf << '"' << symbolPath->tail->getText() << '"';
						break;
					case Symbol::Property:
						{
							if( sym.prop->getType() != Node::Scalar )
								prop->err( string("Illegal reference to non-scalar ") + symbolPath->toString() + "." );

							if( sym.prop->getSubtype() == Node::Runtime )
								prop->err( string("Illegal reference to runtime property ") + symbolPath->toString() + ". Only dynamic expresssions may use runtime properties." );

							string value = (string)*sym.prop;
							if( sym.prop->isEnumValue(value) || sym.prop->isString() )
								exprbuf << '"' << value << '"';
							else
								exprbuf << value;
						}
						break;
					default:
						assert( false );
					}
				}
				else
				{
					// Hopefully a Python symbol.
					exprbuf << symbolPath->toString();
				}
			}
			break;
		default:
			assert( false );
			break;
		}
	}

	// ---
	// --- Execute Python Code
	// ---
	char result[1024 * 4];

	//cout << exprbuf.str() << endl;

	bool success = Interpreter::eval( exprbuf.str(), result, sizeof(result) );
	if( !success )
	{
		prop->err( string("[Python] ") + result );
	}

	_isEvaluating = false;

	return result;
}


// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// --- CLASS Interpreter
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------

bool Interpreter::alive = false;

void Interpreter::init()
{
	assert( !alive );
	Py_Initialize();
	alive = true;
}

void Interpreter::dispose()
{
	assert( alive );
	Py_Finalize();
	alive = false;
}

bool Interpreter::eval( const std::string &expr,
						char *result, size_t result_size )
{
	assert( alive );

	char outputPath[128];
	sprintf( outputPath, "/tmp/proplib.%d", getpid() );

	char script[1024 * 4];
	sprintf( script,
			 "import sys\n"
			 "f = open('%s','w')\n"
			 "try:\n"
			 "  expr=\"\"\"\\\n"
			 "%s \"\"\"\n"
			 "  result = str( eval(expr) )\n"
			 "  f.write( '0' )\n"
			 "  f.write( result )\n"
			 "  f.close()\n"
			 "except:\n"
			 "  f.write( '1' )\n"
			 "  f.write( str(sys.exc_info()[1]) )\n"
			 "  f.close()\n",
			 outputPath,
			 expr.c_str() );
	assert( strlen(script) < sizeof(script) );

	int rc = PyRun_SimpleString( script );
	if( rc != 0 )
	{
		fprintf( stderr, "PyRun_SimpleString failed! rc=%d\n", rc );
		exit( 1 );
	}

	FILE *f = fopen( outputPath, "r" );
	char *result_orig = result;

	size_t n;

	char scriptRC;
	n = fread( &scriptRC, 1, 1, f );
	if( n != 1 )
	{
		fprintf( stderr, "Failed reading script output rc! path=%s\n", outputPath );
		exit( 1 );
	}

	while( !feof(f) && (result_size > 0) )
	{
		n = fread(result, 1, result_size, f);
		result[n] = 0;
		result += n;
		result_size -= n;
	}

	if( !feof(f) )
	{
		fprintf( stderr, "Output of python script exceeds result buffer.\n" );
		exit( 1 );
	}

	fclose( f );

	for( char *tail = result - 1; tail >= result_orig; tail-- )
	{
		if( *tail == '\n' )
			*tail = 0;
		else
			break;
	}

	remove( outputPath );

	return scriptRC == '0';
}
