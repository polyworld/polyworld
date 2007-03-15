// Command line tool to calculate the Complexity of a brainfunction file.
// Compile with: g++ CalcComplexity.cpp -lgsl -lgslcblas -lm -o CalcComplexity.o

// Run with: ./CalcComplexity.o <path/to/brainfunction/filename.txt>
// Ex:       ./CalcComplexity.o /polyworld/run/brain/function/brainFunction_99.txt

//set me to 200 for a Olaf-recommended value.
#define WARN_IF_COMPUTING_COMPLEXITY_OVER_LESSTHAN_N_TIMESTEPS 0
#define CalcAll 0
#define CalcProcessing 1
#define CalcInput 0

#include <iostream.h>
#include "complexity.h"


int main( int argc, char *argv[] )
{
	if( argc != 2 && argc != 3 )	// don't have 1 or 2 arguments.  Spit out usage information.
	{
		cout << "Usage: CalcComplexity brainFunction_log.txt [N]" << endl << endl;
		cout << "* The first argument is the brainFunction file to compute the complexity over.  Both complete and incomplete brainFunction files are supported." << endl;
		cout << "* The 2nd argument is optional, and is the number of timesteps since the beginning of the agent's life to compute the Complexity over.  Ex: a value of 100 will compute Complexity across the first 100 steps of the agent's life." << endl; 
		exit(0);
	}

	int ignore_timesteps_after = 0;
	if( argc == 3 )		// we have 2 arguments, so set the ignore_timesteps_after to a positive value.
	{
		ignore_timesteps_after = atoi(argv[2]);
		assert( ignore_timesteps_after > 0 );
		if( ignore_timesteps_after < WARN_IF_COMPUTING_COMPLEXITY_OVER_LESSTHAN_N_TIMESTEPS )
			cerr << "Warning: Computing Complexity with sampling less than " << WARN_IF_COMPUTING_COMPLEXITY_OVER_LESSTHAN_N_TIMESTEPS << " timesteps." << endl;
	}

//	cout << "LogFile = " << argv[1] << endl;
	double Complexity = 0;
	if( CalcAll )
		Complexity = CalcComplexity( argv[ 1 ] , 'A', ignore_timesteps_after );
	else
		Complexity = -2;

	cout << "Complexity (All) =\t" << Complexity << endl;

	if( CalcProcessing )
		Complexity = CalcComplexity( argv[ 1 ] , 'P', ignore_timesteps_after );
	else
		Complexity = -2;
	cout << "Complexity (Processing) =\t" << Complexity << endl;

	if( CalcInput )
		Complexity = CalcComplexity( argv[ 1 ] , 'I', ignore_timesteps_after );
	else
		Complexity = -2;
	cout << "Complexity (Input) =\t" << Complexity << endl;
                
	return 0;
}
