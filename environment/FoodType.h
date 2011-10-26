#pragma once

#include <map>
#include <string>

#include "Energy.h"
#include "graphics.h"

class FoodType
{
 public:
	static void define( std::string _name,
						Color _color,
						EnergyPolarity _energyPolarity,
						Energy _depletionThreshold );
	static const FoodType *lookup( std::string name );
	static const FoodType *find( const EnergyPolarity &energyPolarity  );
	static int getNumberDefinitions();

	const std::string name;
	const Color color;
	const EnergyPolarity energyPolarity;
	const Energy depletionThreshold;

 private:
	FoodType( std::string _name,
			  Color _color,
			  EnergyPolarity _energyPolarity,
			  Energy _depletionThreshold );

	typedef std::map<std::string, const FoodType *> FoodTypeMap;

	static FoodTypeMap foodTypes;
};
