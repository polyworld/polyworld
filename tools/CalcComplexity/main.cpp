// Command line tool to calculate the Complexity of a brainfunction file.
// Compile with: g++ CalcComplexity.cpp -lgsl -lgslcblas -lm -o CalcComplexity
// Return values:
// >0	actual complexity
// 0	not enough timesteps in agentlifetime
// -1	User specified not to calculate this value
// -2	there was no value for numimputneurons in the BrainFunction file
 
//set me to 200 for a Olaf-recommended value.

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <string>

#include "main.h"

#include "brainfunction.h"
#include "motion.h"

using namespace std;


//---------------------------------------------------------------------------
// main
//---------------------------------------------------------------------------
int main( int argc, char *argv[] )
{
	int exit_value = 0;

	if(argc < 2)
	{
		show_usage("Must specify mode.");
	}

	string mode = consume_arg(argc, argv, 1);

	if(mode == "motion")
	{
		exit_value = process_motion(argc, argv);
	}
	else if(mode == "brainfunction")
	{
		exit_value = process_brainfunction(argc, argv);
	}
	else
	{
		show_usage(string("invalid mode: ") + mode);
	}

	return exit_value;
}

//---------------------------------------------------------------------------
// show_usage
//---------------------------------------------------------------------------
void show_usage(string msg)
{
	cerr << "Usage:" << endl;

	cerr << endl;
	cerr << "--- Brain Function ---" << endl << endl;

	usage_brainfunction();	// in tools/CalcComplexity/brainfunction.cpp
	
	cerr << endl;
	cerr << "--- Motion ---" << endl << endl;

	usage_motion();			// in tools/CalcComplexity/motion.cpp

	cerr << endl;

	if(msg.length() > 0)
	{
		cerr << "---------------" << endl << endl;
		cerr << msg << endl << endl;
	}

	exit(1);
}

//---------------------------------------------------------------------------
// consume_arg
//---------------------------------------------------------------------------
const char *consume_arg(int &argc, char *argv[], int argi)
{
	const char *arg = argv[argi];

	argc--;

	for(int i = argi; i < argc; i++)
	{
	  argv[i] = argv[i + 1];
	}

	return arg;
}

//---------------------------------------------------------------------------
// get_long_option
//---------------------------------------------------------------------------
long get_long_option(string option, int &argc, char *argv[], int argi)
{
	consume_arg(argc, argv, argi);

	if(argi == argc)
	{
		show_usage();
	}

	const char *sval = consume_arg(argc, argv, argi);
	char *endptr;

	long val = strtol(sval, &endptr, 10);

	if(*endptr)
	{
		show_usage(string("Invalid value for option ") + option);
	}

	return val;
}

//---------------------------------------------------------------------------
// get_float_option
//---------------------------------------------------------------------------
float get_float_option(string option, int &argc, char *argv[], int argi)
{
	consume_arg(argc, argv, argi);

	if(argi == argc)
	{
		show_usage();
	}

	const char *sval = consume_arg(argc, argv, argi);
	char *endptr;

	float val = strtof(sval, &endptr);

	if(*endptr)
	{
		show_usage(string("Invalid value for option ") + option);
	}

	return val;
}

// eof
