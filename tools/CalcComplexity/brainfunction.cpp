#define BRAINFUNCTION_CPP

#include <ctype.h>		// for isdigit()
#include <errno.h>
#include <string.h>

#include <iostream>
#include <list>
#include <map>
#include <string>

#include "brainfunction.h"

#include "main.h"

using namespace std;

map<char, string> part_names;

//set me to 200 for a Olaf-recommended value.
#define WARN_IF_COMPUTING_COMPLEXITY_OVER_LESSTHAN_N_TIMESTEPS 0

const char *DEFAULT_PART_COMBOS[5] = {"A","P","I","B","HB"};

#define DEBUG 0

#if DEBUG
	#define DEBUG_STMT(STMT) STMT
#else
	#define DEBUG_STMT(STMT)
#endif

#if DEBUG
static void show_args(list<string> &files,
		      int ignore_timesteps_after,
		      const char **part_combos,
		      int ncombos);
#endif

//---------------------------------------------------------------------------
// process_brainfunction
//---------------------------------------------------------------------------
int process_brainfunction(int argc, char *argv[])
{
	part_names['A'] = "All";
	part_names['P'] = "Processing";
	part_names['I'] = "Input";
	part_names['B'] = "Behavior";
	part_names['H'] = "Health";

	// ---
	// --- Process Command-Line Args
	// ---
	bool bare = false;

	// --- get files to process
	if(argc < 2 )
	{
		show_usage("Must specify brain function file.");
	}

	int argi = 1;
	string arg = argv[argi];

	if(arg == "--bare")
	{
		bare = true;
		consume_arg(argc, argv, argi);
		arg = argv[argi];
	}

	list<string> files;

	if(arg == "--list")
	{
		bool eol = false;

		consume_arg(argc, argv, argi);

		while(argc > 1 && !eol)
		{
			arg = argv[argi];
			
			if(arg == "--")
			{
				eol = true;
			}
			else
			{
				files.push_back(arg);
			}

			consume_arg(argc, argv, argi);
		}

		if(!eol)
		{
			show_usage("expecting '--' at end of file list");
		}

		if(files.size() == 0)
		{
			show_usage("Must specify brain function file in list.");
		}
	}
	else
	{
		files.push_back(arg);

		consume_arg(argc, argv, argi);
	}


	// --- get optional timestep
	int ignore_timesteps_after = 0;	// by default, calculate complexity over the agent's entire lifetime.

	if(isdigit(argv[argi][0]))
	{
		ignore_timesteps_after = atoi( argv[argi] );

		if( ignore_timesteps_after < WARN_IF_COMPUTING_COMPLEXITY_OVER_LESSTHAN_N_TIMESTEPS )
		{
			cerr << "* Warning: Computing Complexity with less than ";
			cerr << WARN_IF_COMPUTING_COMPLEXITY_OVER_LESSTHAN_N_TIMESTEPS << " time samples." << endl;
		}

		consume_arg(argc, argv, argi);
	}

	// --- get part combos
	const char **part_combos;
	int ncombos;

	if( argc == 1 )
	{
		part_combos = DEFAULT_PART_COMBOS;
		ncombos = sizeof(DEFAULT_PART_COMBOS) / sizeof(const char *);
	}
	else
	{

		for(int i = argi ; i < argc; i++)
		{
			for(char *c = argv[i]; *c != 0; c++)
			{
				char C = toupper(*c);

				if(part_names.find(C) == part_names.end())
				{
					cout << "* Error: Didnt know letter '" << C  << "' that you specified.  Exiting." << endl;
					exit(1);
				}

				*c = C;
			}
		}

		ncombos = argc - argi;
		part_combos = (const char **)argv + 1; // skip program name in arg 0
	
		assert( ignore_timesteps_after >= 0 );
		if( ignore_timesteps_after < WARN_IF_COMPUTING_COMPLEXITY_OVER_LESSTHAN_N_TIMESTEPS )
		{
			cerr << "* Warning: Computing Complexity with less than ";
			cerr << WARN_IF_COMPUTING_COMPLEXITY_OVER_LESSTHAN_N_TIMESTEPS << " time samples." << endl;
		}
	}

	DEBUG_STMT(show_args(files, ignore_timesteps_after, part_combos, ncombos));

	// ---
	// --- Perform Complexity Calculations
	// ---

	// --- construct parms
	int nfiles = files.size();	
	int nparms = nfiles * ncombos;
	CalcComplexity_brainfunction_parms *parms = new CalcComplexity_brainfunction_parms[nparms];

	CalcComplexity_brainfunction_parms *_parms = parms;
	for(list<string>::iterator
	      it = files.begin(),
	      it_end = files.end();
	    it != it_end;
	    it++)
	{
		const char *path = it->c_str();

		for(int icombo = 0;
		    icombo < ncombos;
		    icombo++,
		    _parms++)
		{
			_parms->path = path;
			_parms->parts = part_combos[icombo];
			_parms->ignore_timesteps_after = ignore_timesteps_after;
		}
	}

	Callback_bf callback(bare,
			  part_combos,
			  ncombos);

	// --- perform calculations
	CalcComplexity_brainfunction_result *result = CalcComplexity_brainfunction(parms,
										   nparms,
										   &callback);
	delete result;

	return 0;
}


#if DEBUG
//---------------------------------------------------------------------------
// show_args
//---------------------------------------------------------------------------
void show_args(list<string> &files,
	       int ignore_timesteps_after,
	       const char **part_combos,
	       int ncombos)
{
	cout << "files:" << endl;
	for(list<string>::iterator
	      it = files.begin(),
	      it_end = files.end();
	    it != it_end;
	    it++)
	{
		cout << "  " << *it << endl;
	}

	cout << "ignore_timesteps_after: " << ignore_timesteps_after << endl;

	cout << "part combos:" << endl;
	for(int i = 0; i < ncombos; i++)
	{
		cout << "  " << part_combos[i] << endl;
	}

}
#endif


//===========================================================================
// Callback_bf
//===========================================================================


//---------------------------------------------------------------------------
// Callback_bf::begin
//---------------------------------------------------------------------------
void Callback_bf::begin(CalcComplexity_brainfunction_parms *parms,
		     int nparms)
{
	reported = new bool[nparms];
	for(int i = 0; i < nparms; i++)
	{
		reported[i] = false;
	}

	if(!bare)
	{
		printf("#AGENT-NUMBER LIFESPAN NUM-NEURONS");

		for(int i = 0; i < nparts_combos; i++)
		  {
		    printf("%10s ", parts_combos[i]);
		  }

		printf("PATH");
		printf("\n");
		fflush(stdout);
	}

	last_displayed = -1;
}

//---------------------------------------------------------------------------
// Callback_bf::parms_result
//---------------------------------------------------------------------------
void Callback_bf::parms_result(CalcComplexity_brainfunction_result *result,
			    int parms_index)
 	{
#pragma omp critical(complexity_brainfunction__parms_result)
	{
		reported[parms_index] = true;

		if(last_displayed == parms_index - 1)
		{
			display(result);
		}
	}

}

//---------------------------------------------------------------------------
// Callback_bf::display
//---------------------------------------------------------------------------
void Callback_bf::display(CalcComplexity_brainfunction_result *result) {
	CalcComplexity_brainfunction_parms *parms_all = result->parms;

	for(int parms_index = last_displayed + 1;
	    parms_index < result->nparms
	      && reported[parms_index];
	    parms_index++)
	{
		CalcComplexity_brainfunction_parms *parms = parms_all + parms_index;
		int icombo = parms_index % nparts_combos;

		if(icombo == 0)
		{
			printf("%15ld", result->agent_number[parms_index]);

			if(!bare)
			{
				printf("%9ld", result->lifespan[parms_index]);
				printf("%12ld", result->num_neurons[parms_index]);
			}
		}

		printf("%10f ", result->complexity[parms_index]);

		if(icombo == nparts_combos - 1)
		{
			if(!bare)
			{
				printf("%20s ", parms->path);
			}

			printf("\n");
		}

		last_displayed++;
	}

	fflush(stdout);

}

//---------------------------------------------------------------------------
// Callback_bf::end
//---------------------------------------------------------------------------
void Callback_bf::end(CalcComplexity_brainfunction_result *result)
{
	delete[] reported;
	reported = NULL;
}

// eof
