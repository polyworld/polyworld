#include "GeneStats.h"

#include "GenomeUtil.h"
#include "objectxsortedlist.h"

using namespace genome;


GeneStats::GeneStats()
	: _maxAgents( 0 )
	, _agents( NULL )
	, _sum( NULL )
	, _sum2( NULL )
{
}

GeneStats::~GeneStats()
{
	if( _maxAgents )
	{
		delete [] _agents;
		delete [] _sum;
		delete [] _sum2;
		delete [] _mean;
		delete [] _stddev;
	}
}

void GeneStats::init( long maxAgents )
{
	if( _maxAgents != 0 )
	{
		assert( _maxAgents == maxAgents );
	}
	else
	{
		_maxAgents = maxAgents;

		_agents = new agent *[ maxAgents ];

		int ngenes = GenomeUtil::schema->getMutableSize();
		_sum  = new unsigned long[ ngenes ];
		_sum2 = new unsigned long[ ngenes ];
		_mean = new float[ ngenes ];
		_stddev = new float[ ngenes ];
	}
}

float *GeneStats::getMean()
{
	return _mean;
}

float *GeneStats::getStddev()
{
	return _stddev;
}

void GeneStats::compute( Scheduler &scheduler)
{
	if( _maxAgents )
	{
		// Because we'll be performing the stats calculations/recording in parallel
		// with the master task, which will kill and birth agents, we must create a
		// snapshot of the agents alive right now.
		_nagents = objectxsortedlist::gXSortedObjects.getCount(AGENTTYPE);
		objectxsortedlist::gXSortedObjects.reset();
		for( int i = 0; i < _nagents; i++ )
		{
			objectxsortedlist::gXSortedObjects.nextObj( AGENTTYPE, (gobject**)_agents + i );
		}

		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
		// ^^^ PARALLEL TASK Compute
		// ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
		class Compute : public ITask
		{
		public:
			GeneStats *stats;

			Compute( GeneStats *stats )
			{
				this->stats = stats;
			}

			virtual void task_exec( TSimulation *sim )
			{
				long nagents = stats->_nagents;
				agent **agents = stats->_agents;
				unsigned long *sum = stats->_sum;
				unsigned long *sum2 = stats->_sum2;
				int ngenes = GenomeUtil::schema->getMutableSize(); 

				memset( sum, 0, sizeof(*sum) * ngenes );
				memset( sum2, 0, sizeof(*sum2) * ngenes );

				for( int i = 0; i < nagents; i++ )
				{
					agents[i]->Genes()->updateSum( sum, sum2 );
				}

				float *mean = stats->_mean;
				float *stddev = stats->_stddev;
				for( int i = 0; i < ngenes; i++ )
				{
					mean[i] = (float) sum[i] / (float) nagents;
					stddev[i] = sqrt( (float) sum2[i] / (float) nagents  -  mean[i] * mean[i] );
				}
			}
		};

		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		// !!! POST PARALLEL
		// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
		scheduler.postParallel( new Compute(this) );
	}
}
