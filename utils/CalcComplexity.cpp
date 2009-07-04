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
#include <string>
#include "complexity.h"

int main( int argc, char *argv[] )
{
//########################################################################
/* DEFAULT VALUES */
/*	int CalcAll = 1;
	int CalcProcessing = 1;
	int CalcInput = 1;
	int CalcBehavior = 1;
	int CalcHealthBehavior = 1;*/
	char Default_comtypes[][5] = {"A","P","I","B","HB"};
	int ignore_timesteps_after = 0;	// by default, calculate complexity over the critter's entire lifetime.
	int num_Com;
	double Complexity = 0;
/* END DEFAULT VALUES */
//########################################################################

	if( argc < 2 )	// if don't  1, or more arguments.  Spit out usage information.  
	{
		cout << "Usage: CalcComplexity brainFunction_log.txt [N] [A P I B HB]" << endl;
		cout << "\tThe first argument is the brainFunction file to compute the complexity for.\n\t\tBoth complete and incomplete brainFunction files are supported." << endl;
		cout << "\tThe 2nd argument is optional, and is the number of timesteps since the beginning\n\t\tof the agent's life to compute the Complexity over.\n\t\tEx: a value of 100 will compute Complexity across the first 100 steps of the agent's life." << endl; 
		cout << "\tThe 3rd argument is optional, and can be 'A', 'P', 'I', 'B', 'H' or any meaningful combination thereof.\n\t\tIt specifies whether you want to compute the Complexity of All, Processing, Input, Behavior, or Health+Behavior neurons.\n\t\tBy default it computes the Complexity for A, P, I, B, and HB." << endl; 
		exit(1);
	}
	
	// we're going to parse our extra parameters N and A P I B H (if we have them)
	if( argc > 3 )
	{
		if (isdigit(argv[2][0]))
			num_Com = argc - 3;
		else
			num_Com = argc - 2;
			
		char *Com_types[num_Com];//record all the types of combination of complexities
		int CalcAll[num_Com];
		int CalcProcessing[num_Com];
		int CalcInput[num_Com];
		int CalcBehavior[num_Com];
		int CalcHealth[num_Com];
		
		int e = 0;
		for( int i=2; i<argc; i++ )//parse the command of complexities combinations one by one
		{
			char firstchar = (argv[i])[0];
			if( isdigit(firstchar) )			// is the parameter a digit?  If so, treat it as ignore_timesteps_after...
			{
				ignore_timesteps_after = atoi( argv[i] );
				if( ignore_timesteps_after < WARN_IF_COMPUTING_COMPLEXITY_OVER_LESSTHAN_N_TIMESTEPS )
				cerr << "* Warning: Computing Complexity with less than " << WARN_IF_COMPUTING_COMPLEXITY_OVER_LESSTHAN_N_TIMESTEPS << " time samples." << endl;
			}
			else if( firstchar == 'A' || firstchar == 'P' || firstchar == 'I' || firstchar == 'B' || firstchar == 'H')	//Make sure firstchar is either A,P,I,B or H
			{
				CalcAll[e] = 0; CalcProcessing[e] = 0; CalcInput[e] = 0; CalcBehavior[e] = 0;CalcHealth[e] = 0;
				for( int j=0; j<strlen( argv[i] ); j++ )
				{
					if( (argv[i])[j] == 'A' )	CalcAll[e] = 1;
					else if( (argv[i])[j] == 'P' )	CalcProcessing[e] = 1;
					else if( (argv[i])[j] == 'I' )	CalcInput[e] = 1;
					else if( (argv[i])[j] == 'B' )  CalcBehavior[e] = 1;
					else if( (argv[i])[j] == 'H' )  CalcHealth[e] = 1;
					else { cout << "* Error: Didnt know letter '" << argv[i][j]  << "' that you specified.  Exiting." << endl; exit(1); }
				}
				Com_types[e++] = argv[i];
			}	
			else
			{
				cout << "* Error:  Didn't understand argument'" << argv[i] << "'" << endl;
				exit(1);
			}
		}
	
		assert( ignore_timesteps_after >= 0 );
		if( ignore_timesteps_after < WARN_IF_COMPUTING_COMPLEXITY_OVER_LESSTHAN_N_TIMESTEPS )
			cerr << "* Warning: Computing Complexity with less than " << WARN_IF_COMPUTING_COMPLEXITY_OVER_LESSTHAN_N_TIMESTEPS << " time samples." << endl;

		string Com_Flag;
		string Output_Flag;
		for (int j=0;j<num_Com;j++)
		{
			Com_Flag = "";
			Output_Flag = "Complexity (";
			if (CalcAll[j]) {Com_Flag += "All";}
			if (CalcInput[j]) {Com_Flag += "Input";}
			if (CalcHealth[j]) {Com_Flag += "Health";}
			if (CalcProcessing[j]) {Com_Flag += "Processing";}
			if (CalcBehavior[j]) {Com_Flag += "Behavior";}
			Output_Flag += (Com_Flag + ") =\t");
			Complexity = CalcComplexity( argv[1], Com_types[j], ignore_timesteps_after);
			cout << Output_Flag << Complexity <<endl;
		}
	}
	else	// No complexity types specified, so use the default settings
	{
		Complexity = CalcComplexity( argv[1], Default_comtypes[0], ignore_timesteps_after);
		cout << "Complexity (All) =\t" << Complexity << endl;
		
		Complexity = CalcComplexity( argv[1], Default_comtypes[1], ignore_timesteps_after);
		cout << "Complexity (Processing) =\t" << Complexity << endl;
		
		Complexity = CalcComplexity( argv[1], Default_comtypes[2], ignore_timesteps_after);
		cout << "Complexity (Input) =\t" << Complexity << endl;
		
		Complexity = CalcComplexity( argv[1], Default_comtypes[3], ignore_timesteps_after);
		cout << "Complexity (Behavior) =\t" << Complexity << endl;
		
		Complexity = CalcComplexity( argv[1], Default_comtypes[4], ignore_timesteps_after);
		cout << "Complexity (HealthBehavior) =\t" << Complexity << endl;
	}
                
	return 0;
}
