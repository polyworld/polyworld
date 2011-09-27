// Cluster C code goes in polyworld/tools/cluster
// Cluster animation code goes in polyworld/scripts

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "readline.h"
#include <omp.h>
#include <unistd.h>

#include <set>

#include <sys/time.h>
static double hirestime( void )
{
	struct timeval tv;
	gettimeofday( &tv, NULL );

	return (double)tv.tv_sec + (double)tv.tv_usec/1000000.0;
}

// iterator for loop
#define itfor(TYPE,CONT,IT)						\
	for(TYPE::iterator							\
			IT = (CONT).begin(),				\
			IT##_end = (CONT).end();			\
		IT != IT##_end;							\
		++IT)

// const iterator for loop
#define citfor(TYPE,CONT,IT)					\
	for(TYPE::const_iterator					\
			IT = (CONT).begin(),				\
			IT##_end = (CONT).end();			\
		IT != IT##_end;							\
		++IT)


// TODO: Make pathname an argument
// TODO: Make constants GENES and POPSIZE determined from run directory
//       Implies dynamic allocation of G, std2, dists, etc
// TODO: Write cluster info to a file (in run/cluster/<std.dev.>)

#define SLEEP_AT_END false
#define MAIN_TEST false
#define REDUCE_THRESH false

// TODO: Number of genes needs to be determined from the run directory
#define GENES 3000
// TODO: Get population size from the run directory
#define POPSIZE 5000
#define OFFSET 1 

// Debug flags
#define PRINT_ELTS 1
#define DEBUG 0
#define NUM_BINS 16

typedef int AgentId;
typedef std::set<AgentId> AgentIdSet;

class AgentIdArray {
public:
	AgentIdArray( const AgentIdSet &set ) {
		sorted = true;
		numIds = set.size();
		ids = new AgentId[ numIds ];

		int i = 0;
		citfor( AgentIdSet, set, it ) {
			ids[i++] = *it;
		}
	}

	AgentIdArray( const AgentId *buffer, size_t nelements ) {
		sorted = false;
		numIds = nelements;
		ids = new AgentId[ numIds ];

		memcpy( ids, buffer, numIds * sizeof(AgentId) );
	}

	~AgentIdArray() {
		delete ids;
	}

	inline AgentId operator[]( int index ) {
		assert( (index >= 0) && (index < numIds) );

		ensureSorted();

		return ids[index];
	}

	inline bool contains( AgentId id ) {
		ensureSorted();

		return NULL != bsearch( &id, ids, numIds, sizeof(AgentId), compareId );
	}

	inline size_t size() const {
		return numIds;
	}

	inline bool empty() const {
		return numIds == 0;
	}

	inline bool intersects( AgentIdArray &other ) {
		if( this == &other )
			return true;

		if( other.size() < this->size() ) {
			return other.intersects( *this );
		} else {
			for( int i = 0; i < numIds; i++ )
				if( other.contains(ids[i]) )
					return true;

			return false;
		}
	}

private:
	inline void ensureSorted() {
		if( !sorted ) {
			qsort( ids, numIds, sizeof(AgentId), compareId );
			sorted = true;
		}
	}

	static int compareId( const void *_x, const void *_y )
	{
		AgentId x = *(AgentId *)_x;
		AgentId y = *(AgentId *)_y;

		return x - y;
	}

	bool sorted;
	size_t numIds;
	AgentId *ids;
};

template< typename T >
class List {
public:
	struct Node {
		T data;
		int prev;
		int next;
	};
public:

	List( int capacity ) {
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

	~List() {
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

int ids[POPSIZE];
int clusterId = 0;

unsigned char G[POPSIZE][GENES];

float std2[GENES];
float **dists;
float ents[GENES];
float THRESH;

inline float get_distance( AgentId x, AgentId y ) {
	return dists[x][y];
}

float compute_distance(int i, int j) {
	int tmp; float sum = 0;
    unsigned char* x = G[i]; unsigned char* y = G[j];
	for (int gene = 0; gene < GENES; gene++) {
        tmp = (x[gene] > y[gene] ? x[gene] - y[gene] : y[gene] - x[gene]);
        tmp *= tmp;

        if (tmp)
            sum += (ents[gene] * tmp) / std2[gene];
	}
	return sum;
}

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

void print_cluster( AgentIdArray &agents )
{
    int i;

	printf("cluster %d (%lu elts)", clusterId, agents.size());
    if (PRINT_ELTS) {
        printf(" : ");
		for( int i = 0; i < agents.size(); i++ ) {
            printf("%d ", ids[ agents[i] ] );
        }
    }
    printf("\n");
}

struct MaxDist {
	AgentId id;
	float dist;
};

AgentIdArray *build_cluster( AgentId startAgent, const AgentIdSet &allAgents ) {
	AgentId clusterAgents[ allAgents.size() ];
	size_t numClusterAgents = 0;
#define ADD_CLUSTER_AGENT( ID ) clusterAgents[numClusterAgents++] = ID;

	ADD_CLUSTER_AGENT( startAgent );

	if( allAgents.size() > 1 ) {
		List<MaxDist> max_dists( allAgents.size() - 1 );
		{
			MaxDist *node = max_dists.head();
			citfor( AgentIdSet, allAgents, it ) {
				AgentId id = *it;

				if( id != startAgent ) {
					node->id = id;
					node->dist = 0;

					node = max_dists.next( node );
				}
			}
		}	

		AgentId lastPick = startAgent;

		if (DEBUG) printf("%dC ", lastPick);

		while( !max_dists.empty() ) {
			float pickDist = THRESH + 1;
			MaxDist *pick = NULL;

			for( MaxDist *node = max_dists.head(); node != NULL; node = max_dists.next( node ) ) {
				if( node->dist <= THRESH ) {
					float newDist = get_distance( lastPick, node->id );
					if (DEBUG) printf("%d -> %d: %f (cur: %f) \n", lastPick, node->id, newDist, node->dist);
					if( newDist > node->dist ) node->dist = newDist;
					if (DEBUG) printf("%d -> %d: %f (cur: %f) \n", startAgent, node->id, newDist, node->dist);
				}

				if( node->dist > THRESH ) {
					max_dists.remove( node );
				} else if( node->dist < pickDist ) {
					pick = node;
					pickDist = pick->dist;
				}
			}

			if( (pick != NULL) && (pick->dist <= THRESH) ) {
				if(DEBUG) printf("  pick=(%d,%f)\n", pick->id, pick->dist);
				lastPick = pick->id;
				ADD_CLUSTER_AGENT( pick->id );
				max_dists.remove( pick );
			}
		}
	}

	AgentIdArray *result = new AgentIdArray( clusterAgents, numClusterAgents );

	if (DEBUG) printf("%dC | (len %lu)\n", startAgent, result->size());

	return result;
}

struct Cluster {
	AgentId id;
	AgentIdArray *members;
};

void cluster_all( AgentIdSet &agents ) {

    if (DEBUG) printf("beginning cluster %d. genome size: %lu\n", clusterId, agents.size());

	List<Cluster> clusters( agents.size() );
	{
		Cluster *cluster = clusters.head();
		citfor( AgentIdSet, agents, it ) {
			cluster->id = *it;
			cluster = clusters.next( cluster );
		}
	}

	Cluster *nextBigCluster = NULL;

#define UPDATE_NEXT_BIG_CLUSTER( NODE )									\
	if( (nextBigCluster == NULL)										\
		|| ((NODE)->members->size() > nextBigCluster->members->size())	\
		|| (((NODE)->members->size() == nextBigCluster->members->size()) && (NODE)->id < nextBigCluster->id) ) \
	{																	\
		nextBigCluster = NODE;											\
	}

	#pragma omp parallel for schedule(dynamic)
	for( int i = 0; i < agents.size(); i++ ) {
		Cluster *cluster = clusters[i];

		cluster->members = build_cluster( cluster->id, agents );

		#pragma omp critical( cluster_all_nextBigCluster )
		{
			UPDATE_NEXT_BIG_CLUSTER( cluster );
		}
	}

	while( !clusters.empty() && !agents.empty() ) {
		Cluster *bigCluster = nextBigCluster;
		nextBigCluster = NULL;

        if (DEBUG) printf("selecting cluster %d (%lu/%lu agts)\n", 
						  bigCluster->id, bigCluster->members->size(), agents.size());
        if (PRINT_ELTS) print_cluster( *bigCluster->members );

		// remove cluster members from agents needing processing
		for( int i = 0; i < bigCluster->members->size(); i++ ) {
			agents.erase( (*bigCluster->members)[i] );
		}

		clusterId++;

		clusters.remove( bigCluster );
    
		// reduce candidate pool
		if( !clusters.empty() ) {
			Cluster *cluster = clusters.head();

			#pragma omp parallel
			{
				Cluster *cluster_threadLocal = NULL;
				
				do {
					#pragma omp critical( cluster_all_cluster )
					{
						cluster_threadLocal = cluster;
						if( cluster != NULL ) {
							cluster = clusters.next( cluster );
						}
					}

					if( cluster_threadLocal != NULL ) {
						if( cluster_threadLocal->members->intersects(*bigCluster->members) ) {
							#pragma omp critical( cluster_all_clusters )
							{
								clusters.remove( cluster_threadLocal );
								delete cluster_threadLocal->members;
							}
						} else {
							#pragma omp critical( cluster_all_nextBigCluster )
							{
								UPDATE_NEXT_BIG_CLUSTER( cluster_threadLocal );
							}
						}
					}
				} while( cluster_threadLocal != NULL );
			}
		}

		delete bigCluster->members;

        //if (clusters_head != NULL) printf("numCan: %lu\n", clusters.size());
        if (DEBUG) printf("POP: %lu\n", agents.size());
	}

	// is it legal for this to be non-empty afterwards? if so, we at least
	// need to delete any remaining elements.
	assert( clusters.empty() );
    printf("done with clustering\n");
}

void load_genome(int id, int ind) {
    FILE *fp;
    char filename[100];
	// TODO: Make pathname an argument
    sprintf(filename, "genomes/genome_%d.txt", id);

    fp = fopen(filename, "r");
    if (!fp) {
    	printf("Unable to open file \"%s\"\n", filename);
    	exit(1);
    }


    char* s; int i;
    for (i=0;  i< GENES; i++) {
        s = readline(fp);
        G[ind][i] = (unsigned char) atoi(s);
        ids[ind] = id;
    }

    fclose(fp);
}

int *mean(int SIZE) {
    int i, j;

    static int avg[GENES];
    for (j=0; j<GENES; j++)
        avg[j] = 0;

    for (i=0; i<SIZE; i++)
        for (j=0; j<GENES; j++)
            avg[j] += G[i][j];

    for (i=0; i < GENES; i++)
        avg[i] = avg[i] / SIZE;

    return avg;
}

float *stddev(int SIZE, int avg[GENES]) {
    int i, j;

    static float std[GENES];
    for (j=0; j<GENES; j++)
        std[j] = 0;

    for (i=0; i<SIZE; i++)
        for (j=0; j<GENES; j++)
            std[j] += (G[i][j] - avg[j]) * (G[i][j] - avg[j]);

    for (i=0; i < GENES; i++)
        std[i] = sqrt(std[i] / SIZE);

    //print_float_vector(std, GENES);
    return std;
}

void calc_ents() {
    int i, j; float p;
    int bins[NUM_BINS];
   
    // calculate entropy for each gene separately
    for (j=0; j<GENES; j++) {
        // reset bins
        for (i=0; i<NUM_BINS; i++)
            bins[i] = 0;

        //bin gene results
        for (i=0; i<POPSIZE; i++)
            bins[G[i][j] / NUM_BINS] += 1;

        // print_vector(bins, 256);

        // calculate entropy
        for (i=0; i<NUM_BINS; i++) {
            if (bins[i] != 0) {
                p = (float) bins[i] / POPSIZE;
                ents[j] += (-p * (log(p) / log(NUM_BINS)));
            }
        }    

        ents[j] = 1-ents[j];
    }
}

#if !MAIN_TEST
int main(int argc, char *argv[]) {
    int POP = POPSIZE;
	int  i, j;
    float THRESH_FACT = 2.25;
    if (argc == 2) THRESH_FACT = atof(argv[1]);
    
    printf("loading genomes...\n");
    // load genomes
    int *genome;
	for (i = 0; i < POP; i++) {
        if (DEBUG && !(i % 100)) printf("Loading genome %d\n", i);
        load_genome(i+OFFSET, i);
        // printf("loading %d (into %d)\n", i+OFFSET,i);
	}

    // calculate population statistics
    printf("calculating statistics...\n");
    calc_ents();
    //print_float_vector(ents, GENES);


    int *avg = mean(POP);
    float *std = stddev(POP, avg);
    for (i =0; i < GENES; i++)
        std2[i] = pow(std[i],2);
    //print_vector(avg, GENES);
    //print_float_vector(std2, GENES);

    // set threshold
    for (i=0; i<GENES; i++)
        THRESH += ents[i];
    THRESH *= THRESH_FACT;
#if REDUCE_THRESH
	printf("REDUCING THRESH!!!\n");
	THRESH *= .91;
#endif
    printf("THRESH: %f\n", THRESH); 

    // calculate distances
	dists = new float*[POPSIZE];
	for( int i = 0; i < POPSIZE; i++ ) {
		dists[i] = new float[POPSIZE];
	}

    printf("calculating distances...\n");
    float d; float* D;
    for (i = 0; i < POPSIZE; i++) {
        D = dists[i];
        
        #pragma omp parallel for shared(D, i, dists) private(d, j)
	    for (j = i+1; j < POPSIZE; j++) {
            d = compute_distance(i, j);
            D[j] = d;
            //if (DEBUG) printf("%d -> %d: %f\n", i, j, d);
        }
        
        D[i] = 0.0;
        
        #pragma omp parallel for shared(D, i, dists) private(d, j)
        for (j = 0; j < i; j++) {
            d = dists[j][i];
            D[j] = d;
            //if (DEBUG) printf("%d -> %d: %f\n", i, j, d);
        }

    }
	/*
    if (DEBUG)
        for (i = 0; i < POPSIZE; i++)
            print_float_vector(dists[i], POPSIZE);
	*/
   
    // cluster until all elts are clustered
    printf("starting clusters...\n");
	double startTime = hirestime();

	AgentIdSet agents;
	for( int i = 0; i < POP; i++) {
		agents.insert( i );
	}

    while( !agents.empty() ) {
        cluster_all( agents );
        printf("to cluster: %lu\n", agents.size() );
    }

	double endTime = hirestime();
	printf( "clustering time=%f seconds\n", endTime - startTime );

#if SLEEP_AT_END
	fprintf( stderr, "sleeping...\n" );
	while(true) sleep(1);
#endif

    return POP;
}
#else
int main(int argc, char *argv[]) {
	{
		AgentId xids[] = { 3, 1, 2 };
		AgentIdArray x( xids, 3 );

		assert( x[0] == 1 );
		assert( x[1] == 2 );
		assert( x[2] == 3 );

		AgentId yids[] = { 5, 6, 4 };
		AgentIdArray y( yids, 3 );

		assert( y[0] == 4 );
		assert( y[1] == 5 );
		assert( y[2] == 6 );

		assert( x.intersects(x) );
		assert( y.intersects(y) );

		assert( !x.intersects(y) );
		assert( !y.intersects(x) );

		AgentId zids[] = { 6, 3 };
		AgentIdArray z( zids, 2 );

		assert( z[0] == 3 );
		assert( z[1] == 6 );

		assert( z.intersects(x) );
		assert( x.intersects(z) );
		assert( z.intersects(y) );
		assert( y.intersects(z) );
	}

	{
		AgentIdSet set;

		for( int i = 0; i < 10; i++ )
			set.insert( i );

		AgentIdArray x( set );
		printf("--- x ---\n");
		for( int i = 0; i < x.size(); i++ ) {
			printf( "[%d] = %d\n", i, x[i] );
		}

		set.clear();
		for( int i = 10; i < 20; i++ ) 
			set.insert( i );

		AgentIdArray y( set );
		printf("--- y ---\n");
		for( int i = 0; i < y.size(); i++ ) {
			printf( "[%d] = %d\n", i, y[i] );
		}

		set.clear();
		set.insert( 9 );
		set.insert( 10 );

		AgentIdArray z( set );
		printf("--- z ---\n");
		for( int i = 0; i < z.size(); i++ ) {
			printf( "[%d] = %d\n", i, z[i] );
		}

		assert( !x.intersects(y) );
		assert( !y.intersects(x) );
		assert( x.intersects(x) );

		assert( x.intersects(z) );
		assert( z.intersects(x) );

		assert( y.intersects(z) );
		assert( z.intersects(y) );
	}

}
#endif
