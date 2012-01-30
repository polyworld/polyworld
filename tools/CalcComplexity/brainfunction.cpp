#define BRAINFUNCTION_CPP

#include <ctype.h>		// for isdigit()
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include <list>
#include <map>
#include <string>
#include <iostream>
#include <fstream>

#include "brainfunction.h"
#include "Events.h"

#include "main.h"

using namespace std;

map<char, string> part_names;
int MaxLifeSpan = 1000;

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

Events* parse_events( char* filter_events, string brain_function_path );
void find_filter_files( string brain_function_path,	// input
						string& worldfile_path,  	// outputs...
						string& births_deaths_path,
						string& energy_log_path );
long get_max_steps( ifstream& worldfile );
void parse_mate_events( ifstream& births_deaths, Events* events );
void parse_eat_events( ifstream& energy_log, Events* events );


//---------------------------------------------------------------------------
// usage_brainfunction
//---------------------------------------------------------------------------
void usage_brainfunction()
{
	cerr << "CalcComplexity brainfunction [--bare] (<func_file> | --list <func_file>... --) [N] [[APIBH]+[me]*\\d*]..." << endl;
	cerr << "\t--bare :  If set will CalcComplexity will output bare numerical values, with no labels.\n\t\tUsed by CalcComplexity.py, but normally not used from the command line." << endl;
	cerr << "\t<func_file> | --list <func_file>... -- :  The brainFunction file to compute complexity for.\n\t\tIf --list is used, provide a list of files followed by '--'.\n\t\tBoth complete and incomplete brainFunction files are supported." << endl;
	cerr << "\tN :  Optional length of the agent's life (in timesteps) over which Complexity is to be computed.\n\t\tEx: a value of 100 will compute Complexity across the first 100 steps of the agent's life." << endl; 
	cerr << "\t[[APIBH]+[me]*\\d*]... :  Optional space-separated list of complexity types to calculate.\n\t\tThis can be (uppercase only) 'A', 'P', 'I', 'B', 'H' or any meaningful combination thereof.\n\t\tIt specifies whether you want to compute the Complexity of All, Processing, Input, Behavior,\n\t\tor Health+Behavior neurons. By default it computes the Complexity for A, P, I, B, and HB.\n\t\tAny of the complexity types may have one or more lowercase letters appendeded to indicate \n\t\tthat neural activity should be filtered based on behavioral events prior to the\n\t\tcalculation of Complexity. Currently acceptable values are 'm'ate and 'e'at. Warning:\n\t\tFilter order uniquely identifies datalib entries, but doesn't alter what is calculated.\n\t\tAny of the complexity types may have one or more digits appended to specify the number of\n\t\tpoints to use in integrating the area between the (k/N)I(X) and <I(X_k)> curves. If not\n\t\tspecified, the default is effectively 1 (one), which yields the traditional 'simplified\n\t\tTSE complexity'. A value of 0 (zero) will use all points (all values of k) thus yielding\n\t\tfull TSE complexity (though <I(X_k)> will be approximated for large values of N_choose_k)." << endl;
}


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

	// --- default values
	bool bare = false;
	int ignore_timesteps_after = 0;	// calculate complexity over the agent's entire lifetime.
	char* filter_events = NULL;
	int num_filter_events = 0;
	// Note: size_filter_events allows room for 7 characters plus terminator (only 2 needed)
	#define size_filter_events 8
	Events* events = NULL;

	// ---
	// --- Process Command-Line Args
	// ---

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
	
	// --- get optional max number of timesteps to consider

	if( argc > 1 && isdigit(argv[argi][0]) )
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
		ncombos = sizeof( DEFAULT_PART_COMBOS ) / sizeof(const char *);
	}
	else
	{
		for( int i = argi ; i < argc; i++ )
		{
			for( char *c = argv[i]; *c != 0; c++ )
			{
				// lowercase letters indicate event filters
				if( islower( *c ) )
				{
					if( ! filter_events )
						filter_events = (char*) calloc( size_filter_events, 1 );
					bool already_there = false;
					for( int i = 0; i < num_filter_events; i++ )
					{
						if( filter_events[i] == *c )
						{
							already_there = true;
							break;
						}
					}
					if( ! already_there )
					{
						filter_events[num_filter_events] = *c;
						num_filter_events++;
						if( num_filter_events >= size_filter_events )
						{
							cerr << "Error: too many filter events specified" << endl;
							exit( 1 );
						}
					}
					continue;
				}
				
				// digits indicate integration precision
				if( isdigit(*c) )
					continue;
				
				if( part_names.find( *c ) == part_names.end() )
				{
					cerr << "Error: Didnt know letter '" << *c << "' that you specified.  Exiting." << endl;
					exit( 1 );
				}
			}
		}
				
		if( filter_events )
			filter_events[num_filter_events] = '\0'; 	// add c string terminator

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

	// --- build the events list, if needed
	if( filter_events )
		events = parse_events( filter_events, *(files.begin()) );

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
			_parms->events = events;
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
	
	if( filter_events )
		free( filter_events );

	return 0;
}


//---------------------------------------------------------------------------
// parse_events
//---------------------------------------------------------------------------
Events* parse_events( char* filter_events, string brain_function_path )
{
	string worldfile_path;
	string births_deaths_path;
	string energy_log_path;
	
	// Find the normalized.wf worldfile, BirthsDeaths.log, and
	// events/energy.log relative to the provided brainFunction file.
	// TODO:  If we only find a legacy worldfile, generate normalized.wf.
	find_filter_files( brain_function_path,	// input
					   worldfile_path, births_deaths_path, energy_log_path );  // outputs
	
	// Try to open the files
	ifstream worldfile( worldfile_path.c_str() );
	if( ! worldfile.is_open() )
	{
		cerr << "Unable to open file: " << worldfile_path << endl;
		exit( 1 );
	}
	ifstream births_deaths( births_deaths_path.c_str() );
	if( ! births_deaths.is_open() )
	{
		cerr << "Unable to open file: " << births_deaths_path << endl;
		exit( 1 );
	}
	ifstream energy_log( energy_log_path.c_str() );
	if( ! energy_log.is_open() )
	{
		cerr << "Unable to open file: " << energy_log_path << endl;
		exit( 1 );
	}

	// Determine maxSteps from the worldfile
	long maxSteps = get_max_steps( worldfile );
	
	Events* events = new Events( maxSteps );
	
	// Parse BirthsDeaths.log to add mate events
	if( strchr( filter_events, 'm' ) )
		parse_mate_events( births_deaths, events );
	
	// Parse events/energy.log to add eat events
	if( strchr( filter_events, 'e' ) )
		parse_eat_events( energy_log, events );
	
	// Close the files we opened
	worldfile.close();
	births_deaths.close();
	energy_log.close();
	
	return( events );
}


//---------------------------------------------------------------------------
// find_filter_files
//---------------------------------------------------------------------------
void find_filter_files( string brain_function_path,	// input
						string& worldfile_path, string& births_deaths_path, string& energy_log_path )  // outputs
{
	size_t found;
	string root;
	
	// Try to determine the directory layout
	found = brain_function_path.find( "Recent" );
	bool run;
	if( found != string::npos )
	{
		// "Recent" is in the file path, so assume path is
		// .../<run>/brain/Recent/<#>/<brainFunction_file>
		// and worldfile is in .../<run>/
		root = brain_function_path.substr( 0, found-6 );
		run = true;
	}
	else
	{
		// "Recent" is not in the file path, so assume worldfile is
		// in the same directory as the brainFunction file
		found = brain_function_path.rfind( "/" );
		if( found != string::npos )	// keep everything through the final slash
			root = brain_function_path.substr( 0, found+1 );
		else  // no slash in name, so use current working directory
			root = "";
		run = false;
	}

	// Make our best guess about file paths
	worldfile_path = root + "normalized.wf";
	births_deaths_path = root + "BirthsDeaths.log";
	if( run )
		energy_log_path = root + "events/energy.log";
	else
		energy_log_path = root + "energy.log";
}


//---------------------------------------------------------------------------
// get_max_steps
//---------------------------------------------------------------------------
long get_max_steps( ifstream& worldfile )
{
	long maxSteps = 0;
	string line;
	
	// parse the worldfile to extract maxSteps
	while( worldfile.good() )
	{
		getline( worldfile, line );
		if( line.substr(0,8) == "MaxSteps" )
		{
			maxSteps = atol( line.substr(9,string::npos).c_str() );
			break;
		}
	}
	
	return( maxSteps );
}


//---------------------------------------------------------------------------
// parse_mate_events
//---------------------------------------------------------------------------
void parse_mate_events( ifstream& births_deaths, Events* events )
{
	string line;
	getline( births_deaths, line );	// read first line out of the way
	while( births_deaths.good() )
	{
		getline( births_deaths, line );
		vector<string> parts = split( line );
		// If it was a normal BIRTH or VIRTUAL birth event, add a 'm'ate event for the parents
		if( parts.size() > 1 && (parts[1].c_str()[0] == 'B' || parts[1].c_str()[0] == 'V' ) )
		{
			long step = atol( parts[0].c_str() );
			long parent1 = atol( parts[3].c_str() );
			long parent2 = atol( parts[4].c_str() );
			events->AddEvent( step, parent1, 'm' );
			events->AddEvent( step, parent2, 'm' );
		}
	}
}


//---------------------------------------------------------------------------
// parse_eat_events
//---------------------------------------------------------------------------
void parse_eat_events( ifstream& energy_log, Events* events )
{
	string line;
	
	while( energy_log.good() )
	{
		getline( energy_log, line );
		if( line.size() > 0 && isdigit( line.c_str()[0] ) )
		{
			vector<string> parts = split( line, "\t " );
			if( parts[2] == "E" )
			{
				long step = atol( parts[0].c_str() );
				long agent = atol( parts[1].c_str() );
				events->AddEvent( step, agent, 'e' );
			}
		}
	}
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
		printf("#  AGENT-NUMBER LIFESPAN NUM-NEURONS");

		for(int i = 0; i < nparts_combos; i++)
		  {
		    printf("%7s    ", parts_combos[i]);
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
