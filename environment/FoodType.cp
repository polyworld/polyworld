#include "FoodType.h"

#include <assert.h>

#include "misc.h"

using namespace std;


map<string, const FoodType *> FoodType::foodTypes;


FoodType::FoodType( string _name,
					Color _color,
					EnergyPolarity _energyPolarity,
					Energy _depletionThreshold )
	: name( _name )
	, color( _color )
	, energyPolarity( _energyPolarity )
	, depletionThreshold( _depletionThreshold )
{
}

void FoodType::define( string _name,
					   Color _color,
					   EnergyPolarity _energyPolarity,
					   Energy _depletionThreshold )
{
	foodTypes[_name] = new FoodType( _name,
									 _color,
									 _energyPolarity,
									 _depletionThreshold );
}

const FoodType *FoodType::lookup( string name )
{
	return foodTypes[name];
}

const FoodType *FoodType::find( const EnergyPolarity &polarity )
{
	itfor( FoodTypeMap, foodTypes, it )
	{
		if( it->second->energyPolarity == polarity )
		{
			return it->second;
		}
	}

	return NULL;
}

int FoodType::getNumberDefinitions() {
	return (int)foodTypes.size();
}
