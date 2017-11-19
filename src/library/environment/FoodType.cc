#include "FoodType.h"

#include <assert.h>

#include "utils/misc.h"

using namespace std;


map<string, const FoodType *> FoodType::foodTypes;
vector<FoodType *> FoodType::foodTypesVector;


FoodType::FoodType( string _name,
					Color _color,
					EnergyPolarity _energyPolarity,
					EnergyMultiplier _eatMultiplier,
					Energy _depletionThreshold )
	: name( _name )
	, color( _color )
	, energyPolarity( _energyPolarity )
	, eatMultiplier( _eatMultiplier )
	, depletionThreshold( _depletionThreshold )
{
}

void FoodType::define( string _name,
					   Color _color,
					   EnergyPolarity _energyPolarity,
					   EnergyMultiplier _eatMultiplier,
					   Energy _depletionThreshold )
{
	FoodType *foodType = new FoodType( _name,
									   _color,
									   _energyPolarity,
									   _eatMultiplier,
									   _depletionThreshold );
	foodTypes[_name] = foodType;
	foodTypesVector.push_back( foodType );
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

FoodType *FoodType::get( int index )
{
	return foodTypesVector[index];
}

int FoodType::getNumberDefinitions() {
	return (int)foodTypes.size();
}
