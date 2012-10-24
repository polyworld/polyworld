#include "Metabolism.h"

using namespace std;

vector<Metabolism *> Metabolism::metabolisms;
Metabolism::SelectionMode Metabolism::selectionMode;

Metabolism::Metabolism( int index_,
						const string &name_,
						const EnergyPolarity &energyPolarity_,
						EnergyMultiplier eatMultiplier_,
						const Energy &energyDelta_,
						float minEatAge_,
						const FoodType *carcassFoodType_ )
: index( index_ )
, name( name_ )
, energyPolarity( energyPolarity_ )
, eatMultiplier( eatMultiplier_ )
, energyDelta( energyDelta_ )
, minEatAge( minEatAge_ )
, carcassFoodType( carcassFoodType_ )
{
}

void Metabolism::define( const string &name,
						 const EnergyPolarity &energyPolarity,
						 EnergyMultiplier eatMultiplier,
						 const Energy &energyDelta,
						 float minEatAge,
						 const FoodType *carcassFoodType )
{
	Metabolism *metabolism = new Metabolism( metabolisms.size(),
											 name,
											 energyPolarity,
											 eatMultiplier,
											 energyDelta,
											 minEatAge,
											 carcassFoodType );
	metabolisms.push_back( metabolism );
}

int Metabolism::getNumberOfDefinitions()
{
	return metabolisms.size();
}

Metabolism *Metabolism::get( int index )
{
	return metabolisms[index];
}
