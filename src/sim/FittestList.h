#pragma once

#include <assert.h>

#include <iostream>

#include "Genome.h"

//===========================================================================
// FitStruct
//===========================================================================
struct FitStruct
{
	unsigned long	agentID;
	float	fitness;
	float   complexity;
	genome::Genome *genes;
};
typedef struct FitStruct FitStruct;

//===========================================================================
// FittestList
//===========================================================================
class FittestList
{
 public:
	FittestList( int capacity, bool storeGenome );
	virtual ~FittestList();

	void update( class agent *candidate, float fitness );

	bool isFull();
	void clear();
	int size();

	// rank is 0-based
	FitStruct *get( int rank );

	void dump( std::ostream &out );

 private:
	int _capacity;
	bool _storeGenome;
	int _size;

 public: // tmp access
	FitStruct **_elements;
};

inline bool FittestList::isFull() { return _size == _capacity; }
inline int FittestList::size() { return _size; }
inline FitStruct *FittestList::get( int rank ) { assert(rank < _size); return _elements[rank]; }
