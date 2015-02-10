#define MOTION_CPP

#include <errno.h>
#include <string.h>

#include <iostream>
#include <string>

#include "motion.h"

#include "complexity_motion.h"
#include "datalib.h"
#include "main.h"
#include "misc.h"

using namespace std;


//---------------------------------------------------------------------------
// usage_motion
//---------------------------------------------------------------------------
void usage_motion()
{
	cerr << "CalcComplexity motion [option]... run_dir [option]..." << endl;
	cerr << endl;
	cerr << "options:" << endl;
	cerr << "  --first <step>  : first step as integer. (default = 1)" << endl;
	cerr << "  --last <step>   : last step as integer. (default = final step in sim data)" << endl;
	cerr << "  --epoch <n>     : number of steps in epoch as integer. (default = " << DEFAULT_EPOCH << ")" << endl;
	cerr << "  --threshold <f> : fraction of epoch agent must be present in order to be" << endl;
	cerr << "                       part of the calculation. (default = " << DEFAULT_THRESHOLD << ")" << endl;
}

//---------------------------------------------------------------------------
// process_motion
//---------------------------------------------------------------------------
int process_motion(int argc, char *argv[])
{
	// ---
	// --- Process Command-Line Args
	// ---

	long step_begin = 0;
	long step_end = 0;
	long epoch_len = DEFAULT_EPOCH;
	float min_epoch_presence = DEFAULT_THRESHOLD;
	const char *path_run;

	for(int i = 1; i < argc; )
	{
		string arg = argv[i];

		if(0 == arg.compare(0, 2, "--"))
		{
#define opt_arg(NAME,TYPE,VAR) (option == NAME) {VAR = get_##TYPE##_option(option, argc, argv, i);}
#define opt(NAME,STMT) (option == NAME) {STMT; consume_arg(argc, argv, i);}

			string option = arg.substr(2);

			if opt_arg(     "first",        long,  step_begin)
			else if opt_arg("last",         long,  step_end)
			else if opt_arg("epoch",        long,  epoch_len)
			else if opt_arg("threshold",    float, min_epoch_presence)
			else
			{
				show_usage(string("Invalid option: ") + option);
			}
#undef opt_arg
#undef opt
		}
		else
		{
			i++;
		}
	}

	if(argc == 1)
	{
		show_usage("Must specify directory.");
	}
	else if(argc > 2)
	{
		show_usage(string("Unexpected arg: ") + argv[2]);
	}

	if(min_epoch_presence > 1 || min_epoch_presence < 0)
	{
		show_usage("threshold must be >= 0 and <= 1.");
	}

	path_run = argv[1];

	// ---
	// --- Perform Complexity Calculations
	// ---
	MotionComplexity mc( path_run, step_begin, step_end, epoch_len, min_epoch_presence );

	int nepochs = mc.epochs.size();
	int ncalculated = 0;

#pragma omp parallel for shared( ncalculated )
	for( int i = 0; i < nepochs; i++ )
	{
		MotionComplexity::Epoch &epoch = mc.epochs[i];

		mc.calculate( epoch );

#pragma omp critical( ncalculated )
		{
			ncalculated++;

			printf( "\r%d%%", int(100 * (float(ncalculated) / nepochs)) );
			fflush( stdout );
		}
	}

	printf( "\n" );

	// ---
	// --- Save Results
	// ---
	char path_out[512];

	sprintf( path_out,
			 "%s/motion/Complexity.txt",
			 path_run );

	DataLibWriter out( path_out );

	const char *colnames[] =
		{
			"EPOCH-START",
			"EPOCH-END",
			"NIL-RATIO",
			"NAGENTS",
			"NAGENTS-CALCULATED",
			"COMPLEXITY",
			NULL
		};
	datalib::Type coltypes[] =
		{
			datalib::INT,
			datalib::INT,
			datalib::FLOAT,
			datalib::INT,
			datalib::INT,
			datalib::FLOAT
		};
	
	out.beginTable( "Olaf",
					colnames,
					coltypes );

	itfor( MotionComplexity::EpochList, mc.epochs, it )
	{
		out.addRow( it->begin,
					it->end,
					it->complexity.nil_ratio,
					it->agents.size(),
					it->complexity.nagents,
					it->complexity.value );
	}

	out.endTable();

	return 0;
}
