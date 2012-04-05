#include "dynamic.h"

#include <assert.h>
#include <dlfcn.h>
#include <stdio.h>
#include <string.h>

#include <fstream>
#include <sstream>

#include "dom.h"
#include "expression.h"
#include "interpreter.h"
#include "misc.h"
#include "parser.h"

using namespace proplib;
using namespace std;

#define SrcDynProp ".bld/dynprop/src/generated.cpp"
#if __linux__
#define LibDynProp ".bld/dynprop/bin/libdynprop.so"
#else
#define LibDynProp ".bld/dynprop/bin/libdynprop.dylib"
#endif

#define l(content) out << content << endl

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
// --- CLASS DynamicProperties
// ----------------------------------------------------------------------
// ----------------------------------------------------------------------

DynamicProperties::LibraryUpdate DynamicProperties::_update = NULL;
DynamicProperties::LibraryGetMetadata DynamicProperties::_getMetadata = NULL;
Document *DynamicProperties::_doc = NULL;
DynamicProperties::UpdateContext *DynamicProperties::_context = NULL;

void DynamicProperties::init( Document *doc, UpdateContext *context )
{
	_doc = doc;
	_context = context;

	generateLibrarySource();

	SYSTEM( "scons -f scripts/build/SConstruct " LibDynProp " 1>/dev/null" );

	void *libHandle = dlopen( LibDynProp, RTLD_LAZY );
	errif( !libHandle, "Failed opening " LibDynProp );

	typedef void (*LibraryInit)( UpdateContext *context );
	LibraryInit init = (LibraryInit)dlsym( libHandle, "__clink__DynamicProperties_Init" );
	errif( dlerror() != NULL, dlerror() );

	_update = (LibraryUpdate)dlsym( libHandle, "__clink__DynamicProperties_Update" );
	errif( dlerror() != NULL, dlerror() );

	_getMetadata = (LibraryGetMetadata)dlsym( libHandle, "__clink__DynamicProperties_GetMetadata" );
	errif( dlerror() != NULL, dlerror() );

	init( context );
}

void DynamicProperties::update()
{
	_update( _context );
}

void DynamicProperties::getMetadata( PropertyMetadata **metadata, int *count )
{
	_getMetadata( metadata, count );
}

void DynamicProperties::generateLibrarySource()
{
	DynamicPropertyList dynamicProperties;
	getDynamicProperties( _doc, dynamicProperties );

	itfor( DynamicPropertyList, dynamicProperties, it )
	{
		itfor( DynamicScalarAttributeMap, (*it)->attrs(), it_attr )
		{
			string name = it_attr->second->getName();
			if( (name != "state")
				&& (name != "init")
				&& (name != "update") )
			{
				it_attr->second->err( "Invalid attribute name." );
			}
		}
	}

	SYSTEM( "mkdir -p $(dirname " SrcDynProp ")" );
	ofstream out( SrcDynProp );

	l( "// This file is machine-generated. See " << __FILE__ );
	l( "" );
	l( "#include <assert.h>" );
	l( "#include <iostream>" );
	l( "#include <sstream>" );
	l( "#include \"dynamic.h\"" );
	l( "#include \"barrier.h\"" );
	l( "#include \"GenomeUtil.h\"" );
	l( "#include \"globals.h\"" );
	l( "#include \"Metabolism.h\"" );
	l( "#include \"Simulation.h\"" );
	l( "#include \"state.h\"" );
	l( "using namespace std;" );
	l( "using namespace proplib;" );
	l( "" );
	l( "static bool inited = false;" );

	l( "");
	l( "namespace proplib {" );
	l( "");

	// ---
	// --- Generate State Datastructures
	// ---
	generateStateStructs( out, dynamicProperties );

	// ---
	// --- Generate Metadata
	// ---
	DynamicPropertyIndexMap indexMap;
	generateMetadata( out, dynamicProperties, indexMap );

	// ---
	// --- Generate Init Function
	// ---
	generateInitSource( out, dynamicProperties, indexMap );

	// ---
	// --- Generate Update Function
	// ---
	generateUpdateSource( out, dynamicProperties, indexMap );

	// ---
	// --- Generate C Linkage Entry Points
	// ---
	l( "" );
	l( "// These provide public symbols we can access via dlsym()" );
	l(	"extern \"C\"" );
	l( "{" );
	l( "  void __clink__DynamicProperties_Init( proplib::DynamicProperties::UpdateContext *context )" );
	l( "  {" );
	l( "    DynamicProperties_Init( context );" );
	l( "  }" );
	l( "  void __clink__DynamicProperties_Update( proplib::DynamicProperties::UpdateContext *context )" );
	l( "  {" );
	l( "    DynamicProperties_Update( context );" );
	l( "  }" );
	l( "  void __clink__DynamicProperties_GetMetadata( proplib::DynamicProperties::PropertyMetadata **result_metadata, int *result_count )" );
	l( "  {" );
	l( "    assert( inited );" );
	l( "    *result_metadata = metadata;" );
	l( "    *result_count = " << dynamicProperties.size() << ";" );
	l( "  }" );
	l( "}" );

	l( "" );
	l( "}" );
}

void DynamicProperties::generateStateStructs( ofstream &out, DynamicPropertyList &dynamicProperties )
{
	itfor( DynamicPropertyList, dynamicProperties, it )
	{
		DynamicScalarProperty *prop = *it;
		DynamicScalarAttribute *stateAttr = prop->getAttr( "state" );
		if( stateAttr )
		{
			if( !stateAttr->getExpression()->isCppClause() )
				stateAttr->err( "'state' Should be enclosed in {}" );

			l( "struct " << getStateStructName(prop) );
			stateAttr->getExpression()->write( out );
			l( ";" << endl );
		}
	}	
}

void DynamicProperties::generateMetadata( ofstream &out,
										  DynamicPropertyList &dynamicProperties, 
										  DynamicPropertyIndexMap &indexMap )
{
	l( "static proplib::DynamicProperties::PropertyMetadata metadata[] =" );
	l( "{" );

	itfor( DynamicPropertyList, dynamicProperties, it )
	{
		DynamicScalarProperty *prop = *it;

		int index = (int)indexMap.size();
		indexMap[prop] = index;

		l( "  { " );

		// name
		l( "    " << '"' << prop->getFullName(1) << '"' << "," );

		// type
		l( "    " << getDataLibType(prop) << "," );

		// value
		l( "    NULL," );

		// valueChanged
		l( "    false," );

		// state
		if( prop->getAttr("state") )
			l( "    new " << getStateStructName(prop) );
		else
			l( "    NULL" );

		if( prop != dynamicProperties.back() )
			l( "  }," );
		else
			l( "  }" );
	}

	l( "};" );
	l( "" );
}

void DynamicProperties::generateInitSource( ofstream &out,
											DynamicPropertyList &dynamicProperties,
											DynamicPropertyIndexMap &indexMap )
{
	map<DynamicScalarProperty *, string> prop2initBody;
	map<DynamicScalarProperty *, DynamicPropertyList> prop2antecedents;
	itfor( DynamicPropertyList, dynamicProperties, it )
	{
		DynamicScalarProperty *prop = *it;
		prop2initBody[prop] = generateInitFunctionBody( prop, prop2antecedents[prop], indexMap );
	}	

	sortDynamicProperties( dynamicProperties, prop2antecedents );
	
	// ---
	// --- Write Init Source
	// ---

	l( "// ------------------------------------------------------------" );
	l( "// --- DynamicProperties_Init()" );
	l( "// ---" );
	l( "// --- Invoked at init." );
	l( "// ------------------------------------------------------------" );
	l( "void DynamicProperties_Init( proplib::DynamicProperties::UpdateContext *context )" );
	l( "{" );
	l( "  assert( !inited );" );

	itfor( DynamicPropertyList, dynamicProperties, it )
	{
		DynamicScalarProperty *prop = *it;
		int index = indexMap[prop];

		l( "  // " << prop->getFullName(1) );
		l( "  {" );
		l( "    metadata[" << index << "].value = &(" << getCppSymbol(prop) << ");" );
		if( prop->getAttr("state") )
		{
			l( "    " << getStateStructName(prop) << " *state = (" << getStateStructName(prop) << "*) metadata[" << index << "].state;" );
		}
		if( !prop2initBody[prop].empty() )
		{
			l( "    // START EXPRESSION" );
			l( prop2initBody[prop] );
			l( "    // END EXPRESSION" );
		}
		l( "  }" );
	}

	l( "  inited = true;" );
	l( "}" );
	l( "" );
}

string DynamicProperties::generateInitFunctionBody( DynamicScalarProperty *prop,
													DynamicPropertyList &antecedents,
													DynamicPropertyIndexMap &indexMap )
{
	if( prop->getAttr("init") == NULL )
	{
		return "";
	}
	Expression *expr = prop->getAttr( "init" )->getExpression();
	stringstream out;


	itfor( list<ExpressionElement *>, expr->elements, it )
	{
		// Skip outer '{' '}'
		if( (*it == expr->elements.front()) || (*it == expr->elements.back()) )
			continue;

		switch( (*it)->type )
		{
		case ExpressionElement::Misc:
			{
				MiscExpressionElement *element = dynamic_cast<MiscExpressionElement *>( *it );

				out << element->token->getDecorationString();
				out << element->token->text;
			}
			break;
		case ExpressionElement::Symbol:
			{
				SymbolExpressionElement *element = dynamic_cast<SymbolExpressionElement *>( *it );
				SymbolPath *symbolPath = element->symbolPath;

				string text;
				bool resolved = false;

				Symbol sym;
				if( prop->findSymbol(symbolPath, sym) )
				{
					switch( sym.type )
					{
					case Symbol::Property:
						if( sym.prop->getType() == Node::Scalar )
						{
							resolved = true;
							switch( sym.prop->getSubtype() )
							{
							case Node::Const:
								text = (string)*sym.prop;
								break;
							case Node::Dynamic:
								text = getMetadataLValue( sym.prop, indexMap );
								antecedents.push_back( dynamic_cast<DynamicScalarProperty *>(sym.prop) );
								break;
							case Node::Runtime:
								text = getCppSymbol( sym.prop );
								break;
							default:
								assert( false );
							}
						}
						else
						{
							if( sym.prop->getSchema()->getp("cppsym") )
							{
								resolved = true;
								text = getCppSymbol( sym.prop );
							}
							break;
						}
						break;
					case Symbol::EnumValue:
						text = string("\"") + symbolPath->tail->getText() + "\"";
						break;
					default:
						// no-op
						break;
					}
				}

				// Must be a C++ symbol.
				if( !resolved )
					text = symbolPath->toString();

				out << symbolPath->head->token->getDecorationString();
				out << text;
			}
			break;
		default:
			assert( false );
		}
	}

	return out.str();
}

void DynamicProperties::generateUpdateSource( ofstream &out,
											  DynamicPropertyList &dynamicProperties,
											  DynamicPropertyIndexMap &indexMap )
{
	map<DynamicScalarProperty *, string> prop2updateBody;
	map<DynamicScalarProperty *, DynamicPropertyList> prop2antecedents;
	itfor( DynamicPropertyList, dynamicProperties, it )
	{
		DynamicScalarProperty *prop = *it;

		prop2updateBody[prop] = generateUpdateFunctionBody( prop, prop2antecedents[prop], indexMap );
	}	

	sortDynamicProperties( dynamicProperties, prop2antecedents );
	
	// ---
	// --- Write Update Source
	// ---

	l( "// ------------------------------------------------------------" );
	l( "// --- DynamicProperties_Update()" );
	l( "// ---" );
	l( "// --- Invoked once per step. Updates all dynamic properties." );
	l( "// ------------------------------------------------------------" );
	l( "void DynamicProperties_Update( proplib::DynamicProperties::UpdateContext *context )" );
	l( "{" );
	l( "  assert( inited );" );

	itfor( DynamicPropertyList, dynamicProperties, it )
	{
		DynamicScalarProperty *prop = *it;
		int index = indexMap[prop];

		l( "  // " << prop->getFullName(1) );
		l( "  {" );
		l( "    struct local" );
		l( "    {" );
		l( "      static inline " << getCppType(prop) << " update( proplib::DynamicProperties::UpdateContext *context ) " );
		l( "      {" );
		if( prop->getAttr("state") )
		{
			l( "        " << getStateStructName(prop) << " *state = (" << getStateStructName(prop) << "*) metadata[" << index << "].state;" );
		}
		l( "        // START EXPRESSION" );
		l( prop2updateBody[prop] );
		l( "        // END EXPRESSION" );
		l( "      }" );
		l( "    };" );
		l( "    " << getCppType(prop) << " newval = local::update( context );" );
		//l( "    cout << \"" << prop->getFullName(1) << "= \" << newval << endl;" ); 
		l( "    if( newval == " << getMetadataLValue(prop, indexMap) << ")" );
		l( "      metadata[" << index << "].valueChanged = false;" );
		l( "    else" );
		l( "    {" );
		l( "      metadata[" << index << "].valueChanged = true;" );
		itfor( PropertyMap, prop->getSchema()->props(), it_attr )
		{
			string attrName = it_attr->second->getName();

			if( attrName == "min" )
				l( "      assert( newval >= " << (string)*it_attr->second << " );" );
			else if( attrName == "exmin" )
				l( "      assert( newval > " << (string)*it_attr->second << " );" );
			else if( attrName == "max" )
				l( "      assert( newval <= " << (string)*it_attr->second << " );" );
			else if( attrName == "exmax" )
				l( "      assert( newval < " << (string)*it_attr->second << " );" );
		}
			
		l( "      *((" << getCppType(prop) << " *)metadata[" << index << "].value) = newval;" );
		l( "    }" );
		l( "  }" );
	}

	l( "}" );
	l( "" );
}

string DynamicProperties::generateUpdateFunctionBody( DynamicScalarProperty *prop,
													  DynamicPropertyList &antecedents,
													  DynamicPropertyIndexMap &indexMap )
{
	Expression *expr;
	bool skipBraces;
	if( prop->getAttr("update") != NULL )
	{
		skipBraces = true;
		expr = prop->getAttr( "update" )->getExpression();
	}
	else
	{
		skipBraces = false;
		expr = prop->getInitExpression();
	}

	bool hasReturn = false;
	stringstream out;

	itfor( list<ExpressionElement *>, expr->elements, it )
	{
		// Skip outer '{' '}'
		if( skipBraces && ( (*it == expr->elements.front()) || (*it == expr->elements.back()) ) )
			continue;

		switch( (*it)->type )
		{
		case ExpressionElement::Misc:
			{
				MiscExpressionElement *element = dynamic_cast<MiscExpressionElement *>( *it );

				out << element->token->getDecorationString();
				out << element->token->text;
			}
			break;
		case ExpressionElement::Symbol:
			{
				SymbolExpressionElement *element = dynamic_cast<SymbolExpressionElement *>( *it );
				SymbolPath *symbolPath = element->symbolPath;
				bool usesValueKeyword = false;

				if( symbolPath->head->getText() == "value" )
				{
					usesValueKeyword = true;
					symbolPath->head->token->text = prop->getName();
				}
				else if( symbolPath->head->getText() == "return" )
				{
					hasReturn = true;
				}

				string text;
				bool resolved = false;

				Symbol sym;
				if( prop->findSymbol(symbolPath, sym) )
				{
					switch( sym.type )
					{
					case Symbol::Property:
						if( sym.prop->getType() == Node::Scalar )
						{
							resolved = true;
							switch( sym.prop->getSubtype() )
							{
							case Node::Const:
								text = (string)*sym.prop;
								break;
							case Node::Dynamic:
								text = getMetadataLValue( sym.prop, indexMap );
								antecedents.push_back( dynamic_cast<DynamicScalarProperty *>(sym.prop) );
								break;
							case Node::Runtime:
								text = getCppSymbol( sym.prop );
								break;
							default:
								assert( false );
							}
						}
						else
						{
							if( sym.prop->getSchema()->getp("cppsym") )
							{
								resolved = true;
								text = getCppSymbol( sym.prop );
							}
							break;
						}
						break;
					case Symbol::EnumValue:
						text = string("\"") + symbolPath->tail->getText() + "\"";
						break;
					default:
						// no-op
						break;
					}
				}

				// Must be a C++ symbol.
				if( !resolved )
					text = symbolPath->toString();

				// Restore original state.
				if( usesValueKeyword )
					symbolPath->head->token->text = "value";

				out << symbolPath->head->token->getDecorationString();
				out << text;
			}
			break;
		default:
			assert( false );
		}
	}

	if( !hasReturn )
		return string("return ") + out.str() + ";";
	else
		return out.str();
}

string DynamicProperties::getStateStructName( DynamicScalarProperty *prop )
{
	return string("State___") + prop->getFullName( 1, "__" );
}

string DynamicProperties::getDataLibType( DynamicScalarProperty *prop )
{
	string dtype = "";

	string type = prop->getSchema()->get( "type" );
	if( type == "Float" )
		dtype = "FLOAT";
	if( type == "Int" )
		dtype = "INT";
	if( type == "String" )
		dtype = "STRING";
	if( type == "Bool" )
		dtype = "BOOL";

	if( dtype.empty() )
		prop->err( "No appropriate datalib type for dynamic property." );

	return string("datalib::") + dtype;
}

string DynamicProperties::getCppType( Property *prop )
{
	Property *propType = prop->getSchema()->getp( "cpptype" );
	if( propType )
		return (string)*propType;
	else
	{
		string type = prop->getSchema()->get( "type" );
		if( type == "Float" )
			return "float";
		if( type == "Int" )
			return "int";
		if( type == "String" )
			return "string";
		if( type == "Bool" )
			return "bool";

		prop->getSchema()->err( "Cannot determine implicit cpptype" );
		return "";
	}
}

string DynamicProperties::getMetadataLValue( Property *prop_, DynamicPropertyIndexMap &indexMap )
{
	DynamicScalarProperty *prop = dynamic_cast<DynamicScalarProperty *>( prop_ );

	stringstream buf;
	buf << "*((" << getCppType(prop) << "*)metadata[/*" << prop->getFullName(1) << "*/ " << indexMap[prop] << "].value)";

	return buf.str();
}

static bool findSymMacro( Property &prop,
						  string cppsym,
						  int *result_start,
						  int *result_len,
						  vector<string> *result_args )
{
	result_args->clear();

	size_t start = cppsym.find( "$[" );
	if( start != string::npos )
	{
		size_t end = cppsym.find( "]", start );
		if( end == string::npos )
			prop.err( "Missing ']'" );

		*result_start = (int)start;
		*result_len = (int)(end - start + 1);

		size_t pos = start + 2;
		while( pos < end )
		{
			size_t tokStart = cppsym.find_first_not_of( " \t\n", pos );
			if( (cppsym[tokStart] == ',') || (cppsym[tokStart] == ']') )
			{
				pos = tokStart + 1;
				result_args->push_back( "" );
			}
			else
			{
				size_t tokEnd = cppsym.find_first_of( " \t\n,]", tokStart );
				result_args->push_back( cppsym.substr(tokStart, tokEnd - tokStart) );
				pos = cppsym.find_first_of( ",]", tokEnd ) + 1;
			}
		}

		return true;
	}

	return false;
}

string DynamicProperties::getCppSymbol( Property *prop )
{
	Property &propSym = prop->getSchema()->get( "cppsym" );
	string sym = propSym;

	while( true )
	{
		vector<string> macro_args;
		int macro_start;
		int macro_len;

		if( !findSymMacro(propSym, sym, &macro_start, &macro_len, &macro_args) )
			break;

		if( macro_args[0] == "sim" )
		{
			if( macro_args.size() != 1 )
				propSym.err( "$[sim] takes no args." );

			sym.replace( macro_start, macro_len, "context->sim" );
		}
		else if( macro_args[0] == "index" )
		{
			if( macro_args.size() != 1 )
				propSym.err( "$[index] takes no args." );
			if( prop->getParent()->getType() != Node::Array )
				propSym.err( "$[index] only valid for element of array." );

			sym.replace( macro_start, macro_len, prop->getName() );
		}
		else if( macro_args[0] == "ancestor" )
		{
			if( macro_args.size() != 1 )
				propSym.err( "$[ancestor] takes no args." );

			bool resolved = false;
			for( Property *parent = prop->getParent(); parent; parent = parent->getParent() )
			{
				if( parent->getSchema()->getp("cppsym") )
				{
					resolved = true;
					sym.replace( macro_start, macro_len, getCppSymbol(parent) );
					break;
				}
			}

			if( !resolved )
				propSym.err( "Cannot resolve $[ancestor]" );
		} 
		else if( macro_args[0] == "gene" )
		{
			if( macro_args.size() != 3 )
				propSym.err( "expecting $[gene, GENE_NAME, MEMBER]" );

			string geneName = macro_args[1];
			string member = macro_args[2];
			string expansion;

			string err = propSym.getLocation().getDescription() + ": Cannot find gene '" + geneName + "'";

			if( member == "min" )
			{
				expansion = string("genome::GenomeUtil::getGene(\"") + geneName + "\", \"" + err + "\")->to___Interpolated()->smin.__val";
			}
			else if( member == "max" )
			{
				expansion = string("genome::GenomeUtil::getGene(\"") + geneName + "\", \"" + err + "\")->to___Interpolated()->smax.__val";
			}
			else
			{
				propSym.err( "Invalid gene member." );
			}

			sym.replace( macro_start, macro_len, expansion );
		}
		else
		{
			break;
		}
	}

	return sym;
}

void DynamicProperties::getDynamicProperties( Property *container, DynamicPropertyList &result )
{
	itfor( PropertyMap, container->props(), it )
	{
		Property *prop = it->second;
		switch( prop->getType() )
		{
		case Node::Object:
		case Node::Array:
			getDynamicProperties( prop, result );
			break;
		case Node::Scalar:
			if( prop->getSubtype() == Node::Dynamic )
				result.push_back( dynamic_cast<DynamicScalarProperty *>(prop) );
			break;
		default:
			// no-op
			break;
		}
	}
}

void DynamicProperties::sortDynamicProperties( DynamicPropertyList &properties,
											   map<DynamicScalarProperty *, DynamicPropertyList> &prop2antecedents )
{
	struct local
	{
		static bool contains( DynamicPropertyList &result, DynamicScalarProperty *prop )
		{
			itfor( DynamicPropertyList, result, it )
				if( *it == prop )
					return true;
			
			return false;
		}

		static void insert( DynamicScalarProperty *prop,
							DynamicPropertyList &result,
							DynamicPropertyList &inserting,
							map<DynamicScalarProperty *, DynamicPropertyList> &prop2antecedents )
		{
			if( contains(result, prop) || contains(inserting, prop) )
				return;

			inserting.push_back( prop );

			itfor( DynamicPropertyList, prop2antecedents[prop], it )
				insert( *it, result, inserting, prop2antecedents );

			result.push_back( prop );

			inserting.pop_back();
		}
	};

	DynamicPropertyList result;
	DynamicPropertyList inserting;

	itfor( DynamicPropertyList, properties, it )
		local::insert( *it, result, inserting, prop2antecedents );

	assert( result.size() == properties.size() );

	copy( result.begin(), result.end(), properties.begin() );

	assert( result.size() == properties.size() );
}
