// Command line tool to calculate the Complexity of a brainfunction file.
// Compile with: g++ CalcComplexity.cpp -lgsl -lgslcblas -lm -o CalcComplexity.o

// Run with: ./CalcComplexity.o <path/to/brainfunction/filename.txt>
// Ex:       ./CalcComplexity.o /polyworld/run/brain/function/brainFunction_99.txt

#include <iostream.h>
#include "complexity.h"

int main( int argc, char *argv[] )
{

	if( argc == 0 )		// Didn't provide an argument?
	{
		cout << "You must provide the filename of a brainfunction logfile to compute the Complexity of." << endl;
		exit(0);
	}

	else if( argc != 2 )		// More than 1 argument ?
	{
		cout << "You may specify only ONE brainfunction logfile to compute the Complexity of." << endl;
		exit(0);
	}

//	cout << "LogFile = " << argv[1] << endl;

	double Complexity = 0;
	Complexity = CalcComplexity( argv[ 1 ] );

	cout << "Complexity = " << Complexity << endl;

	return 0;
}
