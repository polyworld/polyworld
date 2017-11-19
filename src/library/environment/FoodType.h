#pragma once

#include <map>
#include <string>
#include <vector>

#include "Energy.h"
#include "graphics/graphics.h"

class FoodType
{
 public:
	static void define( std::string _name,
						Color _color,
						EnergyPolarity _energyPolarity,
						EnergyMultiplier _eatMultiplier,
						Energy _depletionThreshold );
	static const FoodType *lookup( std::string name );
	static const FoodType *find( const EnergyPolarity &energyPolarity  );
	static FoodType *get( int index );
	static int getNumberDefinitions();

	const std::string name;
	const Color color;
	const EnergyPolarity energyPolarity;
	EnergyMultiplier eatMultiplier;
	const Energy depletionThreshold;

 private:
	FoodType( std::string _name,
			  Color _color,
			  EnergyPolarity _energyPolarity,
			  EnergyMultiplier _eatMultiplier,
			  Energy _depletionThreshold );

	typedef std::map<std::string, const FoodType *> FoodTypeMap;

	static FoodTypeMap foodTypes;
	static std::vector<FoodType *> foodTypesVector;
};
