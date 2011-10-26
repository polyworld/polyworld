#pragma once

#include <string>
#include <vector>

#include "Energy.h"

// forward decls
class FoodType;

class Metabolism
{
 public:
	static void define( const std::string &name,
						const EnergyPolarity &energyPolarity,
						EnergyMultiplier &eatMultiplier,
						const FoodType *carcassFoodType );
	static int getNumberOfDefinitions();
	static const Metabolism *get( int index );

	const int index;
	const std::string name;
	const EnergyPolarity energyPolarity;
	EnergyMultiplier &eatMultiplier;
	const FoodType *carcassFoodType;

 private:
	Metabolism( int _index,
				const std::string &_name,
				const EnergyPolarity &_energyPolarity,
				EnergyMultiplier &_eatMultiplier,
				const FoodType *_carcassFoodType );

	static std::vector<const Metabolism *> metabolisms;
};
