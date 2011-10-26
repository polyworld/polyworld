#include "Metabolism.h"

using namespace std;

vector<const Metabolism *> Metabolism::metabolisms;

Metabolism::Metabolism( int _index,
						const string &_name,
						const EnergyPolarity &_energyPolarity,
						EnergyMultiplier &_eatMultiplier,
						const FoodType *_carcassFoodType )
: index( _index )
, name( _name )
, energyPolarity( _energyPolarity )
, eatMultiplier( _eatMultiplier )
, carcassFoodType( _carcassFoodType )
{
}

void Metabolism::define( const string &name,
						 const EnergyPolarity &energyPolarity,
						 EnergyMultiplier &eatMultiplier,
						 const FoodType *carcassFoodType )
{
	Metabolism *metabolism = new Metabolism( metabolisms.size(),
											 name,
											 energyPolarity,
											 eatMultiplier,
											 carcassFoodType );
	metabolisms.push_back( metabolism );
}

int Metabolism::getNumberOfDefinitions()
{
	return metabolisms.size();
}

const Metabolism *Metabolism::get( int index )
{
	return metabolisms[index];
}
