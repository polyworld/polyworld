#include "FittestList.h"

#include "agent.h"
#include "GenomeUtil.h"

using namespace genome;
using namespace std;


//===========================================================================
// FittestList
//===========================================================================

//---------------------------------------------------------------------------
// FittestList::FittestList
//---------------------------------------------------------------------------
FittestList::FittestList( int capacity, bool storeGenome )
: _capacity( capacity )
, _storeGenome( storeGenome )
, _size( 0 )
{
	_elements = new FitStruct*[ _capacity ];
	for( int i = 0; i < _capacity; i++ )
	{
		FitStruct *element = new FitStruct;
		element->genes = NULL;
		_elements[i] = element;
	}

	clear();
}

//---------------------------------------------------------------------------
// FittestList::~FittestList
//---------------------------------------------------------------------------
FittestList::~FittestList()
{
	for( int i = 0; i < _capacity; i++ )
	{
		if( _elements[i]->genes )
			delete _elements[i]->genes;
		delete _elements[i];
	}
	delete [] _elements;
}

//---------------------------------------------------------------------------
// FittestList::update
//---------------------------------------------------------------------------
void FittestList::update( agent *candidate, float fitness )
{
	if( !isFull() || (fitness > _elements[_size - 1]->fitness) )
	{
		int rank = -1;
		for( int i = 0; i < _size; i++ )
		{
			if( fitness > _elements[i]->fitness )
			{
				rank = i;
				break;
			}
		}
		if( rank == -1 )
		{
			assert( !isFull() );
			rank = _size;
		}

		if( !isFull() )
		{
			// We're growing the list. We may or may not need to allocate a genome, depending
			// on whether this list was previously a larger size but then cleared.
			if( _storeGenome && (_elements[_size]->genes == NULL) )
				_elements[_size]->genes = GenomeUtil::createGenome();
			_size++;
		}

		FitStruct *newElement = _elements[_size-1];
		for( int i = _size - 1; i > rank; i-- )
			_elements[i] = _elements[i-1];
		_elements[rank] = newElement;

		newElement->fitness = fitness;
		if( _storeGenome )
			newElement->genes->copyFrom( candidate->Genes() );
		newElement->agentID = candidate->Number();
		newElement->complexity = candidate->Complexity();
	}
}

//---------------------------------------------------------------------------
// FittestList::clear
//---------------------------------------------------------------------------
void FittestList::clear()
{
	_size = 0;

	// todo: this shouldn't be necessary. retained to ensure backwards-compatible
	for( int i = 0; i < _capacity; i++ )
	{
		_elements[i]->fitness = 0.0;
		_elements[i]->agentID = 0;
		_elements[i]->complexity = 0.0;
	}
}

//---------------------------------------------------------------------------
// FittestList::dump
//---------------------------------------------------------------------------
void FittestList::dump( ostream &out )
{
	for( int i = 0; i < _capacity; i++ )
	{
		out << _elements[i]->agentID << endl;
		out << _elements[i]->fitness << endl;
		out << _elements[i]->complexity << endl;
		
		assert( false );
		/* PORT TO AbstractFile
		if( _storeGenome )
			_elements[i]->dump(out);
		*/
	}
}
