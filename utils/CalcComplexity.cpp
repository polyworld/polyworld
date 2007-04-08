// Command line tool to calculate the Complexity of a brainfunction file.
// Compile with: g++ CalcComplexity.cpp -lgsl -lgslcblas -lm -o CalcComplexity
// Return values:
// >0	actual complexity
// 0	not enough timesteps in critterlifetime
// -1	User specified not to calculate this value
// -2	there was no value for numimputneurons in the BrainFunction file
 
//set me to 200 for a Olaf-recommended value.
#define WARN_IF_COMPUTING_COMPLEXITY_OVER_LESSTHAN_N_TIMESTEPS 0

#include <iostream.h>
#include <ctype.h>		// for isdigit()
#include "complexity.h"

int main( int argc, char *argv[] )
{
//########################################################################
/* DEFAULT VALUES */
	int CalcAll = 1;
	int CalcProcessing = 1;
	int CalcInput = 1;
	int ignore_timesteps_after = 0;			// by default, calculate complexity over the critter's entire lifetime.
/* END DEFAULT VALUES */
//########################################################################

	if( argc < 2 || argc > 4 )	// if don't have 1, 2, or 3 arguments.  Spit out usage information.
	{
		cout << "\tUsage: CalcComplexity brainFunction_log.txt [N] [API]" << endl << endl;
		cout << "\tThe first argument is the brainFunction file to compute the complexity over.\nBoth complete and incomplete brainFunction files are supported." << endl;
		cout << "\tThe 2nd argument is optional, and is the number of timesteps since the beginning of the agent's life to compute the Complexity over.  Ex: a value of 100 will compute Complexity across the first 100 steps of the agent's life." << endl; 
		cout << "\tThe 3rd argument is optional, and can be 'A', 'P', 'I', or any combination thereof.  It specifies whether you want to compute the Complexity of All, Processing, or Input neurons.  By default it computes the Complexity for A, P, and I." << endl; 
		exit(1);
	}

	// we're going to parse our extra parameters N and API (if we have them)
	for( int i=2; i<argc; i++ )
	{
		char firstchar = (argv[i])[0];
		if( isdigit(firstchar) )			// is the parameter a digit?  If so, treat it as ignore_timesteps_after...
		{
			ignore_timesteps_after = atoi( argv[i] );
			if( ignore_timesteps_after < WARN_IF_COMPUTING_COMPLEXITY_OVER_LESSTHAN_N_TIMESTEPS )
			cerr << "* Warning: Computing Complexity with less than " << WARN_IF_COMPUTING_COMPLEXITY_OVER_LESSTHAN_N_TIMESTEPS << " time samples." << endl;
		}
		else if( firstchar == 'A' || firstchar == 'P' || firstchar == 'I' )	//Make sure firstchar is either A,P, or I
		{
			CalcAll = 0; CalcProcessing = 0; CalcInput = 0;
			for( int j=0; j<strlen( argv[i] ); j++ )
			{
				if( (argv[i])[j] == 'A' )	CalcAll = 1;
				else if( (argv[i])[j] == 'P' )	CalcProcessing = 1;
				else if( (argv[i])[j] == 'I' )	CalcInput = 1;
				else { cout << "* Error:  Didnt know letter '" << argv[i][j]  << "' that you specified.  Exiting." << endl; exit(1); }
			}
		}	
		else
		{
			cout << "* Error:  Didn't understand argument '" << argv[i] << "'" << endl;
			exit(1);
		}
	}

	assert( ignore_timesteps_after >= 0 );
	if( ignore_timesteps_after < WARN_IF_COMPUTING_COMPLEXITY_OVER_LESSTHAN_N_TIMESTEPS )
		cerr << "* Warning: Computing Complexity with less than " << WARN_IF_COMPUTING_COMPLEXITY_OVER_LESSTHAN_N_TIMESTEPS << " time samples." << endl;
/*
	cout << "ignore timesteps after=" << ignore_timesteps_after << endl;
	cout << "LogFile = " << argv[1] << endl;
	cout << "CalcAll = " << CalcAll << endl;
	cout << "CalcPro = " << CalcProcessing << endl;
	cout << "CalcInp = " << CalcInput << endl;
*/
	double Complexity = 0;
	if( CalcAll )
		Complexity = CalcComplexity( argv[ 1 ] , 'A', ignore_timesteps_after );
	else
		Complexity = -1;

	cout << "Complexity (All) =\t" << Complexity << endl;

	if( CalcProcessing )
		Complexity = CalcComplexity( argv[ 1 ] , 'P', ignore_timesteps_after );
	else
		Complexity = -1;
	cout << "Complexity (Processing) =\t" << Complexity << endl;

	if( CalcInput )
		Complexity = CalcComplexity( argv[ 1 ] , 'I', ignore_timesteps_after );
	else
		Complexity = -1;
	cout << "Complexity (Input) =\t" << Complexity << endl;
                
	return 0;
}
