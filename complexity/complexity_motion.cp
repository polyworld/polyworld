#define COMPLEXITY_MOTION_CP

#include "complexity_motion.h"

#include <assert.h>
#include <stdlib.h>

#include "complexity_algorithm.h"
#include "PositionReader.h"

#include <list>

using namespace std;

#define NIL_RAND 1
#define NIL_ZERO 2
#define NIL_TYPE NIL_ZERO

#define NDIMS 2

#define DEBUG_MOTION_COMPLEXITY 0

#if DEBUG_MOTION_COMPLEXITY
	#define DEBUG(STMT) STMT
#else
	#define DEBUG(STMT)
#endif

static gsl_matrix *populate_matrix(PopulateMatrixContext &context,
				   long epoch,
				   long step_begin,
				   long step_end);
static float get_epoch_presence(PositionQuery &query);

#if DEBUG_MOTION_COMPLEXITY
	static void show_positions(PositionQuery &query);
#endif


//---------------------------------------------------------------------------
// CalcComplexity_motion
//---------------------------------------------------------------------------
CalcComplexity_motion_result *CalcComplexity_motion(CalcComplexity_motion_parms &parms,
						    CalcComplexity_motion_callback *callback)
{
	CalcComplexity_motion_result *result = new CalcComplexity_motion_result();
	result->parms_requested = result->parms = parms;
	result->ndimensions = NDIMS; 

#define MFREE(MATRIX) gsl_matrix_free(MATRIX); MATRIX = NULL;

	PopulateMatrixContext context(result);

	if(parms.step_begin == 0) parms.step_begin = 1;

	long sim_end = context.position_reader.getPopulationHistory()->step_end;
	if(parms.step_end == 0 || parms.step_end > sim_end) parms.step_end = sim_end;
	
	// round to epoch boundary
	parms.step_end = parms.step_begin + (((parms.step_end - parms.step_begin + 1) / parms.epoch_len) * parms.epoch_len) - 1;

	result->parms = parms;

	if(callback)
	{
		callback->begin(parms);
	}

	context.computeEpochs();

	int nepochs = context.nepochs;

	result->epochs.count = nepochs;
	result->epochs.complexity = new double[nepochs];
	result->epochs.agent_count = new int[nepochs];
	result->epochs.agent_count_calculated = new int[nepochs];
	result->epochs.nil_count = new int[nepochs];

#pragma omp parallel for
	for(long iepoch = 0;
	    iepoch < nepochs;
	    iepoch++)
	{
		long step = parms.step_begin + (parms.epoch_len * iepoch);
		gsl_matrix *matrix_pos = populate_matrix(context,
							 iepoch,
							 step,
							 step + parms.epoch_len - 1);

		double Complexity;

		if(matrix_pos == NULL)
		{
			Complexity = 0;
		}
		else
		{
			DEBUG(print_matrix(matrix_pos));
			gsl_matrix *COV = mCOV(matrix_pos);
			MFREE(matrix_pos);

			DEBUG(print_matrix(COV));

			Complexity = calcC_det3__optimized( COV );
			MFREE(COV);
		}

		result->epochs.complexity[iepoch] = Complexity;

		if(callback)
		{
			callback->epoch_result(result,
					       iepoch);
		}
	}

	if(callback)
	{
		callback->end(result);
	}

	return result;

#undef MFREE
}

//---------------------------------------------------------------------------
// populate_matrix
//---------------------------------------------------------------------------
gsl_matrix *populate_matrix(PopulateMatrixContext &context,
			    long iepoch,
			    long step_begin,
			    long step_end)
{
#if NIL_TYPE == NIL_RAND
	srand(step_begin);
#endif
	PopulateMatrixContext::Epoch &epoch = context.epochs[iepoch];
	int nagents = epoch.entries.size();
	int nsteps = step_end - step_begin + 1;

	DEBUG(printf("nagents=%d, nsteps=%d\n", nagents, nsteps));

	int nrows = nagents * NDIMS;
	int ncols = nsteps;
	assert(nrows > 0 && ncols > 0);

	gsl_matrix *matrix_pos = gsl_matrix_alloc(nrows,
						  ncols);

	int row_x = 0;
	int row_z = 1;

	PositionQuery query;

	query.steps_epoch.begin = step_begin;
	query.steps_epoch.end = step_end;

	int &nil_count = context.result->epochs.nil_count[iepoch];
	nil_count = 0;

	for(PopulateMatrixContext::EntryList::iterator
	      it = epoch.entries.begin(),
	      it_end = epoch.entries.end();
	    it != it_end;
	    it++)
	{
		query.agent_number = it->first;

#pragma omp critical(complexity_motion__query_positions)
		{
		  context.position_reader.getPositions(&query);
		}
		DEBUG(show_positions(query));

		if(get_epoch_presence(query) < context.result->parms.min_epoch_presence)
		{
			DEBUG(printf("agent %ld below presence threshold\n", query.agent_number));
			continue;
		}

		int col = 0;

		for(long step = query.steps_epoch.begin;
		    step <= query.steps_epoch.end;
		    step++, col++)
		{
			PositionRecord *pos = query.get(step);

			float x;
			float z;

			if(pos)
			{
				assert(pos->step == step);
				
				x = pos->x;
				z = pos->z;
			}
			else
			{
				nil_count++;
#if NIL_TYPE == NIL_ZERO
				x = 0;
				z = 0;
#elif NIL_TYPE == NIL_RAND
				x = randpw() * 10;
				z = randpw() * 10;
#endif
			}

			gsl_matrix_set(matrix_pos,
				       row_x,
				       col,
				       x);
			gsl_matrix_set(matrix_pos,
				       row_z,
				       col,
				       z);
		}

		row_x += 2;
		row_z += 2;
	}

	if(row_x == 0)
	{
		gsl_matrix_free(matrix_pos);
		matrix_pos = NULL;
	}
	else
	{
		if(row_x != nrows)
		{
			// some agents didn't pass the MIN_EPOCH_PRESENCE threshhold
			// shrink the number of rows.

	  		DEBUG(printf("shrinking matrix from %ld to %ld rows.\n", nrows, row_x));

			matrix_pos->size1 = row_x;
		}
	}

	context.result->epochs.agent_count[iepoch] = nagents;
	context.result->epochs.agent_count_calculated[iepoch] = row_x / 2;

	return matrix_pos;
}

//---------------------------------------------------------------------------
// get_epoch_presence
//---------------------------------------------------------------------------
float get_epoch_presence(PositionQuery &query)
{
	float presence =
	  float(query.steps_agent.end - query.steps_agent.begin)
	  / float(query.steps_epoch.end - query.steps_epoch.begin);

	return presence;
}

#if DEBUG_MOTION_COMPLEXITY
//---------------------------------------------------------------------------
// show_positions
//---------------------------------------------------------------------------
void show_positions(PositionQuery &query)
{
	printf("agent_number=%ld\n",
	       query.agent_number);

	float presence = get_epoch_presence(query);

	printf("  epoch=[%ld,%ld], agent=[%ld,%ld], presence=%f\n",
	       query.steps_epoch.begin, query.steps_epoch.end,
	       query.steps_agent.begin, query.steps_agent.end,
	       presence);


	for(long step = query.steps_epoch.begin;
	    step <= query.steps_epoch.end;
	    step++)
	{
		PositionRecord *pos = query.get(step);

		if(pos)
		{
			printf("    step %ld = {%ld, (%f,%f,%f)}\n",
			       step,
			       pos->step, pos->x, pos->y, pos->z);
		}
		else
		{
			printf("    step %ld = NIL\n",
			       step);
		}
	}
}
#endif


//===========================================================================
// PopulateMatrixContext
//===========================================================================

//---------------------------------------------------------------------------
// PopulateMatrixContext::PopulateMatrixContext
//---------------------------------------------------------------------------
PopulateMatrixContext::PopulateMatrixContext(CalcComplexity_motion_result *result)
  : position_reader(result->parms.path)
{
	this->result = result;
	this->step = 0;
	this->epochs = NULL;
	this->nepochs = 0;
}

//---------------------------------------------------------------------------
// PopulateMatrixContext::~PopulateMatrixContext
//---------------------------------------------------------------------------
PopulateMatrixContext::~PopulateMatrixContext()
{
	if(nepochs) delete[] epochs;
}

//---------------------------------------------------------------------------
// PopulateMatrixContext::computeEpochs
//---------------------------------------------------------------------------
void PopulateMatrixContext::computeEpochs()
{
	assert(nepochs == 0);

	CalcComplexity_motion_parms &parms = result->parms;

	nepochs = (parms.step_end - parms.step_begin + 1) / parms.epoch_len;
	epochs = new Epoch[nepochs];

	long epoch_len = parms.epoch_len;
	long begin = parms.step_begin;
	long end = parms.step_end;

#define EPOCH(STEP) ((STEP - begin) / epoch_len)

	long epoch_begin = EPOCH(begin);
	long epoch_end = EPOCH(end);

	const PopulationHistory *ph = position_reader.getPopulationHistory();

	for(PopulationHistory::AgentNumberMap::const_iterator
	      it_crit = ph->numberLookup.begin(),
	      it_crit_end = ph->numberLookup.end();
	    it_crit != it_crit_end;
	    it_crit++)
	{
		const PopulationHistoryEntry *entry = it_crit->second;

		DEBUG(printf("crit %ld --> [%ld,%ld]\n", entry->agent_number, entry->step_begin, entry->step_end));

		long entry_begin = entry->step_begin;
		long entry_end = entry->step_end;

		if((entry_begin >= begin && entry_begin <= end)
		   || (entry_end >= begin && entry_end <= end))
		{
			long epoch_begin_entry = max(epoch_begin,
						     EPOCH(entry_begin));
			long epoch_end_entry = min(epoch_end,
						   EPOCH(entry_end));

			for(long iepoch = epoch_begin_entry;
			    iepoch <= epoch_end_entry;
			    iepoch++)
			{
			  int idx = iepoch - epoch_begin;
			  assert(idx < nepochs);
				Epoch *epoch = &epochs[iepoch - epoch_begin];

				epoch->entries[entry->agent_number] = entry;
			}
		}
	}

#undef EPOCH

}

// eof
