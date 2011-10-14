// Cluster C code goes in polyworld/tools/cluster
// Cluster animation code goes in polyworld/scripts

#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "readline.h"
#include <omp.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include <algorithm>
#include <iostream>
#include <fstream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION usage
// ---
// --------------------------------------------------------------------------------
void usage( const char *msg = NULL ) {
	fprintf( stderr, "usage:\n" );
	fprintf( stderr, "\n" );
	fprintf( stderr, "qt_clust cluster [-m clusterPartitionModulus] [-f threshFact] [run]\n" );
	fprintf( stderr, "\tPerform cluster analysis.\n" );
	fprintf( stderr, "\n" );
	fprintf( stderr, "qt_clust compareCentroids [-n max_clusters] subdir_A subdir_B [run]\n" );
	fprintf( stderr, "\tCompute the distance between cluster centroids from two cluster files.\n" );
	fprintf( stderr, "\n" );
	fprintf( stderr, "qt_clust checkThresh [-n <max_clusters>] subdir [run]\n" );
	fprintf( stderr, "\tAnalyze how well clusters conform to THRESH.\n" );
	fprintf( stderr, "\n" );
	fprintf( stderr, "qt_clust centroidDists [-n <max_clusters>] subdir [run]\n" );
	fprintf( stderr, "\tCompute distance between centroids of all clusters.\n" );
	fprintf( stderr, "\n" );
	fprintf( stderr, "\n" );
	fprintf( stderr, "Examples:\n" );
	fprintf( stderr, "\n" );
	fprintf( stderr, "\tqt_clust cluster -m 10 ./run\n" );
	fprintf( stderr, "\n" );
	fprintf( stderr, "\tqt_clust compareCentroids -n 10 m1f2.125 m10f2.125\n" );
	
	if( msg ) {
		fprintf( stderr, "\n" );
		fprintf( stderr, "%s\n", msg );
	}

	exit( 1 );
}

// ================================================================================
// ================================================================================
// ================================================================================
// ================================================================================
// ===
// === GLOBALS
// ===
// ================================================================================
// ================================================================================
// ================================================================================
// ================================================================================
float THRESH = 0;
int GENES = 0;

struct CliParms {
	int clusterPartitionModulus;
	float threshFact;
	const char *path_run;
	int nclusters;

	CliParms() {
		clusterPartitionModulus = 1;
		threshFact = 2.125;
		path_run = "../../run";
		nclusters = -1;
	}
} cliParms;

// ================================================================================
// ================================================================================
// ================================================================================
// ================================================================================
// ===
// === MACROS
// ===
// ================================================================================
// ================================================================================
// ================================================================================
// ================================================================================

// TODO: Make pathname an argument
// TODO: Make constants GENES and POPSIZE determined from run directory
//       Implies dynamic allocation of G, std2, dists, etc
// TODO: Write cluster info to a file (in run/cluster/<std.dev.>)

#define SORT_GENES false

#include "config.h"

// Debug flags
#define VERBOSE false
#define PRINT_ELTS 1
#define DEBUG 0
#define SLEEP_AT_END false
#define SANITY_CHECKS true
#define PROMPT_STAGES false

// iterator for loop
#define itfor(TYPE,CONT,IT)						\
	for(TYPE::iterator							\
			IT = (CONT).begin(),				\
			IT##_end = (CONT).end();			\
		IT != IT##_end;							\
		++IT)

// reverse iterator for loop
#define ritfor(TYPE,CONT,IT)					\
	for(TYPE::reverse_iterator					\
			IT = (CONT).rbegin(),				\
			IT##_end = (CONT).rend();			\
		IT != IT##_end;							\
		++IT)

// const iterator for loop
#define citfor(TYPE,CONT,IT)					\
	for(TYPE::const_iterator					\
			IT = (CONT).begin(),				\
			IT##_end = (CONT).end();			\
		IT != IT##_end;							\
		++IT)

#define errif( condition, msg... )										\
	if(condition) {														\
		fprintf(stderr, "%s:%d Failed condition \"%s\"\n", __FILE__, __LINE__, #condition); \
		fprintf(stderr, msg);											\
		exit(1);														\
	}

#define err( msg... ) {fprintf(stderr, msg); exit(1);}

#if PROMPT_STAGES
#define STAGE(MESSAGE...) {printf(MESSAGE); printf("Press enter..."); getchar();}
#else
#define STAGE(MESSAGE...) printf(MESSAGE)
#endif


// ================================================================================
// ================================================================================
// ================================================================================
// ================================================================================
// ===
// ===
// ===
// ================================================================================
// ================================================================================
// ================================================================================
// ================================================================================
typedef int AgentId;
typedef vector<AgentId> AgentIdVector;
typedef set<AgentId> AgentIdSet;

typedef int AgentIndex;
typedef set<AgentIndex> AgentIndexSet;

// ================================================================================
// ================================================================================
// ================================================================================
// ================================================================================
// ===
// === PROTOTYPES
// ===
// ================================================================================
// ================================================================================
// ================================================================================
// ================================================================================
void write_members( FILE *f, class Cluster *cluster );
unsigned char *load_genome( AgentId id );


// --------------------------------------------------------------------------------
// ---
// --- FUNCTION hirestime
// ---
// --------------------------------------------------------------------------------
static double hirestime( void )
{
	struct timeval tv;
	gettimeofday( &tv, NULL );

	return (double)tv.tv_sec + (double)tv.tv_usec/1000000.0;
}

// --------------------------------------------------------------------------------
// --- TODO: We don't need to use this class anymore. Switch to using vector.
// ---
// --- CLASS AgentIndexArray
// ---
// --- Designed to hold a set of agent Indexes, where emphasis is placed on small
// --- RAM footprint and efficient computation of intersection.
// ---
// --------------------------------------------------------------------------------
class AgentIndexArray {
public:
	AgentIndexArray( AgentIndexSet &set ) {
		sorted = true;
		numIndexes = set.size();
		indexes = new AgentIndex[ numIndexes ];

		int i = 0;
		itfor( AgentIndexSet, set, it ) {
			indexes[i++] = *it;
		}
	}

	AgentIndexArray( const AgentIndex *buffer, size_t nelements ) {
		sorted = false;
		numIndexes = nelements;
		indexes = new AgentIndex[ numIndexes ];

		memcpy( indexes, buffer, numIndexes * sizeof(AgentIndex) );
	}

	~AgentIndexArray() {
		delete indexes;
	}

	inline AgentIndex &operator[]( int i ) {
		assert( (i >= 0) && (i < numIndexes) );

		return indexes[i];
	}

	inline size_t size() const {
		return numIndexes;
	}

	inline bool empty() const {
		return numIndexes == 0;
	}

	inline void sort() {
		if( !sorted ) {
			qsort( indexes, numIndexes, sizeof(AgentIndex), compareIndex );
			sorted = true;
		}
	}

	inline bool intersects( AgentIndexArray &other ) {
		if( this == &other )
			return true;

		if( other.size() < this->size() ) {
			return other.intersects( *this );
		} else {
			assert( other.sorted );

			for( int i = 0; i < numIndexes; i++ )
				if( other.contains(indexes[i]) )
					return true;

			return false;
		}
	}

private:
	inline bool contains( AgentIndex index ) {
		return NULL != bsearch( &index, indexes, numIndexes, sizeof(AgentIndex), compareIndex );
	}


	static int compareIndex( const void *_x, const void *_y )
	{
		AgentIndex x = *(AgentIndex *)_x;
		AgentIndex y = *(AgentIndex *)_y;

		return x - y;
	}

	bool sorted;
	size_t numIndexes;
	AgentIndex *indexes;
};

// --------------------------------------------------------------------------------
// ---
// --- CLASS PopulationPartition
// ---
// --------------------------------------------------------------------------------
class PopulationPartition {
public:
	PopulationPartition( AgentIdVector *_members,
						 bool _keepGenomesInRamAfterStatisticsComputed,
						 bool _keepGenomesInRamAfterCentroidsComputed)
		: members(*_members)
		, keepGenomesInRamAfterStatisticsComputed(_keepGenomesInRamAfterStatisticsComputed)
		, keepGenomesInRamAfterCentroidsComputed(_keepGenomesInRamAfterCentroidsComputed)
	{
		genomes = new unsigned char*[ members.size() ];
		memset( genomes, 0, sizeof(unsigned char*) * members.size() );

		// The IDs must be sorted because we use a binary search to map from ID to index -- see getIndex()
		sort( members.begin(), members.end() );

#if SANITY_CHECKS
		for( int i = 0; i < members.size(); ++i ) {
			errif( getIndex( getId(i) ) != i,
				   "i=%d, id=%d, index=%d\n",
				   i, getId(i), getIndex(getId(i)) );
		}
#endif
	}

	~PopulationPartition() {
		for( int i = 0; i < members.size(); i++ ) {
			if(genomes[i]) delete genomes[i];
		}
		delete genomes;
		delete &members;
	}

	inline unsigned char *getGenomeById( AgentId id ) {
		return getGenomeByIndex( getIndex(id) );
	}

	inline unsigned char *getGenomeByIndex( AgentIndex index ) {
		unsigned char *genome = genomes[index];
		if( genome == NULL ) {
			#pragma omp critical( PopulationPartition_genomes )
			{
				genome = genomes[index];
				if( genome == NULL ) {
					genome = load_genome( getId(index) );
					genomes[index] = genome;
				}
			}
		}

		return genome;
	}

	void endGenomeStatisticsComputation( AgentId id ) {
		if( !keepGenomesInRamAfterStatisticsComputed ) {
			deleteGenome( id );
		}
	}

	void endCentroidsComputation() {
		if( !keepGenomesInRamAfterCentroidsComputed ) {
			deleteGenomes();
		}
	}

	void endNeighborComputation( AgentId id ) {
		deleteGenome( id );
	}

	inline AgentId getId( AgentIndex index ) {
		return members[index];
	}

	inline AgentIndex getIndex( AgentId id ) {
		// perform binary search
		AgentIdVector::iterator it = lower_bound( members.begin(), members.end(), id );
		assert( *it == id );

		return distance( members.begin(), it );
	}

	inline AgentIdVector *createAgentIdVector( AgentIndexArray &indexes ) {
		AgentIdVector *result = new AgentIdVector(); // TODO constructor specifying capacity?

		for( int i = 0; i < indexes.size(); i++ ) {
			result->push_back( getId(indexes[i]) );
		}

		return result;
	}

private:
	void deleteGenomes() {
		itfor( AgentIdVector, members, it ) {
			deleteGenome( *it );
		}
	}

	void deleteGenome( AgentId id ) {
		AgentIndex index = getIndex( id );
		if( genomes[index] ) {
			delete genomes[index];
			genomes[index] = NULL;
		}
	}

public:
	AgentIdVector &members;

private:
	bool keepGenomesInRamAfterStatisticsComputed;
	bool keepGenomesInRamAfterCentroidsComputed;
	unsigned char **genomes;
};

typedef vector<PopulationPartition *> PopulationPartitionVector;

// --------------------------------------------------------------------------------
// ---
// --- CLASS GeneEntropyCalculator
// ---
// --------------------------------------------------------------------------------
class GeneEntropyCalculator {
public:
	GeneEntropyCalculator() {
		numGenomes = 0;

		bins = new int*[NUM_BINS];
		for( int i = 0; i < NUM_BINS; i++ ) {
			bins[i] = new int[GENES];
			memset( bins[i], 0, GENES * sizeof(int) );
		}
	}

	~GeneEntropyCalculator() {
		for( int i = 0; i < NUM_BINS; i++ ) {
			delete bins[i];
		}
		delete bins;
	}

	void addGenome( unsigned char *genome ) {
		numGenomes++;

		for( int igene = 0; igene < GENES; igene++ ) {
			// TODO: this bin calculation only works because 16 * 16 = 256
			int bin = genome[igene] / NUM_BINS;
			bins[bin][igene] += 1;
		}
	}

	float *getResult() {
		float *result = new float[GENES];
		memset( result, 0, GENES * sizeof(float) );

		for( int igene = 0; igene < GENES; igene++ ) {
			for( int ibin = 0; ibin < NUM_BINS; ibin++ ) {
				if( bins[ibin][igene] != 0 ) {
					float p = (float) bins[ibin][igene] / numGenomes;
					result[igene] += (-p * (log(p) / log(NUM_BINS)));
				}
			}

			result[igene] = 1 - result[igene];
		}

		return result;
	}


private:
	int numGenomes;
	int **bins;
};

// --------------------------------------------------------------------------------
// ---
// --- CLASS GeneStdDev2Calculator
// ---
// --------------------------------------------------------------------------------
class GeneStdDev2Calculator {
public:
	GeneStdDev2Calculator() {
		numGenomes = 0;

		sum = new double[GENES];
		memset( sum, 0, GENES * sizeof(double) );
		sum2 = new double[GENES];
		memset( sum2, 0, GENES * sizeof(double) );
	}

	~GeneStdDev2Calculator() {
		delete sum;
		delete sum2;
	}

	void addGenome( unsigned char *genome ) {
		numGenomes++;

		for( int i = 0; i < GENES; i++ ) {
			sum[i] += genome[i];
			sum2[i] += genome[i] * genome[i];
		}
	}

	float *getResult() {
		double mean[GENES];
		for( int i = 0; i < GENES; i++ ) {
			mean[i] = sum[i] / numGenomes;
		}

		float *result = new float[GENES];
		for( int i = 0; i < GENES; i++ ) {
			result[i] = float( sum2[i] / numGenomes - mean[i] * mean[i] );
		}
 
		return result;
	}

private:
	int numGenomes;
	double *sum;
	double *sum2;
};

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION compute_distance
// ---
// --- Calculate distance between two genomes.
// ---
// --------------------------------------------------------------------------------
inline float compute_distance(float **deltaCache, unsigned char *x, unsigned char *y) {
	float sum = 0;

	for (int gene = 0; gene < GENES; gene++) {
		int delta = x[gene] - y[gene];

		sum += delta < 0 ? deltaCache[gene][-delta] : deltaCache[gene][delta];
	}

	return sum;
}
/*
inline float compute_distance(float *ents, float *stddev2, unsigned char *x, unsigned char *y) {
	int tmp; float sum = 0;

	for (int gene = 0; gene < (GENES*.80); gene++) {
        tmp = x[gene] - y[gene];

        if (tmp) {
			tmp *= tmp;
            sum += (ents[gene] * tmp) / stddev2[gene];
		}
	}

	if( sum > THRESH ) return sum;

	for (int gene = (GENES*.80); gene < (GENES*.90); gene++) {
        tmp = x[gene] - y[gene];

        if (tmp) {
			tmp *= tmp;
            sum += (ents[gene] * tmp) / stddev2[gene];
		}
	}

	if( sum > THRESH ) return sum;

	for (int gene = (GENES*.90); gene < (GENES); gene++) {
        tmp = x[gene] - y[gene];

        if (tmp) {
			tmp *= tmp;
            sum += (ents[gene] * tmp) / stddev2[gene];
		}
	}

	return sum;
}
*/

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION create_distance_deltaCache
// ---
// --------------------------------------------------------------------------------
float **create_distance_deltaCache( float *ents, float *stddev2 ) {
	float **deltaCache = new float*[GENES];

	for( int igene = 0; igene < GENES; igene++ ) {
		deltaCache[igene] = new float[256];

		for( int delta = 0; delta < 256; delta++ ) {
			float result;

			int tmp = delta;

			if (tmp) {
				tmp *= tmp;
				result = (ents[igene] * tmp) / stddev2[igene];
			} else {
				result = 0;
			}

			deltaCache[igene][delta] = result;
		}
	}

	return deltaCache;
}

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION create_distanceCache
// ---
// --------------------------------------------------------------------------------
float **create_distanceCache( float **deltaCache, PopulationPartition *population ) {
	int numGenomes = population->members.size();

	// allocate distance cache
	float **distanceCache = new float*[numGenomes];
	for( int i = 0; i < numGenomes; i++ ) {
		distanceCache[i] = new float[numGenomes];
	}

	double startTime = hirestime();

	for( AgentIndex i = 0; i < numGenomes; i++) {
		float *D = distanceCache[i];
        
		#pragma omp parallel for
		for( AgentIndex j = i+1; j < numGenomes; j++ ) {
			float d = compute_distance(deltaCache, population->getGenomeByIndex(i), population->getGenomeByIndex(j));
			D[j] = d;
		}
        
		D[i] = 0.0;
        
		#pragma omp parallel for
		for( AgentIndex j = 0; j < i; j++ ) {
			float d = distanceCache[j][i];
			D[j] = d;
		}
	}

	double endTime = hirestime();
	printf( "distance time=%f seconds\n", endTime - startTime );

	/*
	  {
	  FILE *f = fopen("dists.txt", "w");
	  for( i = 0; i < POPSIZE; i++ ) {
	  for( j = i+1; j < POPSIZE; j++ ) {
	  float val = distanceCache[i][j];
	  if(val > THRESH) val = THRESH;
	  fprintf( f, "%f ", val );
	  }
	  fprintf(f, "\n");
	  }
	  fclose(f);
	  }
	*/

	return distanceCache;
}

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION dispose_distanceCache
// ---
// --------------------------------------------------------------------------------
void dispose_distanceCache( float **distanceCache, PopulationPartition *population ) {
	for( int i = 0; i < population->members.size(); i++ ) {
		delete distanceCache[i];
	}
	delete distanceCache;
}

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION get_distance
// ---
// --- Fetch genomic distance between two agents from cache
// ---
// --------------------------------------------------------------------------------
inline float get_distance( float **distanceCache, AgentIndex x, AgentIndex y ) {
	return distanceCache[x][y];
}

// --------------------------------------------------------------------------------
// ---
// --- CLASS Cluster
// ---
// --------------------------------------------------------------------------------
typedef int ClusterId;

class Cluster {
public:
	Cluster( PopulationPartition *_population, ClusterId _id, AgentIdVector *_members )
		: members( *_members )
	{
		population = _population;
		id = _id;
		centroidGenome = NULL;
	}

	~Cluster() {
		delete &members;
		if( centroidGenome ) delete centroidGenome;
	}

	void createCentroid() {
		float sum[GENES];
		memset( sum, 0, GENES * sizeof(float) );

		itfor( AgentIdVector, members, it ) {
			unsigned char *memberGenome = population->getGenomeById( *it );
			
			for( int igene = 0; igene < GENES; igene++ ) {
				sum[igene] += memberGenome[igene];
			}
		}

		centroidGenome = new unsigned char[GENES];
		for( int i = 0; i < GENES; i++ ) {
			centroidGenome[i] = (unsigned char)( sum[i] / members.size() + 0.5 );
		}
	}

	static bool sort__member_size_descending( const Cluster *x, const Cluster *y ) {
		return y->members.size() < x->members.size();
	}

	PopulationPartition *population;
	ClusterId id;
	AgentIdVector &members;
	AgentIdVector neighbors;
	unsigned char *centroidGenome;
};

typedef vector<Cluster *> ClusterVector;
typedef vector<ClusterVector *> ClusterMatrix;

// --------------------------------------------------------------------------------
// ---
// --- CLASS ListBuffer
// ---
// --- A lightweight doubly-linked list, where all nodes reside in single heap
// --- buffer allocated on instantiation. Links are achieved with int rather than
// --- pointers to save RAM.
// ---
// --------------------------------------------------------------------------------
template< typename T >
class ListBuffer {
public:
	struct Node {
		T data;
		int prev;
		int next;
	};
public:

	ListBuffer( int capacity ) {
		assert( capacity > 0 );

		_buffer = new Node[capacity];
		_head = _buffer;

		for( int i = 0; i < capacity; i++ ) {
			Node *node = _buffer + i;

			node->prev = i - 1;
			node->next = i + 1;
		}

		_buffer[capacity - 1].next = -1;
	}

	~ListBuffer() {
		delete _buffer;
	}

	inline T *operator[]( int index ) {
		return (T*)(_buffer + index);
	}

	inline bool empty() {
		return _head == NULL;
	}

	inline T *head() {
		return (T *)_head;
	}

	inline T *next( T *curr ) {
		if( ((Node*)curr)->next == -1 ) {
			return NULL;
		} else {
			return (T*)(_buffer + ((Node*)curr)->next);
		}
	}

	inline void remove( T *data ) {
		if( ((Node*)data)->prev != -1 ) {
			_buffer[((Node*)data)->prev].next = ((Node*)data)->next;
		}
		if( ((Node*)data)->next != -1 ) {
			_buffer[((Node*)data)->next].prev = ((Node*)data)->prev;
		}
		if( ((Node*)data) == _head ) {
			if( ((Node*)data)->next == -1 ) 
				_head = NULL;
			else
				_head = _buffer + ((Node*)data)->next;
		}
	}

private:
	Node *_head;
	Node *_buffer;
};

void print_vector(int v[], int size) {
	int i;
	printf("| [");
	for (i =0; i < size; i++) {
		printf("%3d ", v[i]);
	}
	printf("]\n");
}

void print_float_vector(float v[], int size) {
	int i;
	printf("(");
	for (i =0; i < size; i++) {
		printf(" %f ", v[i]);
	}
	printf(")\n");
}

int sum_arr(int a[], int ELTS) { 
	int i, sum; sum = 0;
	for (i = 0; i < ELTS; i++)
		sum += a[i];
	return sum;
}

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION create_candidate_cluster
// ---
// --- Build a candidate cluster from a given starting agent.
// ---
// --------------------------------------------------------------------------------
struct MaxDist {
	AgentIndex index;
	float dist;
};

AgentIndexArray *create_candidate_cluster( float **distanceCache,
										   AgentIndex startAgent,
										   const AgentIndexSet &allAgents ) {
	AgentIndex clusterAgents[ allAgents.size() ];
	size_t numClusterAgents = 0;
#define ADD_CLUSTER_AGENT( ID ) clusterAgents[numClusterAgents++] = ID;

	ADD_CLUSTER_AGENT( startAgent );

	if( allAgents.size() > 1 ) {
		ListBuffer<MaxDist> max_dists( allAgents.size() - 1 );
		{
			MaxDist *node = max_dists.head();
			citfor( AgentIndexSet, allAgents, it ) {
				AgentId index = *it;

				if( index != startAgent ) {
					node->index = index;
					node->dist = 0;

					node = max_dists.next( node );
				}
			}
		}	

		AgentIndex lastPick = startAgent;

		if (DEBUG) printf("%dC ", lastPick);

		while( !max_dists.empty() ) {
			float pickDist = THRESH + 1;
			MaxDist *pick = NULL;

			for( MaxDist *node = max_dists.head(); node != NULL; node = max_dists.next( node ) ) {
				if( node->dist <= THRESH ) {
					float newDist = get_distance( distanceCache, lastPick, node->index );
					if (DEBUG) printf("%d -> %d: %f (cur: %f) \n", lastPick, node->index, newDist, node->dist);
					if( newDist > node->dist ) node->dist = newDist;
					if (DEBUG) printf("%d -> %d: %f (cur: %f) \n", startAgent, node->index, newDist, node->dist);
				}

				if( node->dist > THRESH ) {
					max_dists.remove( node );
				} else if( node->dist < pickDist ) {
					pick = node;
					pickDist = pick->dist;
				}
			}

			if( (pick != NULL) && (pick->dist <= THRESH) ) {
				if(DEBUG) printf("  pick=(%d,%f)\n", pick->index, pick->dist);
				lastPick = pick->index;
				ADD_CLUSTER_AGENT( pick->index );
				max_dists.remove( pick );
			}
		}
	}

	AgentIndexArray *result = new AgentIndexArray( clusterAgents, numClusterAgents );

	if (DEBUG) printf("%dC | (len %lu)\n", startAgent, result->size());

	return result;
}


// --------------------------------------------------------------------------------
// ---
// --- FUNCTION create_cluster
// ---
// --- Create the largest cluster possible for the remaining agents.
// ---
// --------------------------------------------------------------------------------
Cluster *create_cluster( float **distanceCache,
						 PopulationPartition *population,
						 AgentIndexSet &remainingAgents,
						 ClusterId clusterId ) {

	AgentIndexArray *biggestMembers = NULL;
	AgentIndex biggestStartAgent;

	AgentIndexSet::iterator it = remainingAgents.begin();
	AgentIndexSet::iterator it_end = remainingAgents.end();
	
	#pragma omp parallel
	{
		AgentIndexSet::iterator it_threadLocal = it_end;

		do {
			#pragma omp critical( cluster_single__it )
			{
				it_threadLocal = it;
				if( it != it_end ) {
					++it;
				}
			}

			if( it_threadLocal != it_end ) {
				AgentIndex startAgent = *it_threadLocal;
				AgentIndexArray *members = create_candidate_cluster( distanceCache, 
																	 startAgent, 
																	 remainingAgents );

				#pragma omp critical ( cluster_single__biggest )
				{
					if( (biggestMembers == NULL)
						|| (members->size() > biggestMembers->size())
						|| ((members->size() == biggestMembers->size()) && (startAgent < biggestStartAgent)) )
					{
						if( biggestMembers ) {
							delete biggestMembers;
						}

						biggestMembers = members;
						biggestStartAgent = startAgent;
					} else {
						delete members;
					}
				}
			}
		} while( it_threadLocal != it_end );
	}

	biggestMembers->sort();

	Cluster *result = new Cluster( population,
								   clusterId,
								   population->createAgentIdVector(*biggestMembers) );

	delete biggestMembers;

	return result;
}

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION create_clusters
// ---
// --- Assigns all agents in partition to a cluster.
// ---
// --------------------------------------------------------------------------------
void create_clusters( float **distance_deltaCache,
					  PopulationPartition *population,
					  ClusterVector &allClusters ) {
	printf("calculating distances...\n");
	float **distanceCache = create_distanceCache( distance_deltaCache, population );

	double startTime = hirestime();

	// ---
	// --- Create set of agent indexes to be clustered
	// ---
	AgentIndexSet remainingAgents;
	for( int i = 0; i < population->members.size(); i++) {
		remainingAgents.insert( i );
	}

	// ---
	// --- Create clusters in until no agents remaining.
	// ---
    printf("starting clustering...\n");

	ClusterVector clusters;

	while( !remainingAgents.empty() ) {
		Cluster *cluster = create_cluster( distanceCache,
										   population,
										   remainingAgents,
										   clusters.size() + allClusters.size());
		clusters.push_back( cluster );

		// remove cluster members from agents needing processing
		for( int i = 0; i < cluster->members.size(); i++ ) {
			remainingAgents.erase( population->getIndex(cluster->members[i]) );
		}

#if VERBOSE
		write_cluster( stdout, cluster );
#endif
	}

	double endTime = hirestime();
	printf( "clustering time=%f seconds\n", endTime - startTime );

#if SANITY_CHECKS
	itfor( ClusterVector, clusters, it_cluster ) {
		Cluster *cluster = *it_cluster;

		for( int i = 0; i < cluster->members.size(); i++ ) {
			for( int j = i+1; j < cluster->members.size(); j++ ) {
				AgentIndex iindex = population->getIndex( cluster->members[i] );
				AgentIndex jindex = population->getIndex( cluster->members[j] );
				float dist = get_distance( distanceCache,
										   iindex,
										   jindex );
				errif( dist > THRESH,
					   "cluster=%d, i=%d, j=%d, nmembers=%lu, dist=%f, THRESH=%f\n",
					   cluster->id, i, j, cluster->members.size(), dist, THRESH );
			}
		}
	}
#endif

	// ---
	// --- Dispose Distance Cache
	// ---
	dispose_distanceCache( distanceCache, population );

	// ---
	// --- Add clusters to result
	// ---
	itfor( ClusterVector, clusters, it ) {
		allClusters.push_back( *it );
	}
}

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION create_centroids
// ---
// --------------------------------------------------------------------------------
void create_centroids( float **distance_deltaCache,
					   PopulationPartition *population,
					   ClusterVector &clusters ) {

	#pragma omp parallel for
	for( int i = 0; i < clusters.size(); i++ ) {
		clusters[i]->createCentroid();
	}

#if SANITY_CHECKS
	itfor( ClusterVector, clusters, it_cluster ) {
		Cluster *cluster = *it_cluster;
		itfor( AgentIdVector, cluster->members, it_member ) {
			float dist = compute_distance(distance_deltaCache,
										  population->getGenomeById(*it_member),
										  cluster->centroidGenome);
			errif( dist > THRESH,
				   "cluster=%d, member=%d, nmembers=%lu, dist=%f, THRESH=%f\n",
				   cluster->id, *it_member, cluster->members.size(), dist, THRESH );
		}
	}
#endif
}

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION find_neighbors
// ---
// --------------------------------------------------------------------------------
void find_neighbors( float **distance_deltaCache,
					 PopulationPartition *clusterPartition,
					 PopulationPartition *neighborPartition,
					 ClusterVector &clusters_unsorted,
					 AgentIdVector &orphans ) {
	double startTime = hirestime();

	ClusterVector clusters( clusters_unsorted );
	sort( clusters.begin(), clusters.end(), Cluster::sort__member_size_descending );

#if SANITY_CHECKS
	// We assume clusters are sorted by size in descending order.
	for( int i = 1; i < clusters.size(); i++ ) {
		errif( clusters[i]->members.size() > clusters[i-1]->members.size(), "i=%d\n", i );
	}
#endif

	AgentIdSet neighborCandidates( neighborPartition->members.begin(), neighborPartition->members.end() );
	AgentIdVector neighborCandidatesVector;

	for( int icluster = 0; icluster < clusters.size(); icluster++ ) {
		Cluster *cluster = clusters[icluster];

		neighborCandidatesVector.resize( neighborCandidates.size() );
		copy( neighborCandidates.begin(), neighborCandidates.end(), neighborCandidatesVector.begin() );

		// ---
		// --- Determine neighbor candidates for cluster by finding agents that don't violate THRESH
		// --- for any of cluster's members.
		// ---
		AgentIdVector clusterNeighborCandidates;

		#pragma omp parallel for
		for( int ineighbor = 0; ineighbor < neighborCandidatesVector.size(); ineighbor++ ) {
			AgentId neighborId = neighborCandidatesVector[ ineighbor ];
			unsigned char *neighborGenome = neighborPartition->getGenomeById( neighborId );
			bool isNeighbor = true;

			for( int iclusterMember = 0;
				 isNeighbor && (iclusterMember < cluster->members.size());
				 iclusterMember++ )
			{
				AgentId clusterId = cluster->members[ iclusterMember ];
				unsigned char *clusterGenome = clusterPartition->getGenomeById( clusterId );
				float dist = compute_distance( distance_deltaCache,
											   clusterGenome,
											   neighborGenome );
				if( dist > THRESH ) {
					isNeighbor = false;
				}
			}

			if( isNeighbor ) {
				#pragma omp critical(clusterNeighborCandidates)
				{
					clusterNeighborCandidates.push_back( neighborId );
				}
			}
		}

		// ---
		// --- Eliminate neighbor candidates that violate THRESH with other candidates.
		// ---
		AgentIdSet validNeighbors( clusterNeighborCandidates.begin(), clusterNeighborCandidates.end() );

		typedef map<AgentId, AgentIdSet> ViolationMap;
		ViolationMap violations;

		typedef map<AgentId, double> DistMap;
		DistMap dists;

		for( int i = 0; i < clusterNeighborCandidates.size(); i++ ) {
			dists[ clusterNeighborCandidates[i] ] = 0;
		}

		for( int i = 0; i < clusterNeighborCandidates.size(); i++ ) {
			AgentId i_id = clusterNeighborCandidates[i];
			unsigned char *i_genome = neighborPartition->getGenomeById( i_id );

			#pragma omp parallel for
			for( int j = i + 1; j < clusterNeighborCandidates.size(); j++ ) {
				AgentId j_id = clusterNeighborCandidates[j];
				unsigned char *j_genome = neighborPartition->getGenomeById( j_id );

				float dist = compute_distance( distance_deltaCache,
											   i_genome,
											   j_genome );

				double &i_dist = dists[i_id];
				#pragma omp atomic
				i_dist += dist;

				double &j_dist = dists[j_id];
				#pragma omp atomic
				j_dist += dist;

				if( dist > THRESH ) {
					#pragma omp critical(violations)
					{
						violations[i_id].insert( j_id );
						violations[j_id].insert( i_id );
					}
				}
			}
		}

		while( !violations.empty() ) {
			ViolationMap::iterator it_biggest = violations.end();
		
			itfor( ViolationMap, violations, it_violations ) {
				if( (it_biggest == violations.end()) || (it_violations->second.size() > it_biggest->second.size()) ) {
					it_biggest = it_violations;
				}
			}

			if( it_biggest->second.size() == 1 ) {
				break;
			}

			itfor( AgentIdSet, it_biggest->second, it_id ) {
				AgentIdSet &otherSet = violations[*it_id];
				otherSet.erase( it_biggest->first );
				if( otherSet.empty() ) {
					violations.erase( *it_id );
				}
			}
			validNeighbors.erase( it_biggest->first );
			violations.erase( it_biggest );
		}

		itfor( ViolationMap, violations, it_violations ) {
			AgentId first = it_violations->first;
			AgentId second = *( it_violations->second.begin() );

			if( first < second ) {
				if( dists[first] < dists[second] ) {
					validNeighbors.erase( second );
				} else {
					validNeighbors.erase( first );
				}
			}
		}

		cluster->neighbors.resize( validNeighbors.size() );
		copy( validNeighbors.begin(), validNeighbors.end(), cluster->neighbors.begin() );

		itfor( AgentIdSet, validNeighbors, it ) {
			neighborCandidates.erase( *it );
		}
	}

	orphans.resize( neighborCandidates.size() );
	copy( neighborCandidates.begin(), neighborCandidates.end(), orphans.begin() );

	double endTime = hirestime();

#if VERBOSE
	itfor( ClusterVector, clusters, it_cluster ) {
		Cluster *cluster = *it_cluster;

		if( !cluster->neighbors.empty() ) {
			printf( "neighbors %d (%lu elts) : ", cluster->id, cluster->neighbors.size() );
			itfor( AgentIdVector, cluster->neighbors, it_neighbor ) {
				printf( "%d ", *it_neighbor );
			}
			printf("\n");
		}
	}
#endif

	printf( "find neighbors time=%f seconds\n", endTime - startTime );
	printf( "  %% orphans=%f (%lu/%lu)\n",
			float(orphans.size()) / neighborPartition->members.size(),
			orphans.size(),
			neighborPartition->members.size() );
}

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION is_regular_file
// ---
// --------------------------------------------------------------------------------
bool is_regular_file( const char *path ) {
		struct stat st;
		int rc = stat( path, &st );

		return (rc == 0) && S_ISREG(st.st_mode);
}


// --------------------------------------------------------------------------------
// ---
// --- FUNCTION read_last_nonblank_line
// ---
// --------------------------------------------------------------------------------
char *read_last_nonblank_line( const char *path ) {
	errif( !is_regular_file(path), "Invalid file: %s\n", path );

	FILE *f = fopen( path, "r" );
	errif( !f, "Failed opening %s\n", path );

	long linestart = -1;
	long lineend;
	bool nonblank = false;

	int rc = fseek( f, -1, SEEK_END );
	errif( rc != 0, "Failed seeking end of %s\n", path );

	lineend = ftell(f);

	while( true ) {
		long offset = ftell(f);
		if( offset == 0 ) {
			linestart = 0;
			break;
		}

		char c = fgetc(f);

		if( c == '\n' ) {
			if( nonblank ) {
				linestart = offset + 1;
				break;
			} else {
				lineend = offset - 1;
			}
		} else if( !isblank(c) ) {
			nonblank = true;
		}

		rc = fseek( f, offset - 1, SEEK_SET );
		errif( rc != 0, "Failed seeking to offset %ld of %s\n", offset - 1, path );
	}

	if( !nonblank ) {
		return NULL;
	} else {
		rc = fseek( f, linestart, SEEK_SET );
		errif( rc != 0, "Failed seeking linestart %ld of %s\n", linestart, path );

		long linelen = lineend - linestart + 1;
		char *result = (char *)malloc( linelen + 1 );
		rc = fread( result, 1, linelen, f );
		errif( rc != linelen, "Failed reading line of %s\n", path );

		result[linelen] = 0;

		return result;
	}
}

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION get_run_path
// ---
// --------------------------------------------------------------------------------
char *get_run_path( const char *relpath ) {
	char buf[1024];

	if( cliParms.path_run[strlen(cliParms.path_run) - 1] == '/' ) {
		sprintf( buf, "%s%s", cliParms.path_run, relpath );
	} else {
		sprintf( buf, "%s/%s", cliParms.path_run, relpath );
	}

	return strdup( buf );
}

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION write_members
// ---
// --------------------------------------------------------------------------------
void write_members( FILE *f, Cluster *cluster ) {
	if( !cluster->members.empty() ) {
		fprintf( f, "cluster %d (%lu elts) : ", cluster->id, cluster->members.size() );

		itfor( AgentIdVector, cluster->members, it ) {
			fprintf( f, "%d ", *it );
		}
		fprintf( f, "\n" );
	}
}

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION write_neighbors
// ---
// --------------------------------------------------------------------------------
void write_neighbors( FILE *f, Cluster *cluster ) {
	if( !cluster->neighbors.empty() ) {
		fprintf( f, "cluster %d (%lu elts) : ", cluster->id, cluster->neighbors.size() );

		itfor( AgentIdVector, cluster->neighbors, it ) {
			fprintf( f, "%d ", *it );
		}
		fprintf( f, "\n" );
	}
}

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION write_members_and_neighbors
// ---
// --------------------------------------------------------------------------------
void write_members_and_neighbors( FILE *f, Cluster *cluster ) {
	if( !cluster->members.empty() || !cluster->neighbors.empty() ) {
		AgentIdVector all( cluster->members.size() + cluster->neighbors.size() );

		merge( cluster->members.begin(), cluster->members.end(),
			   cluster->neighbors.begin(), cluster->neighbors.end(),
			   all.begin() );
		
		fprintf( f, "cluster %d (%lu elts) : ", cluster->id, all.size() );
		itfor( AgentIdVector, all, it ) {
			fprintf( f, "%d ", *it );
		}
		fprintf( f, "\n" );
	}
}

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION get_results_dir
// ---
// --------------------------------------------------------------------------------
char *get_results_dir( const char *subdir ) {
	char reldir[1024];
	sprintf( reldir, "qt_clust/%s", subdir );
	return get_run_path( reldir );
}

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION get_results_path
// ---
// --------------------------------------------------------------------------------
char *get_results_path( const char *subdir, const char *type ) {
	char *dir = get_results_dir( subdir );

	char buf[1024];
	sprintf( buf, "%s/%s.txt", dir, type );
	free( dir );

	return strdup( buf );
}

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION write_results
// ---
// --------------------------------------------------------------------------------
void write_results( ClusterVector &clusters ) {
	char subdir[1024];
	sprintf( subdir, "m%df%g", cliParms.clusterPartitionModulus, cliParms.threshFact );

	{
		char cmd[1024 * 4];
		sprintf( cmd, "mkdir -p %s", get_results_dir(subdir) );
		errif( 0 != system(cmd), "Failed executing '%s'\n", cmd );
	}

#define FOPEN(HANDLE,TYPE)												\
	FILE *HANDLE = fopen(get_results_path(subdir,TYPE),"w");			\
	errif(HANDLE==NULL, "%s\n", get_results_path(subdir,TYPE));

	{
		FOPEN(f, "threshFact");

		fprintf( f, "%f\n", cliParms.threshFact );

		fclose(f);
	}

	{
		FOPEN(f, "members");

		itfor( ClusterVector, clusters, it ) {
			write_members( f, *it );
		}

		fclose(f);
	}

	{
		FOPEN(f, "members_neighbors");

		itfor( ClusterVector, clusters, it ) {
			write_members_and_neighbors( f, *it );
		}

		fclose(f);
	}

	{
		FOPEN(f, "neighbors");

		itfor( ClusterVector, clusters, it ) {
			write_neighbors( f, *it );
		}

		fclose(f);
	}

#undef FOPEN
}

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION load_genome
// ---
// --- Load genome for single agent from file.
// ---
// --------------------------------------------------------------------------------
unsigned char *load_genome( AgentId id ) {
	char *dir_genome = get_run_path( "genome/agents" );
    char path_genome[1024];
    sprintf(path_genome, "%s/genome_%d.txt", dir_genome, id);

    FILE *fp = fopen(path_genome, "r");
	errif( !fp, "Unable to open file \"%s\"\n", path_genome);

	unsigned char *genome = new unsigned char[GENES];

    char* s; int i;
    for (i=0;  i< GENES; i++) {
        s = readline(fp);
        genome[i] = (unsigned char) atoi(s);
    }

    fclose(fp);
	free( dir_genome );

	return genome;
}

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION get_genome_size
// ---
// --------------------------------------------------------------------------------
int get_genome_size() {
	char *lastline = read_last_nonblank_line( get_run_path("genome/meta/geneindex.txt") );
	stringstream sin( lastline );
	int offset = -1;
	sin >> offset;

	assert( offset >= 0 );

	return offset + 1;
}

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION get_agent_ids
// ---
// --------------------------------------------------------------------------------
AgentIdVector *get_agent_ids() {
	const char *path_genomes = get_run_path("genome/agents");

	DIR *dir = opendir( path_genomes );
	errif( !dir, "Failed opening dir %s\n", path_genomes );
	
	AgentIdVector *ids = new AgentIdVector();

	dirent *ent;
	while( ent = readdir(dir) ) {
		if( 0 == strncmp(ent->d_name, "genome_", 7) ) {
			AgentId id = atoi( ent->d_name + 7 );
			ids->push_back( (int)ids->size() + 1 );
		}
	}

	closedir( dir );

	return ids;
}

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION define_partitions
// ---
// --------------------------------------------------------------------------------
void define_partitions(	PopulationPartition **out_clusterPartition,
						PopulationPartition **out_neighborPartition ) {
	AgentIdVector *ids_global = get_agent_ids();
	AgentIdVector *ids_cluster = new AgentIdVector();
	AgentIdVector *ids_neighbor = new AgentIdVector();

	for( int i = 0; i < ids_global->size(); i++ ) {
		if( (i % cliParms.clusterPartitionModulus) == 0 ) {
			ids_cluster->push_back( ids_global->at(i) );
		} else {
			ids_neighbor->push_back( ids_global->at(i) );
		}
	}

	*out_clusterPartition = new PopulationPartition( ids_cluster,
													 true,
													 false );
	*out_neighborPartition = new PopulationPartition( ids_neighbor,
													  false,
													  true );

	delete ids_global;
}

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION create_distance_metrics
// ---
// --------------------------------------------------------------------------------
struct DistanceMetrics {
	float **deltaCache;
};

DistanceMetrics create_distance_metrics( PopulationPartitionVector &partitions ) {
	GeneEntropyCalculator entropyCalculator;
	GeneStdDev2Calculator stddev2Calculator;

	itfor( PopulationPartitionVector, partitions, it ) {
		PopulationPartition *partition = *it;

		itfor( AgentIdVector, partition->members, it ) {
			AgentId id = *it;
			unsigned char *genome = partition->getGenomeById( id );

			entropyCalculator.addGenome( genome );
			stddev2Calculator.addGenome( genome );

			partition->endGenomeStatisticsComputation( id );
		}
	}

	float *entropy = entropyCalculator.getResult();
	float *stddev2 = stddev2Calculator.getResult();

	DistanceMetrics result = {NULL};

	THRESH = 0;
    for( int i=0; i<GENES; i++ )
        THRESH += entropy[i];
    THRESH *= cliParms.threshFact;

	result.deltaCache = create_distance_deltaCache( entropy, stddev2 );

	delete entropy;
	delete stddev2;

	return result;
}

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION create_sorted
// ---
// --------------------------------------------------------------------------------
template< typename T >
T *create_sorted( int *order, T *unsorted, int len ) {
	T *sorted = new T[len];

	for( int i = 0; i < len; i++ ) {
		sorted[i] = unsorted[order[i]];
	}

	delete unsorted;

	return sorted;
}

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION sort_genes
// ---
// --------------------------------------------------------------------------------
void sort_genes( PopulationPartition *population, float **out_ents, float **out_stddev2 ) {
#if SORT_GENES
	// TODO: this code is out of date. note that we must also sort the delta cache.
		int sortedGeneOrder[GENES];

		printf("sorting genes by entropy...\n");
		{
			double startTime = hirestime();

			for( i = 0; i < GENES; i++ ) {
				sortedGeneOrder[i] = i;
			}
			struct local {
				static int compareEnt( const void *p1, const void *p2 ) {
					int i1 = *(int*)p1;
					int i2 = *(int*)p2;

					float diff = ents[i2] - ents[i1];

					return diff == 0
						? 0
						: diff < 0
						? -1
						: 1;
				}
			};
			qsort( sortedGeneOrder, GENES, sizeof(int), local::compareEnt );

			for( i = 0; i < POPSIZE; i++ ) {
				unsigned char *sorted = create_sorted_genome( sortedGeneOrder, allGenomes[i] );
				delete allGenomes[i];
				allGenomes[i] = sorted;
			}

			double endTime = hirestime();
			printf( "sort genome time=%f seconds\n", endTime - startTime );
		}
#endif
}

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION compute_clusters
// ---
// --------------------------------------------------------------------------------
void compute_clusters() {
	GENES = get_genome_size();

	// ---
	// --- DEFINE PARTITIONS
	// ---
    STAGE("defining partitions...\n");

	PopulationPartition *clusterPartition;
	PopulationPartition *neighborPartition;

	define_partitions( &clusterPartition, &neighborPartition );

	PopulationPartitionVector partitions;
	partitions.push_back( clusterPartition );
	partitions.push_back( neighborPartition );

	// ---
    // --- CREATE DISTANCE METRICS
	// ---
    STAGE("creating distance metrics...\n");

	DistanceMetrics distanceMetrics = create_distance_metrics( partitions );

    printf("THRESH: %f\n", THRESH); 

	// ---
	// --- SORT GENES
	// ---
	STAGE( "sorting genes...\n" );

	//sort_genes( globalPartition, &ents, &stddev2 );

	// ---
	// --- CREATE CLUSTERS
	// ---
	STAGE( "creating clusters...\n" );

	bool neighbors = !neighborPartition->members.empty();

	ClusterVector clusters; 
	create_clusters( distanceMetrics.deltaCache,
					 clusterPartition,
					 clusters );

	if( neighbors ) {
		// ---
		// --- CREATE CENTROIDS
		// ---
		STAGE( "creating centroids...\n" );

		create_centroids( distanceMetrics.deltaCache,
						  clusterPartition,
						  clusters );

		printf( "NOT PURGING CLUSTER GENOMES!\n" );
		/*
		itfor( PopulationPartitionVector, partitions, it ) {
			(*it)->endCentroidsComputation();
		}
		*/

		// ---
		// --- FIND NEIGHBORS
		// ---
		STAGE( "finding neighbors...\n" );

		AgentIdVector orphans;

		for( int i = 0; i < 1; i++ ) {
			itfor( ClusterVector, clusters, it ) {
				(*it)->neighbors.clear();
			}
			orphans.clear();

			find_neighbors( distanceMetrics.deltaCache,
							clusterPartition,
							neighborPartition,
							clusters,
							orphans );
		}

		if( !orphans.empty() ) {
			// ---
			// --- CLUSTER ORPHANS
			// ---
			STAGE( "clustering orphans...\n" );

			PopulationPartition orphanPartition( new AgentIdVector(orphans),
												 false,
												 false );

			create_clusters( distanceMetrics.deltaCache,
							 &orphanPartition,
							 clusters );
		}
	}

	// ---
	// --- WRITE RESULTS
	// ---
	STAGE( "writing results...\n" );

	write_results( clusters );

#if SLEEP_AT_END
	fprintf( stderr, "sleeping...\n" );
	while(true) sleep(1);
#endif
}

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION parse_clusters
// ---
// --------------------------------------------------------------------------------
class ParsedCluster {
public:
	ParsedCluster( ClusterId _id, AgentIdVector _members )
		: id(_id)
		, members(_members)
	{}

	ClusterId id;
	AgentIdVector members;
};

typedef vector<ParsedCluster> ParsedClusterVector;
typedef vector<ParsedClusterVector> ParsedClusterMatrix;

void parse_clusters( const char *path, ParsedClusterVector &clusters ) {
	errif( !is_regular_file(path), "Invalid file: %s\n", path );
		
	ifstream fin( path );
	errif( !fin.is_open(), "Failed opening file %s\n", path );

	const int BUF_SIZE = 1024 * 1024 * 64; // 64 MB line buffer should be plenty
	char *buf = new char[ BUF_SIZE ];

	set<ClusterId> clusterIds;

	while( fin ) {
		fin.getline( buf, BUF_SIZE );
		assert( fin.gcount() < BUF_SIZE );

		if( fin.gcount() > 0 ) {
			istringstream lin( buf );
			string token;

			lin >> token;
			if( token == "cluster" ) {
				ClusterId id;
				lin >> id;

				errif( clusterIds.find(id) != clusterIds.end(), "Found duplicate cluster ID (%d) in %s.\n", id, path );
				clusterIds.insert( id );
					
				// parse "(N"
				lin >> token;
				int nmembers = atoi( token.c_str() + 1 );

				// skip "elts)" , ":"
				lin >> token; lin >> token;
					
				AgentIdVector members;

				while( lin ) {
					AgentId agent = 0;
					lin >> agent;
					assert( !lin.fail() );
					assert( agent );

					members.push_back( agent );

					while( (' ' == lin.peek()) || ('\n' == lin.peek()) ) {
						lin.ignore( 1 );
					}
				}

				assert( nmembers == members.size() );
				clusters.push_back( ParsedCluster(id, members) );
			}
		}
	}

	delete buf;
}

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION load_clusters
// ---
// --------------------------------------------------------------------------------

struct LoadedClusters {
	PopulationPartitionVector *partitions;
	ClusterMatrix *clusterMatrix;
};

LoadedClusters load_clusters( bool singlePartition,
							  const char **subdirs, int nsubdirs ) {

	errif( !singlePartition, "Need to implement support for multiple partitions\n" );
	
	LoadedClusters result;
	result.partitions = new PopulationPartitionVector();
	result.clusterMatrix = new ClusterMatrix();

	ParsedClusterMatrix parsedClusters;

	cout << "Parsing cluster files..." << endl;
	for( int i = 0; i < nsubdirs; i++ ) {
		parsedClusters.push_back( ParsedClusterVector() );
		parse_clusters( get_results_path(subdirs[i], "members_neighbors"), parsedClusters.back() );
	}

	cout << "Creating partition..." << endl;
	AgentIdVector *ids;
	{
		// Create set of all agent IDs from all files.
		AgentIdSet ids_all;
		itfor( ParsedClusterMatrix, parsedClusters, it_vector ) {
			itfor( ParsedClusterVector, *it_vector, it_parsedCluster ) {
				itfor( AgentIdVector, it_parsedCluster->members, it_member ) {
					ids_all.insert( *it_member );
				}
			}
		}

		// Copy to vector
		ids = new AgentIdVector( ids_all.begin(), ids_all.end() );
	}

	PopulationPartition *partition = new PopulationPartition(ids, true, true);
	result.partitions->push_back( partition );

	cout << "Creating clusters..." << endl;

	itfor( ParsedClusterMatrix, parsedClusters, it_vector ) {
		ClusterVector *clusters = new ClusterVector();
		result.clusterMatrix->push_back( clusters );

		itfor( ParsedClusterVector, *it_vector, it_parsedCluster ) {
			Cluster *cluster = new Cluster( partition,
											it_parsedCluster->id,
											new AgentIdVector(it_parsedCluster->members) );
			clusters->push_back( cluster );
		}

	}

	return result;
}

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION load_parms
// ---
// --------------------------------------------------------------------------------
void load_parms( const char **subdirs, int nsubdirs ) {
	GENES = get_genome_size();
	cout << "GENES: " << GENES << endl;

	for( int i = 0; i < nsubdirs; i++ ) {
		FILE *f = fopen( get_results_path(subdirs[i], "threshFact"), "r" );
		errif( f == NULL, "Cannot load threshFact file for %s\n", subdirs[i] );
		float threshFact = atof( readline(f) );
		fclose(f);
		
		if( i > 0 ) {
			if( threshFact != cliParms.threshFact ) {
				err( "Cannot use multiple threshFact values.\n" );
			}
		} else {
			cliParms.threshFact = threshFact;
		}
	}

	cout << "THRESH_FACT: " << cliParms.threshFact << endl;
}

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION util__compareCentroids
// ---
// --------------------------------------------------------------------------------
void util__compareCentroids( const char *subdir_a, const char *subdir_b ) {
	const char *subdirs[] = {subdir_a, subdir_b};
	LoadedClusters loadedClusters = load_clusters( true, subdirs, 2 );

	load_parms( subdirs, 2 );

	cout << "Computing distance metrics..." << endl;

	DistanceMetrics distanceMetrics = create_distance_metrics( *(loadedClusters.partitions) );
	cout << "THRESH: " << THRESH << endl;

	cout << "Creating centroids..." << endl;

	itfor( ClusterMatrix, (*loadedClusters.clusterMatrix), it_clusters ) {
		ClusterVector *clusters = *it_clusters;
		itfor( ClusterVector, (*clusters), it_cluster ) {
			Cluster *cluster = *it_cluster;
			cluster->createCentroid();
		}
	}

	cout << "Calculating centroid distances..." << endl;

	ClusterVector &clusters_a = *(loadedClusters.clusterMatrix->at(0));
	ClusterVector &clusters_b =  *(loadedClusters.clusterMatrix->at(1));

	typedef pair<Cluster *, float> ClusterDistance;
	typedef vector<ClusterDistance> DistanceVector;
	typedef vector<pair<Cluster *, DistanceVector *> > ClusterDistances;

	ClusterDistances clusterDistances;

	itfor( ClusterVector, clusters_a, it_cluster_a ) {
		DistanceVector *distances = new DistanceVector();

		itfor( ClusterVector, clusters_b, it_cluster_b ) {
			float dist = compute_distance( distanceMetrics.deltaCache,
										   (*it_cluster_a)->centroidGenome,
										   (*it_cluster_b)->centroidGenome );

			distances->push_back( make_pair(*it_cluster_b, dist) );
		}

		struct local {
			static bool compare( const ClusterDistance &x, const ClusterDistance &y ) {
				return x.second < y.second;
			}
		};

		stable_sort( distances->begin(), distances->end(), local::compare );

		clusterDistances.push_back( make_pair(*it_cluster_a, distances) );
	}

	int nclusters =
		cliParms.nclusters == -1
			? clusterDistances.size()
			: min(cliParms.nclusters, (int)clusterDistances.size());

	int i = 0;
	itfor( ClusterDistances, clusterDistances, it ) {
		Cluster *cluster_a = it->first;
		ClusterDistance &best = it->second->front();
		Cluster *cluster_b = best.first;
		float dist = best.second;

		cout << "\t" << cluster_a->id << " --> " << cluster_b->id << " = " << dist << endl;

		if( ++i >= nclusters ) {
			break;
		}
	}
}

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION util__checkThresh
// ---
// --------------------------------------------------------------------------------
void util__checkThresh( const char *subdir ) {
	const char *subdirs[] = {subdir};

	LoadedClusters loadedClusters = load_clusters( true, subdirs, 1 );
	PopulationPartition *partition = loadedClusters.partitions->front();

	load_parms( subdirs, 1 );

	cout << "Computing distance metrics..." << endl;

	DistanceMetrics distanceMetrics = create_distance_metrics( *(loadedClusters.partitions) );
	cout << "THRESH: " << THRESH << endl;

	cout << "Analyzing thresh for clusters..." << endl;


	itfor( ClusterMatrix, (*loadedClusters.clusterMatrix), it_clusters ) {
		ClusterVector *clusters = *it_clusters;

		int nclusters =
			cliParms.nclusters == -1
			? clusters->size()
			: min(cliParms.nclusters, (int)clusters->size());

		for( int icluster = 0; icluster < nclusters; icluster++ ) {
			Cluster *cluster = clusters->at( icluster );

			int nexceedThresh = 0;
			int total = 0;
			const int N = cluster->members.size();

			for( int i = 0; i < N; i++ ) {
				#pragma omp parallel for
				for( int j = i+1; j < N; j++ ) {
					AgentId id_i = cluster->members[i];
					AgentId id_j = cluster->members[j];

					float dist = compute_distance( distanceMetrics.deltaCache,
												   partition->getGenomeById(id_i),
												   partition->getGenomeById(id_j) );

					if( dist > THRESH ) {
						#pragma omp atomic
						nexceedThresh++;
					}
					#pragma omp atomic
					total++;
				}
			}

			printf( "\t%d (n=%lu): Nd=%d Nd>THRESH=%d (%.2f%%)\n",
					cluster->id, cluster->members.size(),
					total, nexceedThresh, 100.0f * nexceedThresh / total );
		}
	}
}

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION util__centroidDists
// ---
// --------------------------------------------------------------------------------
void util__centroidDists( const char *subdir ) {
	const char *subdirs[] = {subdir};

	LoadedClusters loadedClusters = load_clusters( true, subdirs, 1 );
	PopulationPartition *partition = loadedClusters.partitions->front();

	load_parms( subdirs, 1 );

	cout << "Computing distance metrics..." << endl;

	DistanceMetrics distanceMetrics = create_distance_metrics( *(loadedClusters.partitions) );
	cout << "THRESH: " << THRESH << endl;

	cout << "Creating centroids..." << endl;

	ClusterVector &clusters = *(loadedClusters.clusterMatrix->at(0));
	
	itfor( ClusterVector, clusters, it ) {
		(*it)->createCentroid();
	}

	int nclusters = cliParms.nclusters == -1 ? (int)clusters.size() : min(cliParms.nclusters, (int)clusters.size());

	for( int i = 0; i < nclusters; i++ ) {
		for( int j = i+1; j < nclusters; j++ ) {
			Cluster *icluster = clusters[i];
			Cluster *jcluster = clusters[j];

			float dist = compute_distance( distanceMetrics.deltaCache,
										   icluster->centroidGenome,
										   jcluster->centroidGenome );

			cout << "\t" << icluster->id << " --> " << jcluster->id << " = " << dist << endl;
		}
	}
}

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION main
// ---
// --------------------------------------------------------------------------------
int main(int argc, char *argv[]) {
	if( argc == 1 ) {
		usage();
	}

	string mode = argv[1];

	argc--;
	argv++;

	if( mode == "cluster" ) {
		int opt;
		char *endptr;

		while( (opt = getopt(argc, argv, "m:f:")) != -1 ) {
			switch(opt) {
			case 'm': {
				char *endptr;
				cliParms.clusterPartitionModulus = strtol( optarg, &endptr, 10 );
				if( *endptr ) {
					err( "Invalid -m value -- expecting int.\n" );
				} else if( cliParms.clusterPartitionModulus < 1 ) {
					err( "Invalid -m value -- must be >= 1.\n" );
				}
			} break;
			case 'f': {
				char *endptr;
				cliParms.threshFact = strtof( optarg, &endptr );
				if( *endptr ) {
					err( "Invalid -f value -- expecting float.\n" );
				} else if( cliParms.threshFact <= 0 ) {
					err( "Invalid -f value -- must be > 0.\n" );
				}
			} break;
			default:
				exit(1);
			}
		}

		if( optind < argc ) {
			cliParms.path_run = argv[optind++];
		}

		if( optind < argc ) {
			err( "Unexpected arg '%s'\n", argv[optind] );
		}

		compute_clusters();
	} else {
		int opt;

		while( (opt = getopt(argc, argv, "n:")) != -1 ) {
			switch(opt) {
			case 'n': {
				char *endptr;
				cliParms.nclusters = strtol( optarg, &endptr, 10 );
				if( *endptr ) {
					err( "Invalid -n value -- expecting int.\n" );
				} else if( cliParms.clusterPartitionModulus < 1 ) {
					err( "Invalid -n value -- must be >= 1.\n" );
				}
			} break;
			default:
				exit(1);
			}
		}

		if( mode == "compareCentroids" ) {
			if( optind >= argc ) err( "Missing subdir_A\n" );
			const char *subdir_a = argv[optind++];

			if( optind >= argc ) err( "Missing subdir_B\n" );
			const char *subdir_b = argv[optind++];

			if( optind < argc ) {
				cliParms.path_run = argv[optind++];
			}

			if( optind < argc ) err( "Unpexected arg '%s'\n", argv[optind] );

			util__compareCentroids( subdir_a, subdir_b );
		} else if( mode == "checkThresh" ) {
			if( optind >= argc ) err( "Missing subdir\n" );
			const char *subdir = argv[optind++];

			if( optind < argc ) {
				cliParms.path_run = argv[optind++];
			}

			if( optind < argc ) err( "Unpexected arg '%s'\n", argv[optind] );

			util__checkThresh( subdir );
		} else if( mode == "centroidDists" ) {
			if( optind >= argc ) err( "Missing subdir\n" );
			const char *subdir = argv[optind++];

			if( optind < argc ) {
				cliParms.path_run = argv[optind++];
			}

			if( optind < argc ) err( "Unpexected arg '%s'\n", argv[optind] );

			util__centroidDists( subdir );
		} else {
			err( "Unknown mode '%s'\n", mode.c_str() );
		}
	}

    return 0;
}
