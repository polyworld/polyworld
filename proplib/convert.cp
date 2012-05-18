#include "convert.h"

#include <fstream>
#include <map>
#include <stack>

#include "builder.h"
#include "dom.h"
#include "editor.h"
#include "misc.h"
#include "parser.h"

using namespace std;
using namespace proplib;

#define REMOVE( CONTAINER, PROPNAME )					\
	{													\
		Property *prop = (CONTAINER)->getp(PROPNAME);	\
		if( prop )										\
		{												\
			editor->remove( prop );						\
		}												\
	}

#define SETIF( CONTAINER, PROPNAME, OLDVAL, NEWVAL )		\
	{														\
		Property *prop = CONTAINER->getp(PROPNAME);			\
		if( prop )											\
		{													\
			if( (string)*prop == OLDVAL )					\
			{												\
				editor->set( prop, NEWVAL );				\
			}												\
		}													\
	}														\


#define RAWVAL( PROP ) dynamic_cast<ConstScalarProperty *>(PROP)->getExpression()->toString(false)

#define SETIFRAW( CONTAINER, PROPNAME, OLDVAL, NEWVAL )		\
	{														\
		Property *prop = CONTAINER->getp(PROPNAME);			\
		if( prop )											\
		{													\
			if( RAWVAL(prop) == OLDVAL )					\
			{												\
				editor->set( prop, NEWVAL );				\
			}												\
		}													\
	}														\


bool WorldfileConverter::isV1( string path )
{
	ifstream in( path.c_str() );
	return in.get() != '@';
}

void WorldfileConverter::convertV1SyntaxToV2( string pathIn, string pathOut )
{
	stack<bool> isObjectArray;

	Tokenizer tokenizer( pathIn, new ifstream(pathIn.c_str()) );

	ofstream out( pathOut.c_str() );

	out << "@version 2" << endl;

	for( Token *tok = tokenizer.next(); tok->type != Token::Eof; tok = tokenizer.next() )
	{
		if( tok->type == Token::LeftSquare )
		{
			out << Token::toString( tok, tok, true );
			tok = tokenizer.next();

			switch( tok->type )
			{
			case Token::RightSquare:
				// no-op
				break;
			case Token::Number:
				isObjectArray.push( false );
				break;
			default:
				isObjectArray.push( true );
				out << " { ";
				break;
			}

			out << Token::toString( tok, tok, true );
		}
		else if( tok->type == Token::RightSquare )
		{
			if( isObjectArray.top() )
			{
				out << tok->getDecorationString();
				out << " } ";
				out << tok->text;
			}
			else
			{
				out << Token::toString( tok, tok, true );
			}

			isObjectArray.pop();
		}
		else if( tok->type == Token::Comma )
		{
			if( isObjectArray.top() )
			{
				out << tok->getDecorationString();
				out << " } , { ";
			}
			else
				out << Token::toString( tok, tok, true );
		}
		else if( (tok->type == Token::Misc) && (tok->text == "$") )
		{
			out << " ";
		}
		else if( (tok->type == Token::Misc) && (tok->text == "\\\"") )
		{
			out << tok->getDecorationString();
			out << '\"';
		}
		else
		{
			out << Token::toString( tok, tok, true );
		}
	}
}

void WorldfileConverter::convertV1PropertiesToV2( DocumentEditor *editor, Document *doc )
{
#define QUOTE( CONTAINER, PROPNAME )									\
	{																	\
		Property *prop = (CONTAINER)->getp(PROPNAME);					\
		if( prop )														\
		{																\
			string value = RAWVAL(prop);								\
			editor->set( prop, string("\"") + value + "\"" );			\
		}																\
	}

#define CONDITION_TO_DYN( CONTAINER, PROPNAME )							\
	{																	\
		Property *prop = CONTAINER->getp(PROPNAME);						\
		if( prop && prop->getType() == Node::Object )					\
		{																\
			Property &conditions = prop->get("Conditions");				\
			if( conditions.size() > 1 )									\
				conditions.err( "Cannot auto convert multiple conditions" ); \
			editor->remove( prop );										\
			Property *value = &( conditions.get(0).get("Value") );		\
			editor->rename( value, PROPNAME, false );					\
			editor->move( value, CONTAINER, false );					\
		}																\
	}																	\


	// ---
	// --- Monitor
	// ---
	REMOVE( doc, "AgentTracking" );
	REMOVE( doc, "BrainMonitorFrequency" );
	REMOVE( doc, "CameraAngleStart" );
	REMOVE( doc, "CameraColor" );
	REMOVE( doc, "CameraFieldOfView" );
	REMOVE( doc, "CameraHeight" );
	REMOVE( doc, "CameraRadius" );
	REMOVE( doc, "CameraRotationRate" );
	REMOVE( doc, "ChartBorn" );
	REMOVE( doc, "ChartFitness" );
	REMOVE( doc, "ChartFoodEnergy" );
	REMOVE( doc, "ChartGeneSeparation" );
	REMOVE( doc, "ChartPopulation" );
	REMOVE( doc, "MonitorAgentRank" );
	REMOVE( doc, "MonitorGeneSeparation" );
	REMOVE( doc, "OverHeadRank" );
	REMOVE( doc, "ShowVision" );
	REMOVE( doc, "StatusFrequency" );
	REMOVE( doc, "StatusToStdout" );

	// ---
	// --- Logging
	// ---
	REMOVE( doc, "RecordFoodPatchStats" );
	REMOVE( doc, "RecordMovie" );
	REMOVE( doc, "RecordGeneSeparation" );
	REMOVE( doc, "RecordPerformanceStats" );
	SETIF( doc, "RecordPosition", "True", "Precise" );
	SETIF( doc, "RecordSeparations", "True", "Contact" );

	// ---
	// --- Conditionals
	// ---
	CONDITION_TO_DYN( doc, "EatMateMinDistance" );
	CONDITION_TO_DYN( doc, "MaxEatVelocity" );
	CONDITION_TO_DYN( doc, "MaxEatYaw" );
	CONDITION_TO_DYN( doc, "MaxMateVelocity" );
	CONDITION_TO_DYN( doc, "MinEatVelocity" );
	{
		Property *prop = doc->getp("EnergyUseMateMode");
		if( prop && (RAWVAL(prop) != "Constant") )
			prop->err( "Cannot auto convert conditional" );
	}
	REMOVE( doc, "EnergyUseMateConditional" );
	REMOVE( doc, "EnergyUseMateMode" );

	// ---
	// --- Misc
	// ---
	QUOTE( doc, "ComplexityType" );
	REMOVE( doc, "StickyEdges" );
	REMOVE( doc, "WrapAround" );

	// ---
	// --- AgentMetabolisms
	// ---
	{
		Property *metabolisms = doc->getp("AgentMetabolisms");
		if( metabolisms )
		{
			itfor( PropertyMap, metabolisms->elements(), it )
			{
				Property *metabolism = it->second;

				CONDITION_TO_DYN( metabolism, "EatMultiplier" );
				CONDITION_TO_DYN( metabolism, "MinEatAge" );
				QUOTE( metabolism, "CarcassFoodTypeName" );
				QUOTE( metabolism, "Name" );
			}
		}
	}

	// ---
	// --- Barriers
	// ---
	{
		Property *barriers = doc->getp( "Barriers" );
		if( barriers )
		{
			itfor( PropertyMap, barriers->elements(), it )
			{
				Property *barrier = it->second;
				Property *keyframes = barrier->getp( "KeyFrames" );
				if( keyframes )
				{
					if( keyframes->size() > 1 )
						keyframes->err( "Cannot auto convert" );
					editor->removeChildren( barrier );

					Property *keyframe0 = keyframes->elements().begin()->second;

					itfor( PropertyMap, keyframe0->props(), it_coord )
					{
						Property *coord = it_coord->second;
						if( (coord->getName()[0] == 'X') || (coord->getName()[0] == 'Z') )
							editor->move( it_coord->second, barrier, false );
					}
				}
			}
		}
	}

	// ---
	// --- Domains
	// ---
	{
		Property *domains = doc->getp( "Domains" );
		if( domains )
		{
			itfor( PropertyMap, domains->elements(), it_domain )
			{
				Property *domain = it_domain->second;

				Property *foodPatches = domain->getp( "FoodPatches" );
				if( foodPatches )
				{
					itfor( PropertyMap, foodPatches->props(), it_patch )
					{
						Property *patch = it_patch->second;
						QUOTE( patch, "FoodTypeName" );
						REMOVE( patch, "OnCondition" );
					}
				}
			}
		}
	}

	// ---
	// --- FoodTypes
	// ---
	{
		Property *foodTypes = doc->getp( "FoodTypes" );
		if( foodTypes )
		{
			itfor( PropertyMap, foodTypes->elements(), it_foodType )
			{
				Property *foodType = it_foodType->second;
				QUOTE( foodType, "Name" );
			}
		}
	}

	// ---
	// --- Brain recording
	// ---
	{
		REMOVE( doc, "BrainAnatomyRecordSeeds" );
		REMOVE( doc, "BrainFunctionRecordSeeds" );

		{
			bool setRecordBrain = false;
			bool recordBrain = false;

			{
				Property *prop = doc->getp("BrainAnatomyRecordAll");
				if( prop )
				{
					setRecordBrain = true;
					recordBrain |= (bool)*prop;
					editor->remove( prop );
				}
			}

			{
				Property *prop = doc->getp("BrainFunctionRecordAll");
				if( prop )
				{
					setRecordBrain = true;
					recordBrain |= (bool)*prop;
					editor->remove( prop );
				}
			}

			if( setRecordBrain )
			{
				editor->set( "RecordBrain", "True" );
			}
		}

		{
			bool setFrequency = false;
			string frequency;
			typedef map<string,bool> RecordMap;
			RecordMap record;

			// --------------------------------------
#define SET_FREQUENCY(OLDPROPNAME,RECORD_NAME)							\
			{															\
				Property *prop = doc->getp( OLDPROPNAME );				\
				if( prop )												\
				{														\
					string freq = *prop;								\
					if( !frequency.empty() && (freq != "0") && (frequency != "0") && (freq != frequency) ) \
						prop->err( "Found different non-zero brain record frequencies!" ); \
					if( freq != "0" )									\
					{													\
						setFrequency = true;							\
						frequency = freq;								\
					}													\
					record[ RECORD_NAME ] |= (freq != "0");				\
					editor->remove( prop );								\
				}														\
			}
			// --------------------------------------

			SET_FREQUENCY("BrainFunctionRecentRecordFrequency", "RecordBrainRecent" );
			SET_FREQUENCY("BestRecentBrainAnatomyRecordFrequency", "RecordBrainBestRecent" );
			SET_FREQUENCY("BestRecentBrainFunctionRecordFrequency", "RecordBrainBestRecent" );
			SET_FREQUENCY("BestSoFarBrainAnatomyRecordFrequency", "RecordBrainBestSoFar" );
			SET_FREQUENCY("BestSoFarBrainFunctionRecordFrequency", "RecordBrainBestSoFar" );


			if( setFrequency )
				editor->set( "EpochFrequency", frequency );

			itfor( RecordMap, record, it )
			{
				editor->set( it->first, it->second ? "True" : "False" );
			}

#undef SET_FREQUENCY
		}
	}

}

void WorldfileConverter::convertDeprecatedV2Properties( DocumentEditor *editor, Document *doc )
{
	REMOVE( doc, "MinBiasLrate" );
	REMOVE( doc, "MaxBiasLrate" );

	SETIFRAW( doc, "GenomeLayout", "L", "None" );
	SETIFRAW( doc, "GenomeLayout", "N", "NeurGroup" );

	// ---
	// --- LegacyMode
	// ---
	{
		Property *legacy = doc->getp( "LegacyMode" );
		if( legacy )
		{
			if( (bool)*legacy )
				editor->setMeta( "@defaults", "legacy" );

			editor->remove( legacy );
		}
	}

	// ---
	// --- FoodTypes
	// ---
	{
		Property *foodTypes = doc->getp( "FoodTypes" );
		if( foodTypes )
		{
			itfor( PropertyMap, foodTypes->elements(), it_foodType )
			{
				Property *foodType = it_foodType->second;

				Property *overrideColor = foodType->getp( "OverrideFoodColor" );
				if( overrideColor )
				{
					if( !(bool)*overrideColor )
						REMOVE( foodType, "FoodColor" );

					editor->remove( overrideColor );
				}
			}
		}
	}

	// ---
	// --- Domains
	// ---
	{
		Property *domains = doc->getp( "Domains" );
		if( domains )
		{
			itfor( PropertyMap, domains->elements(), it_domain )
			{
				Property *domain = it_domain->second;

				Property *brickPatches = domain->getp( "BrickPatches" );
				if( brickPatches )
				{
					itfor( PropertyMap, brickPatches->props(), it_patch )
					{
						Property *patch = it_patch->second;
						Property *overrideColor = patch->getp( "OverrideBrickColor" );
						if( overrideColor )
						{
							if( !(bool)*overrideColor )
								REMOVE( patch, "BrickColor" );

							editor->remove( overrideColor );
						}
					}
				}
			}
		}
	}
}
