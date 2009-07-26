#define MOTION_CPP

#include <errno.h>
#include <string.h>

#include <iostream>
#include <string>

#include "motion.h"

#include "main.h"
#include "PositionPath.h"

using namespace std;


//---------------------------------------------------------------------------
// process_motion
//---------------------------------------------------------------------------
int process_motion(int argc, char *argv[])
{
	// ---
	// --- Process Command-Line Args
	// ---

	bool bare = false;
	bool run_dir = true;

	CalcComplexity_motion_parms parms;
	parms.step_begin = 0;
	parms.step_end = 0;
	parms.epoch_len = DEFAULT_EPOCH;
	parms.min_epoch_presence = DEFAULT_THRESHOLD;

	for(int i = 1; i < argc; )
	{
		string arg = argv[i];

		if(0 == arg.compare(0, 2, "--"))
		{
#define opt_arg(NAME,TYPE,VAR) (option == NAME) {VAR = get_##TYPE##_option(option, argc, argv, i);}
#define opt(NAME,STMT) (option == NAME) {STMT; consume_arg(argc, argv, i);}

			string option = arg.substr(2);

			if opt_arg(     "first",        long,  parms.step_begin)
			else if opt_arg("last",         long,  parms.step_end)
			else if opt_arg("epoch",        long,  parms.epoch_len)
			else if opt_arg("threshold",    float, parms.min_epoch_presence)
			else if opt(    "bare",         bare = true)
			else if opt(    "position-dir", run_dir = false)
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

	if(parms.min_epoch_presence > 1 || parms.min_epoch_presence < 0)
	{
		show_usage("threshold must be >= 0 and <= 1.");
	}

	parms.path = argv[1];

	if(run_dir)
	{
		parms.path = strdup(PositionPath::position_dir(parms.path).c_str());
	}
	else
	{
		parms.path = strdup(parms.path);
	}

	// ---
	// --- Perform Complexity Calculations
	// ---

	if(!bare)
	{
		printf("# path = %s\n", parms.path);
		fflush(stdout);
	}

	Callback callback(bare);

	CalcComplexity_motion_result *result = CalcComplexity_motion(parms,
								     &callback);
	free((void *)parms.path);
	delete result;

	return 0;
}

//===========================================================================
// Callback
//===========================================================================

//---------------------------------------------------------------------------
// Callback::begin
//---------------------------------------------------------------------------
void Callback::begin(CalcComplexity_motion_parms &parms)
{
	if(!bare)
	{
		printf("# first step = %ld\n", parms.step_begin);
		printf("# last step = %ld\n", parms.step_end);
		printf("# epoch length = %ld\n", parms.epoch_len);
		printf("# threshold = %f\n", parms.min_epoch_presence);
		printf("#\n");
		printf("#{ EPOCH-START EPOCH-END NIL-RATIO NAGENTS NAGENTS-CALCULATED COMPLEXITY\n");
		fflush(stdout);
	}
}

//---------------------------------------------------------------------------
// Callback::epoch_result
//---------------------------------------------------------------------------
void Callback::epoch_result(CalcComplexity_motion_result *result,
			   int epoch_index)
{
#pragma omp critical(complexity_motion__epoch_result)
	{
		if(reported == NULL)
		{
			reported = new bool[result->epochs.count];
			for(int i = 0; i < result->epochs.count; i++)
			{
				reported[i] = false;
			}

			last_displayed = -1;
		}

		reported[epoch_index] = true;

		if(last_displayed == epoch_index - 1)
		{
			display(result);
		}
	}

}

//---------------------------------------------------------------------------
// Callback::end
//---------------------------------------------------------------------------
void Callback::end(CalcComplexity_motion_result *result)
{
	if(!bare)
	{
		printf("#} TOTAL-NIL-RATIO\n");

		printf("%18f", float(total_nil) / total_elements);
		printf("\n");
		fflush(stdout);
	}

	if(reported)
	{
		delete[] reported;
		reported = NULL;
	}
}

//---------------------------------------------------------------------------
// Callback::display
//---------------------------------------------------------------------------
void Callback::display(CalcComplexity_motion_result *result)
{
	CalcComplexity_motion_parms &parms = result->parms;

	for(int epoch_index = last_displayed + 1;
	    epoch_index < result->epochs.count
	      && reported[epoch_index];
	    epoch_index++)
	{
		long start = parms.step_begin + (epoch_index * parms.epoch_len);
		long end = start + parms.epoch_len - 1;
		long nagents_calculated = result->epochs.agent_count_calculated[epoch_index];
		long nelements = nagents_calculated * result->ndimensions * parms.epoch_len;
		long nil_count = result->epochs.nil_count[epoch_index];
		float nil_ratio = float(nil_count) / nelements;
	
		total_elements += nelements;
		total_nil += nil_count;
	
		printf("%14ld ", start);
		printf("%9ld ", end);
		printf("%9f ", nil_ratio);
		printf("%9d ", result->epochs.agent_count[epoch_index]);
		printf("%20ld ", nagents_calculated);
	
		printf("%10f", result->epochs.complexity[epoch_index]);
		printf("\n");
		fflush(stdout);

		last_displayed++;
	}
}

// eof
