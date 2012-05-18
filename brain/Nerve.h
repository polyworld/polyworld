#pragma once

#include <string>

class Nerve
{
 public:
	enum Type
	{
		INPUT = 0,
		OUTPUT,
		__NTYPES
	};

	enum ActivationBuffer
	{
		CURRENT = 0,
		SWAP,
		__NBUFFERS
	};

	const Type type;
	const std::string name;

 private:
	friend class NervousSystem;

	Nerve( Type type,
		   const std::string &name,
		   int igroup );

 public:
	float get( int ineuron = 0,
			   ActivationBuffer buf = CURRENT );
	void set( float activation,
			  ActivationBuffer buf = CURRENT );
	void set( int ineuron,
			  float activation,
			  ActivationBuffer buf = CURRENT );
	int getIndex();
	int getNeuronCount();

 public:
	void config( int numneurons,
				 int index );
	void config( float **activations,
				 float **activations_swap );
	
 private:
	int numneurons;
	float **activations[__NBUFFERS];
	int index;
};
