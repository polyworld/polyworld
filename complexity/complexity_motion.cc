#include "complexity_motion.h"

#include <assert.h>

#include <map>
#include <string>

#include "datalib.h"
#include "misc.h"


using namespace std;

#define NIL_RAND 1
#define NIL_ZERO 2
#define NIL_TYPE NIL_ZERO

#define NDIMS 2

#define DEBUG_MOTION_COMPLEXITY false

#if DEBUG_MOTION_COMPLEXITY
	#define DEBUG(STMT) STMT
#else
	#define DEBUG(STMT)
#endif


// -----------------------------------------------------------------------------
// ---
// --- ctor()
// ---
// -----------------------------------------------------------------------------
MotionComplexity::MotionComplexity( const char *path_run,
									long step_begin,
									long step_end,
									long epochlen,
									float min_epoch_presence )
{
	this->path_run = path_run;
	this->min_epoch_presence = min_epoch_presence;

	computeEpochs( step_begin,
				   step_end,
				   epochlen );
}

// -----------------------------------------------------------------------------
// ---
// --- dtor()
// ---
// -----------------------------------------------------------------------------
MotionComplexity::~MotionComplexity()
{
}

// -----------------------------------------------------------------------------
// ---
// --- calculate()
// ---
// -----------------------------------------------------------------------------
float MotionComplexity::calculate( Epoch &epoch )
{
	gsl_matrix *matrix_pos = populateMatrix( epoch );

	if( matrix_pos == NULL )
	{
		epoch.complexity.value = 0;
	}
	else
	{
#define MFREE(MATRIX) gsl_matrix_free(MATRIX); MATRIX = NULL;

		DEBUG( print_matrix(matrix_pos) );
		gsl_matrix *COV = calcCOV( matrix_pos );
		MFREE(matrix_pos);

		DEBUG( print_matrix(COV) );

		double det = determinant( COV );
		double I_n = CalcI( COV, det );
		epoch.complexity.value = calcC_k_exact( COV, I_n, COV->size1 - 1 );
		
		MFREE(COV);
	}

	return epoch.complexity.value;
}

// -----------------------------------------------------------------------------
// ---
// --- computeEpochs()
// ---
// -----------------------------------------------------------------------------
void MotionComplexity::computeEpochs( long begin,
									  long end,
									  long epochlen )
{
	DataLibReader in( (string(path_run) + "/lifespans.txt").c_str() );
	in.seekTable( "LifeSpans" );

	// ---
	// --- Determine actual time constraints
	// ---
	if( begin <= 0 )
	{
		begin = 1;
	}

	in.seekRow( -1 );
	long sim_end = in.col( "DeathStep" );
	
	if( end <= 0 )
	{
		end = sim_end;
	}
	else
	{
		end = min( end, sim_end );
	}

	// round to epoch boundary
	end = begin + (((end - begin + 1) / epochlen) * epochlen) - 1;

	assert( end > begin );
	
	// ---
	// --- Alloc/Init Epochs
	// ---
	for( long step = begin;
		 step <= end;
		 step += epochlen )
	{
		Epoch epoch;

		epoch.index = epochs.size();
		epoch.begin = step;
		epoch.end = step + epochlen - 1;

		epoch.complexity.value = 0;
		epoch.complexity.nagents = 0;
		epoch.complexity.nil_ratio = 0;

		epochs.push_back( epoch );
	}

	// ---
	// --- Parse Agents
	// ---
	in.rewindTable();

	// read into a map so we sort by agent number
	typedef map<long, Agent> AgentMap;
	
	AgentMap agents;

	while( in.nextRow() )
	{
		Agent a;

		a.number = in.col( "Agent" );
		a.begin = a.birth = (int)in.col("BirthStep") + 1;
		a.end = in.col( "DeathStep" );

		agents[a.number] = a;
	}

	// ---
	// --- Add Agents to Epochs
	// ---
	itfor( AgentMap, agents, it )
	{
		Agent &a = it->second;

		// if agent was alive during time we're analyzing
		if( (a.begin >= begin && a.begin <= end)
			|| (a.end >= begin && a.end <= end) )
		{
#define IEPOCH(STEP) (STEP - begin) / epochlen;

			int iepoch_abegin = IEPOCH( max(a.begin, begin) );
			int iepoch_aend = IEPOCH( min( a.end, end) );

			// Update epochs this agent was alive during
			for( int iepoch = iepoch_abegin;
				 iepoch <= iepoch_aend;
				 iepoch++ )
			{
				Epoch &epoch = epochs[iepoch];

				// ---
				// --- Add time-clipped Agent to Epoch
				// ---
				Agent a_epoch;

				a_epoch.number = a.number;
				a_epoch.birth = a.birth;
				a_epoch.begin = max( a.begin, epoch.begin );
				a_epoch.end = min( a.end, epoch.end );

				epoch.agents.push_back( a_epoch );
			}
		}
	}

	DEBUG( itfor( EpochList, epochs, it )
		   {
			   printf( "--- %d ---\n", it->index );
			   printf( "  BEGIN=%ld, END=%ld\n", it->begin, it->end );

			   itfor( AgentList, it->agents, ita )
			   {
				   printf( "  #%ld  (%ld,%ld)\n", ita->number, ita->begin, ita->end );
			   }
		   } )
}

// -----------------------------------------------------------------------------
// ---
// --- getPresence()
// ---
// -----------------------------------------------------------------------------
float MotionComplexity::getPresence( Agent &agent,
									 Epoch &epoch )
{
	return float(agent.end - agent.begin)
		/ float(epoch.end - epoch.begin);
}

// -----------------------------------------------------------------------------
// ---
// --- populateMatrix()
// ---
// -----------------------------------------------------------------------------
gsl_matrix *MotionComplexity::populateMatrix( Epoch &epoch )
{
#if NIL_TYPE == NIL_RAND
	srand(epoch.begin);
#endif

	// ---
	// --- Alloc Matrix
	// ---
	int nagents = epoch.agents.size();
	int nsteps = epoch.end - epoch.begin + 1;
	int nrows = nagents * NDIMS;
	int ncols = nsteps;
	assert(nrows > 0 && ncols > 0);

	gsl_matrix *matrix_pos = gsl_matrix_alloc(nrows,
											  ncols);

	int row_x = 0;
	int row_z = 1;
	long nil_count = 0;

	// ---
	// --- Iterate Agents
	// ---
	itfor( AgentList, epoch.agents, it )
	{
		Agent &agent = *it;

		// ---
		// --- Open Position File
		// ---
		char path[512];
		sprintf( path,
				 "%s/motion/position/position_%ld.txt",
				 path_run, agent.number );

		DataLibReader in( path );
		in.seekTable( "Positions" );

		// ---
		// --- Presence Filter
		// ---
		if(getPresence(agent, epoch) < min_epoch_presence)
		{
			DEBUG(printf("agent %ld below presence threshold\n", agent.number));
			continue;
		}

		// ---
		// --- Iterate Steps
		// ---
		for(long step = epoch.begin;
		    step <= epoch.end;
		    step++)
		{
			float x;
			float z;

			if( step >= agent.begin && step <= agent.end )
			{
				in.seekRow( step - agent.birth );

				x = in.col( "x" );
				z = in.col( "z" );
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

			DEBUG( printf(" step=%ld, agent=%ld, (x,z)=(%f,%f)\n", step, agent.number, x, z) );

			gsl_matrix_set(matrix_pos,
						   row_x,
						   step - epoch.begin,
						   x);
			gsl_matrix_set(matrix_pos,
						   row_z,
						   step - epoch.begin,
						   z);
		}

		row_x += 2;
		row_z += 2;
	}

	// ---
	// --- Post-Processing Matrix Fixup
	// ---
	if(row_x == 0)
	{
		// All agents were filtered out
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

	epoch.complexity.nagents = row_x / 2;
	epoch.complexity.nil_ratio = float(nil_count) / (epoch.complexity.nagents * NDIMS * nsteps);

	return matrix_pos;
}
