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
#include <zlib.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include <algorithm>
#include <iostream>
#include <fstream>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace std;


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

#define NUM_BINS 16 // TODO: binning code is broken. only works cuz 16*16=256

// ---
// --- Debug flags
// ---
#define VERBOSE false
#define DEBUG false
#define SANITY_CHECKS false
// If true, user must press enter to proceed through major stages of analysis.
// This provides an opportunity to do things like look at process memory usage.
#define PROMPT_STAGES false

#define GENOME_CACHE_FILE_PATH "genomeCache.bin"

// STL iterator for loop
#define itfor(TYPE,CONT,IT)						\
	for(TYPE::iterator							\
			IT = (CONT).begin(),				\
			IT##_end = (CONT).end();			\
		IT != IT##_end;							\
		++IT)

#define err( msg... ) {fprintf(stderr, msg); exit(1);}

// if condition is true, then print message and exit
#define errif( condition, msg... )										\
	if(condition) {														\
		fprintf(stderr, "%s:%d Failed condition \"%s\"\n", __FILE__, __LINE__, #condition); \
		fprintf(stderr, msg);											\
		exit(1);														\
	}

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
// === COMMAND-LINE PARAMETERS
// ===
// ================================================================================
// ================================================================================
// ================================================================================
// ================================================================================

struct CliParms {
	string mode;
	string subdir;
	int clusterNumber;
	int certaintyPower;
	int clusterPartitionModulus;
	float threshFact;
	int genomeCacheCapacity;
	int clusterStrideDivisor;
	int neighborCandidateStride;
	enum {
		NA_MEASURE_MEMBERS,
		NA_CLUSTER
	} neighborAlgorithm;
	const char *neighborAlgorithmName;
	const char *path_run;
	int nclusters;

	CliParms() {
		certaintyPower = 1;
		clusterPartitionModulus = 1;
		threshFact = 2.125;
		genomeCacheCapacity = -1;
		clusterStrideDivisor = -1;
		neighborCandidateStride = 1;
		neighborAlgorithm = NA_MEASURE_MEMBERS;
		neighborAlgorithmName = "measureMembers";
		path_run = "./run";
		nclusters = -1;
	}
} cliParms;

// ================================================================================
// ================================================================================
// ================================================================================
// ================================================================================
// ===
// === MAIN
// ===
// ================================================================================
// ================================================================================
// ================================================================================
// ================================================================================

int main(int argc, char *argv[]) {
	// ---
	// --- FUNCTION PROTOTYPES
	// ---
	void usage();
	void compute_clusters();
	void util__compareCentroids( const char *subdir_a, const char *subdir_b );
	void util__checkThresh( const char *subdir );
	void util__centroidDists( const char *subdir );
	void util__zscore( const char *subdir );
	void util__metabolism( const char *subdir );
	void util__ancestry( const char *subdir );
	void util__genedist( const char *subdir );

	if( argc == 1 ) {
		usage();
	}

	cliParms.mode = argv[1];

	// copy 0 to 1 so error messages from getopt show program name
	argv[1] = argv[0];
	// shift forward one in args to let getopt process args after mode.
	argc--;
	argv++;

	if( (cliParms.mode == "cluster") || (cliParms.mode == "subcluster") ) {
		while( true ) {
			static struct option long_options[] = {
				{"certaintyPower", 1, 0, 'p'},
				{"clusterPartitionModulus", 1, 0, 'm'},
				{"clusterStrideDivisor", 1, 0, 'd'},
				{"threshFact", 1, 0, 'f'},
				{"genomeCacheCapacity", 1, 0, 'g'},
				{"neighborCandidateStride", 1, 0, 's'},
				{"neighborAlgorithm", 1, 0, 'n'},
				{0, 0, 0, 0}
			};
			int option_index = 0;

			int opt = getopt_long(argc, argv, "p:m:d:f:g:s:n:",
								  long_options, &option_index);
			if( opt == -1 )
				break;

			switch(opt) {
			case 'p': {
				char *endptr;
				cliParms.certaintyPower = strtol( optarg, &endptr, 10 );
				if( *endptr ) {
					err( "Invalid -p value -- expecting int.\n" );
				} else if( cliParms.certaintyPower < 1 ) {
					err( "Invalid -p value -- must be >= 1.\n" );
				}
			} break;
			case 'm': {
				char *endptr;
				cliParms.clusterPartitionModulus = strtol( optarg, &endptr, 10 );
				if( *endptr ) {
					err( "Invalid -m value -- expecting int.\n" );
				} else if( cliParms.clusterPartitionModulus < 1 ) {
					err( "Invalid -m value -- must be >= 1.\n" );
				}
			} break;
			case 'd': {
				char *endptr;
				cliParms.clusterStrideDivisor = strtol( optarg, &endptr, 10 );
				if( *endptr ) {
					err( "Invalid -d value -- expecting int.\n" );
				} else if( cliParms.clusterStrideDivisor == 0 ) {
					err( "Invalid -d value -- cannot be 0.\n" );
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
			case 'g': {
				char *endptr;
				cliParms.genomeCacheCapacity = strtol( optarg, &endptr, 10 );
				if( *endptr ) {
					err( "Invalid -g value -- expecting int.\n" );
				} else if( (cliParms.genomeCacheCapacity < -1) || (cliParms.genomeCacheCapacity == 0) ) {
					err( "Invalid -g value -- must be > 0 or -1.\n" );
				}
			} break;
			case 's': {
				char *endptr;
				cliParms.neighborCandidateStride = strtol( optarg, &endptr, 10 );
				if( *endptr ) {
					err( "Invalid -s value -- expecting int.\n" );
				} else if( cliParms.neighborCandidateStride < 1 ) {
					err( "Invalid -s value -- must be >= 1.\n" );
				}
			} break;
				
			case 'n': {
				string algname( optarg );

				if( algname == "measureNeighbors" ) {
					cliParms.neighborAlgorithm = CliParms::NA_MEASURE_MEMBERS;
				} else if( algname == "cluster" ) {
					cliParms.neighborAlgorithm = CliParms::NA_CLUSTER;
				} else {
					err( "Invalid -n value -- must be (measureNeighbors|cluster)\n" );
				}

				cliParms.neighborAlgorithmName = strdup( optarg );
			} break;
			default:
				exit(1);
			}
		}

		if( cliParms.mode == "subcluster" ) {
			if( optind == argc ) err( "missing subdir\n" );
			cliParms.subdir = argv[optind++];
			if( optind == argc ) err( "missing clusterNumber\n" );
			char *endptr;
			cliParms.clusterNumber = strtol( argv[optind++], &endptr, 10 );
			if( *endptr ) {
				err( "Invalid clusterNumber -- expecting int.\n" );
			} else if( cliParms.clusterNumber < 0 ) {
				err( "Invalid clusterNumber -- must be >= 0.\n" );
			}
		}

		if( optind < argc ) {
			cliParms.path_run = argv[optind++];
		}

		if( optind < argc ) {
			err( "Unexpected arg '%s'\n", argv[optind] );
		}

		// ---
		// --- PERFORM CLUSTERING
		// ---
		compute_clusters();
	} else {
		int opt;

		while( (opt = getopt(argc, argv, "n:g:")) != -1 ) {
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
			case 'g': {
				char *endptr;
				cliParms.genomeCacheCapacity = strtol( optarg, &endptr, 10 );
				if( *endptr ) {
					err( "Invalid -g value -- expecting int.\n" );
				} else if( cliParms.genomeCacheCapacity < 1 ) {
					err( "Invalid -g value -- must be >= 1.\n" );
				}
			} break;
			default:
				exit(1);
			}
		}

		if( cliParms.mode == "compareCentroids" ) {
			if( optind >= argc ) err( "Missing subdir_A\n" );
			const char *subdir_a = argv[optind++];

			if( optind >= argc ) err( "Missing subdir_B\n" );
			const char *subdir_b = argv[optind++];

			if( optind < argc ) {
				cliParms.path_run = argv[optind++];
			}

			if( optind < argc ) err( "Unpexected arg '%s'\n", argv[optind] );

			// ---
			// --- COMPARE CENTROIDS
			// ---
			util__compareCentroids( subdir_a, subdir_b );
		} else if( cliParms.mode == "checkThresh" ) {
			if( optind >= argc ) err( "Missing subdir\n" );
			const char *subdir = argv[optind++];

			if( optind < argc ) {
				cliParms.path_run = argv[optind++];
			}

			if( optind < argc ) err( "Unpexected arg '%s'\n", argv[optind] );

			// ---
			// --- CHECK THRESH
			// ---
			util__checkThresh( subdir );
		} else if( cliParms.mode == "centroidDists" ) {
			if( optind >= argc ) err( "Missing subdir\n" );
			const char *subdir = argv[optind++];

			if( optind < argc ) {
				cliParms.path_run = argv[optind++];
			}

			if( optind < argc ) err( "Unpexected arg '%s'\n", argv[optind] );

			// ---
			// --- CENTROID DISTS
			// ---
			util__centroidDists( subdir );
		} else if( cliParms.mode == "zscore" ) {
			if( optind >= argc ) err( "Missing subdir\n" );
			const char *subdir = argv[optind++];

			if( optind < argc ) {
				cliParms.path_run = argv[optind++];
			}

			if( optind < argc ) err( "Unpexected arg '%s'\n", argv[optind] );

			// ---
			// --- ZSCORE
			// ---
			util__zscore( subdir );
		} else if( cliParms.mode == "metabolism" ) {
			if( optind >= argc ) err( "Missing subdir\n" );
			const char *subdir = argv[optind++];

			if( optind < argc ) {
				cliParms.path_run = argv[optind++];
			}

			if( optind < argc ) err( "Unpexected arg '%s'\n", argv[optind] );

			// ---
			// --- METABOLISM
			// ---
			util__metabolism( subdir );
		} else if( cliParms.mode == "ancestry" ) {
			if( optind >= argc ) err( "Missing subdir\n" );
			const char *subdir = argv[optind++];

			if( optind < argc ) {
				cliParms.path_run = argv[optind++];
			}

			if( optind < argc ) err( "Unpexected arg '%s'\n", argv[optind] );

			// ---
			// --- ANCESTRY
			// ---
			util__ancestry( subdir );
		} else if( cliParms.mode == "genedist" ) {
			if( optind >= argc ) err( "Missing subdir\n" );
			const char *subdir = argv[optind++];

			if( optind < argc ) {
				cliParms.path_run = argv[optind++];
			}

			if( optind < argc ) err( "Unpexected arg '%s'\n", argv[optind] );

			// ---
			// --- GENEDIST
			// ---
			util__genedist( subdir );
		} else {
			err( "Unknown mode '%s'\n", cliParms.mode.c_str() );
		}
	}

    return 0;
}

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION usage
// ---
// --------------------------------------------------------------------------------
void usage() {
	#define p(x...) fprintf( stderr, "%s\n", x )

	p( "usage:" );
	p( "" );
	p( "qt_clust cluster [opt...] [run]" );
	p( "qt_clust subcluster [opt...] subdir clusterNumber [run]" );
	p( "   Perform cluster analysis." );
	p( "" );
	p( "   -p,--certaintyPower arg" );
	p( "        Use certainty^N in distance calculations.");
	p( "" );
	p( "   -m,--clusterPartitionModulus arg" );
	p( "        Modulus used for determining which agents are placed in the cluster partition." );
	p( "      A value of 2 will result in 50% of agents being clustered, 3 will be 33%, etc." ); 
	p( "" );
	p( "   -d,--clusterStrideDivisor arg" );
	p( "        Controls how many agents remaining to be clustered will be used as start agents." );
	p( "      The set of agents will be stepped through with a value of:" );
	p( "           max( 1, int(remainingAgents.size() / stride_divisor) )" );
	p( "      A reasonable value is 1000 - 3000." );
	p( "" );
	p( "   -f,--threshFact arg" );
	p( "        Set the value of THRESH_FACT." );
	p( "" );
	p( "   -g,--genomeCacheCapacity arg" );
	p( "        Set the max number of genomes that will be cached in RAM." );
	p( "" );
	p( "   -s,--neighborCandidateStride arg" );
	p( "        Sets the increment value for stepping through a cluster's members when determining" );
	p( "      if a potential neighbor violates THRESH with a cluster. A reasonable value is 2." );
	p( "" );
	p( "   -n,--neighborAlgorithm arg" );
	p( "        Specifies algorithm of neighboring pass. Values values are 'measureNeighbors' and" );
	p( "      'cluster'. Default is 'measureNeighbors'." );
	p( "" );
	p( "" );
	p( "qt_clust compareCentroids [-n max_clusters] [-g genomeCacheCapacity] subdir_A subdir_B [run]" );
	p( "   Compute the distance between cluster centroids from two cluster files." );
	p( "" );
	p( "qt_clust checkThresh [-n max_clusters] [-g genomeCacheCapacity] subdir [run]" );
	p( "   Analyze how well clusters conform to THRESH." );
	p( "" );
	p( "qt_clust centroidDists [-n max_clusters] [-g genomeCacheCapacity] subdir [run]" );
	p( "   Compute distance between centroids of all clusters." );
	p( "" );
	p( "qt_clust zscore [-n max_clusters] [-g genomeCacheCapacity] subdir [run]" );
	p( "   Compute zscore between clusters." );
	p( "" );
	p( "qt_clust metabolism [-n max_clusters] [-g genomeCacheCapacity] subdir [run]" );
	p( "   Show metabolsim counts for each cluster." );
	p( "" );
	p( "qt_clust ancestry [-n max_clusters] [-g genomeCacheCapacity] subdir [run]" );
	p( "   Show ancestral clusters for each cluster." );
	p( "" );
	p( "qt_clust genedist [-n max_clusters] [-g genomeCacheCapacity] subdir [run]" );
	p( "   Determine which genes contribute most to distance between clusters." );
	p( "" );
	p( "" );
	p( "Examples:" );
	p( "" );
	p( "   qt_clust cluster -m 10 ./run" );
	p( "" );
	p( "   qt_clust compareCentroids -n 10 m1f2.125 m10f2.125" );

	#undef p

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
int GENESN4 = 0;


// ================================================================================
// ================================================================================
// ================================================================================
// ================================================================================
// ===
// === AgentId/AgentIndex Types
// ===
// ================================================================================
// ================================================================================
// ================================================================================
// ================================================================================

// ---
// --- AgentId is the ID of the agent in the simulation being analyzed.
// ---
typedef int AgentId;
typedef vector<AgentId> AgentIdVector;
typedef set<AgentId> AgentIdSet;

// ---
// --- AgentIndex is the 0-based index of the agent within a PopulationPartition.
// ---
typedef int AgentIndex;
typedef vector<AgentIndex> AgentIndexVector;
typedef set<AgentIndex> AgentIndexSet;

typedef int ClusterId;
typedef set<ClusterId> ClusterIdSet;

// ================================================================================
// ================================================================================
// ================================================================================
// ================================================================================
// ===
// === MISC UTIL FUNCTIONS
// ===
// ================================================================================
// ================================================================================
// ================================================================================
// ================================================================================

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
	errif( !is_regular_file(path), "File does not exist: %s\n", path );

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
// --- FUNCTION load_genome
// ---
// --- Load genome for single agent from file.
// ---
// --------------------------------------------------------------------------------
void load_genome( AgentId id, unsigned char *genome ) {
	char *dir_genome = get_run_path( "genome/agents" );
    char path_genome[1024];
    sprintf(path_genome, "%s/genome_%d.txt", dir_genome, id);

	// 4 bytes per gene ("xxx\n") + fudge.
	char buf[4 * GENES + 128];
	size_t nread;

    FILE *fp = fopen(path_genome, "r");
	if( fp ) {
		nread = fread( buf, sizeof(char), sizeof(buf), fp );
		fclose(fp);
	} else {
		sprintf( path_genome, "%s/genome_%d.txt.gz", dir_genome, id );

		gzFile gfp = gzopen( path_genome, "r" );
		errif( !gfp, "Unable to open file \"%s\"\n", path_genome);

		int rc = gzread( gfp, buf, sizeof(buf) );
		errif( rc <= 0, "Failed reading gzip file %s\n", path_genome );

		nread = rc;

		gzclose(gfp);
	}

	errif( (nread == 0) || (nread >= sizeof(buf)),
		   "Unreasonable number of bytes read: %lu\n", nread );

	size_t i = 0;
	int igene = 0;
	while( i < nread ) {
		if( isdigit(buf[i]) ) {
			genome[igene++] = atoi( buf + i );
			do {
				i++;
			} while( isdigit(buf[i]) );
		} else {
			i++;
		}
	}

	errif( igene != GENES, "Unexpected number of genes for agent %d: %d\n", id, igene );

	free( dir_genome );
}

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION set_genome_size
// ---
// --------------------------------------------------------------------------------
int GENES_DIST = 0;
void set_genome_size() {
	char *lastline = read_last_nonblank_line( get_run_path("genome/meta/geneindex.txt") );
	stringstream sin( lastline );
	int offset = -1;
	sin >> offset;

	assert( offset >= 0 );

	GENES = offset + 1;
	GENESN4 = int(GENES / 4) * 4;
	if( GENESN4 < GENES ) GENESN4 -= 4;
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

				assert( nmembers == (int)members.size() );
				clusters.push_back( ParsedCluster(id, members) );
			}
		}
	}

	delete [] buf;
}

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION get_agent_ids
// ---
// --------------------------------------------------------------------------------
AgentIdVector *get_agent_ids() {
	AgentIdVector *ids = new AgentIdVector();

	if( cliParms.mode == "subcluster" ) {
		char *path_clusters = get_results_path( cliParms.subdir.c_str(), "members_neighbors" );
		ParsedClusterVector clusters;

		parse_clusters( path_clusters, clusters );
		bool found = false;

		itfor( ParsedClusterVector, clusters, it ) {
			if( it->id == cliParms.clusterNumber ) {
				ids->resize( it->members.size() );
				copy( it->members.begin(), it->members.end(), ids->begin() );
				found = true;
				break;
			}
		}

		errif( !found, "Failed finding cluster %d\n", cliParms.clusterNumber );
		
	} else {
		const char *path_genomes = get_run_path("genome/agents");

		DIR *dir = opendir( path_genomes );
		errif( !dir, "Failed opening dir %s\n", path_genomes );
	
		dirent *ent;
		while( NULL != (ent = readdir(dir)) ) {
			if( 0 == strncmp(ent->d_name, "genome_", 7) ) {
				AgentId id = atoi( ent->d_name + 7 );
				ids->push_back( id );
			}
		}

		closedir( dir );
	}

	sort( ids->begin(), ids->end() );

	return ids;
}

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION print_vector
// ---
// --------------------------------------------------------------------------------
void print_vector(int v[], int size) {
	int i;
	printf("| [");
	for (i =0; i < size; i++) {
		printf("%3d ", v[i]);
	}
	printf("]\n");
}

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION print_float_vector
// ---
// --------------------------------------------------------------------------------
void print_float_vector(float v[], int size) {
	int i;
	printf("(");
	for (i =0; i < size; i++) {
		printf(" %f ", v[i]);
	}
	printf(")\n");
}

// ================================================================================
// ================================================================================
// ================================================================================
// ================================================================================
// ===
// === CLASS ListBuffer
// ===
// === A lightweight doubly-linked list, where all nodes reside in single heap
// === buffer allocated on instantiation. Links are achieved with int rather than
// === pointers to save RAM.
// ================================================================================
// ================================================================================
// ================================================================================
// ================================================================================

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

// ================================================================================
// ================================================================================
// ================================================================================
// ================================================================================
// ===
// === GENOME CACHE
// ===
// ================================================================================
// ================================================================================
// ================================================================================
// ================================================================================

namespace __GenomeCache {
	typedef list<class GenomeCacheSlot *> GenomeCacheSlotList;

	// --------------------------------------------------------------------------------
	// ---
	// --- CLASS GenomeCacheSlot
	// ---
	// --------------------------------------------------------------------------------
	class GenomeCacheSlot {
	public:
		GenomeCacheSlot( AgentId id, const GenomeCacheSlotList::iterator &_it_unreferencedAllocatedSlots )
			: it_unreferencedAllocatedSlots( _it_unreferencedAllocatedSlots )
		{
			this->id = id;
			genes = NULL;
			nreferences = 0;
			offset_cacheFile = -1;
			sorted = false;
		}

		AgentId id;
		unsigned char *genes;
		int nreferences;
		long offset_cacheFile;
		bool sorted;
		GenomeCacheSlotList::iterator it_unreferencedAllocatedSlots;
	};

	typedef map<AgentId, GenomeCacheSlot> GenomeCacheSlotLookup;
}

// --------------------------------------------------------------------------------
// ---
// --- CLASS GenomeRef
// ---
// --------------------------------------------------------------------------------
class GenomeRef {
public:
	GenomeRef( const GenomeRef &other ) {
		// do not allow copies
		assert( false );
	}

	GenomeRef( class GenomeCache *cache, __GenomeCache::GenomeCacheSlot *slot ) {
		this->cache = cache;
		this->slot = slot;
	}

	~GenomeRef(); // function implementation below class GenomeCache

	inline unsigned char *genes() const {
		return slot->genes;
	}

private:
	GenomeCache *cache;
	friend class GenomeCache;
	__GenomeCache::GenomeCacheSlot *slot;
};

// --------------------------------------------------------------------------------
// ---
// --- CLASS GenomeCache
// ---
// --------------------------------------------------------------------------------
class GenomeCache {
public:
	// --------------------------------------------------------------------------------
	// --- FUNCTION ctor
	// --------------------------------------------------------------------------------
	GenomeCache( int capacity, AgentIdVector *agents )	{
		using namespace __GenomeCache;

		this->capacity = capacity;
		nallocated = 0;
		sorted = false;
		nmisses = 0;
		f_cacheFile = NULL;

		// Create a slot for each genome.
		itfor( AgentIdVector, *agents, it ) {
			slotLookup.insert( make_pair(*it, GenomeCacheSlot(*it, unreferencedAllocatedSlots.end())) );
		}
	}

	// --------------------------------------------------------------------------------
	// --- FUNCTION dtor
	// --------------------------------------------------------------------------------
	~GenomeCache() {
		if( f_cacheFile ) {
			fclose( f_cacheFile );
			remove( GENOME_CACHE_FILE_PATH );
		}
	}

	// --------------------------------------------------------------------------------
	// --- FUNCTION get
	// --------------------------------------------------------------------------------
	inline GenomeRef get( AgentId id ) {
		return GenomeRef( this, refslot(id) );
	}

	// --------------------------------------------------------------------------------
	// --- FUNCTION refslot
	// --------------------------------------------------------------------------------
	__GenomeCache::GenomeCacheSlot *refslot( AgentId id ) {
		using namespace __GenomeCache;

		GenomeCacheSlot *slot = &( slotLookup.find(id)->second );
		bool loadGenes = false;

		#pragma omp critical( slot_references )
		{
			// Check if slot is allocated.
			if( slot->genes != NULL ) {
				add_reference( slot );
			} else {
				loadGenes = true;
			}
		}

		if( loadGenes ) {
			#pragma omp critical( slot_load )
			{
				if( slot->genes == NULL ) {
					if( nallocated < capacity ) {
						nallocated++;
						unsigned char *genome = new unsigned char[GENES];
						load( slot, genome );
						#pragma omp critical( slot_references )
						{
							slot->genes = genome;
							if( sorted && !slot->sorted ) {
								sort( slot );
							}
							add_reference( slot );
						}
					} else {
						nmisses++;
						unsigned char *genes;
						GenomeCacheSlot *deallocatedSlot;

						#pragma omp critical( slot_references )
						{
							deallocate_slot( &deallocatedSlot, &genes );
						}

						if( deallocatedSlot->offset_cacheFile == -1 ) {
							store_cacheFile_slot( deallocatedSlot, genes );
						}

						load( slot, genes );

						#pragma omp critical( slot_references )
						{
							slot->genes = genes;
							if( sorted && !slot->sorted ) {
								sort( slot );
							}
							add_reference( slot );
						}
					}
				} else {
					#pragma omp critical( slot_references )
					{
						add_reference( slot );
					}
				}
			}
		}

		return slot;
	}

	// --------------------------------------------------------------------------------
	// --- FUNCTION unrefslot
	// --------------------------------------------------------------------------------
	inline void unrefslot( __GenomeCache::GenomeCacheSlot *slot ) {
		#pragma omp critical( slot_references )
		{
			slot->nreferences--;
			if( slot->nreferences == 0 ) {
				unreferencedAllocatedSlots.push_front( slot );
				slot->it_unreferencedAllocatedSlots = unreferencedAllocatedSlots.begin();
			}
		}
	}

	// --------------------------------------------------------------------------------
	// --- FUNCTION unrefslot
	// --------------------------------------------------------------------------------
	inline void unrefslot( AgentId id ) {
		unrefslot( &( slotLookup.find(id)->second ) );
	}

	// --------------------------------------------------------------------------------
	// --- FUNCTION sort
	// --------------------------------------------------------------------------------
	void sort( vector<int> &order ) {
		using namespace __GenomeCache;

		sorted = true;
		sortOrder = order;

		itfor( GenomeCacheSlotLookup, slotLookup, it ) {
			GenomeCacheSlot *slot = &(it->second);
			if( slot->genes ) {
				sort( slot );
			}
		}
	}

public:
	int nmisses;

private:
	// --------------------------------------------------------------------------------
	// --- FUNCTION add_reference
	// --------------------------------------------------------------------------------
	inline void add_reference( __GenomeCache::GenomeCacheSlot *slot ) {
		slot->nreferences++;
		if( (slot->nreferences == 1)
			&& (slot->it_unreferencedAllocatedSlots != unreferencedAllocatedSlots.end()) )
		{
			unreferencedAllocatedSlots.erase( slot->it_unreferencedAllocatedSlots );
		}
	}

	// --------------------------------------------------------------------------------
	// --- FUNCTION deallocate_slot
	// --------------------------------------------------------------------------------
	void deallocate_slot( __GenomeCache::GenomeCacheSlot **out_deallocatedSlot, unsigned char **out_genes ) {
		using namespace __GenomeCache;

		errif( unreferencedAllocatedSlots.empty(), "genome references exceed cache capacity!\n" );

		GenomeCacheSlot *deallocatedSlot = unreferencedAllocatedSlots.back();
		// remove from list.
		unreferencedAllocatedSlots.pop_back();
		deallocatedSlot->it_unreferencedAllocatedSlots = unreferencedAllocatedSlots.end();

		*out_genes = deallocatedSlot->genes;
		deallocatedSlot->genes = NULL;
		*out_deallocatedSlot = deallocatedSlot;
	}

	// --------------------------------------------------------------------------------
	// --- FUNCTION load
	// --------------------------------------------------------------------------------
	void load( __GenomeCache::GenomeCacheSlot *slot, unsigned char *genes ) {
		if( slot->offset_cacheFile != -1 ) {
			int rc;

			rc = fseek( f_cacheFile, slot->offset_cacheFile, SEEK_SET);
			errif( 0 != rc, "Failed seeking to cache slot (rc=%d)\n", rc );

			rc = fread( genes, 1, GENES, f_cacheFile);
			errif( rc != GENES,
				   "Failed reading cache slot, rc=%d, offset=%ld (%s)\n",
				   rc, slot->offset_cacheFile, strerror(errno) );
		} else {
			load_genome( slot->id, genes );
		}
	}

	// --------------------------------------------------------------------------------
	// --- FUNCTION store_cacheFile_slot
	// --------------------------------------------------------------------------------
	void store_cacheFile_slot( __GenomeCache::GenomeCacheSlot *slot, unsigned char *genes ) {
		if( f_cacheFile == NULL ) {
			errif( NULL == (f_cacheFile = fopen( GENOME_CACHE_FILE_PATH, "w+" )),
				   "Failed creating genome cache file\n" );
		}

		errif( 0 != fseek(f_cacheFile, 0, SEEK_END),
			   "Failed seeking end of genome cache.\n" );
		errif( -1 == (slot->offset_cacheFile = ftell(f_cacheFile)),
			   "Failed getting genome cache file position.\n" );

		errif( (size_t)GENES != fwrite(genes, 1, GENES, f_cacheFile),
			   "Failed writing genome cache slot.\n" );		
	}

	// --------------------------------------------------------------------------------
	// --- FUNCTION sort
	// --------------------------------------------------------------------------------
	void sort( __GenomeCache::GenomeCacheSlot *slot ) {
		assert( !slot->sorted );

		unsigned char buf[GENES];
		memcpy( buf, slot->genes, GENES );

		for( int i = 0; i < GENES; i++ ) {
			slot->genes[i] = buf[ sortOrder[i] ];
		}

		slot->sorted = true;
	}

private:

	FILE *f_cacheFile;
	int capacity;
	int nallocated;
	bool sorted;
	vector<int> sortOrder;
	__GenomeCache::GenomeCacheSlotLookup slotLookup;
	__GenomeCache::GenomeCacheSlotList unreferencedAllocatedSlots;
};

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION ~GenomeRef
// ---
// --------------------------------------------------------------------------------
GenomeRef::~GenomeRef() {
	cache->unrefslot( slot );
	slot = NULL;
}

// ================================================================================
// ================================================================================
// ================================================================================
// ================================================================================
// ===
// === CLASS PopulationPartition
// ===
// ================================================================================
// ================================================================================
// ================================================================================
// ================================================================================

class PopulationPartition {
public:
	PopulationPartition( AgentIdVector *_members,
						 GenomeCache *_genomeCache )
		: members(*_members)
		, genomeCache(_genomeCache)
	{
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
		delete &members;
	}

	inline GenomeRef getGenomeById( AgentId id ) {
		return genomeCache->get( id );
	}

	inline GenomeRef getGenomeByIndex( AgentIndex index ) {
		return genomeCache->get( getId(index) );
	}

	inline AgentId getId( AgentIndex index ) {
		return members[index];
	}

	inline AgentIndex getIndex( AgentId id ) {
		// perform binary search
		AgentIdVector::iterator it = lower_bound( members.begin(), members.end(), id );
		assert( *it == id );

		return std::distance( members.begin(), it );
	}

	inline AgentIdVector *createAgentIdVector( AgentIndex *indexes, int nindexes ) {
		AgentIdVector *result = new AgentIdVector(); // TODO constructor specifying capacity?

		for( int i = 0; i < nindexes; i++ ) {
			result->push_back( getId(indexes[i]) );
		}

		return result;
	}

public:
	AgentIdVector &members;
	GenomeCache *genomeCache;
};

typedef vector<PopulationPartition *> PopulationPartitionVector;

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION define_partitions
// ---
// --------------------------------------------------------------------------------
void define_partitions(	AgentIdVector *ids_global,
						GenomeCache *genomeCache,
						PopulationPartition **out_clusterPartition,
						PopulationPartition **out_neighborPartition ) {
	AgentIdVector *ids_cluster = new AgentIdVector();
	AgentIdVector *ids_neighbor = new AgentIdVector();

	for( int i = 0; i < (int)ids_global->size(); i++ ) {
		if( (i % cliParms.clusterPartitionModulus) == 0 ) {
			ids_cluster->push_back( ids_global->at(i) );
		} else {
			ids_neighbor->push_back( ids_global->at(i) );
		}
	}

	*out_clusterPartition = new PopulationPartition( ids_cluster, genomeCache );
	*out_neighborPartition = new PopulationPartition( ids_neighbor, genomeCache );
}


// ================================================================================
// ================================================================================
// ================================================================================
// ================================================================================
// ===
// === DISTANCE FUNCTIONS
// ===
// ================================================================================
// ================================================================================
// ================================================================================
// ================================================================================

// --------------------------------------------------------------------------------
// ---
// --- CLASS GeneDistanceDeltaCache
// ---
// --------------------------------------------------------------------------------
struct GeneDistanceDeltaCache {
	float deltaValue[256];
};

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION compute_distance
// ---
// --- Calculate distance between two genomes.
// ---
// --------------------------------------------------------------------------------
inline float compute_distance( GeneDistanceDeltaCache *deltaCache, unsigned char *x, unsigned char *y ) {
	float sum = 0;

	for( int gene = 0; gene < GENESN4; gene += 4 ) {
		sum += deltaCache[ gene+0 ].deltaValue[ abs(x[gene+0] - y[gene+0]) ];
		sum += deltaCache[ gene+1 ].deltaValue[ abs(x[gene+1] - y[gene+1]) ];
		sum += deltaCache[ gene+2 ].deltaValue[ abs(x[gene+2] - y[gene+2]) ];
		sum += deltaCache[ gene+3 ].deltaValue[ abs(x[gene+3] - y[gene+3]) ];
	}

	for( int gene = GENESN4; gene < GENES; gene++ ) {
		sum += deltaCache[ gene ].deltaValue[ abs(x[gene] - y[gene]) ];
	}

	return sum;
}

inline float compute_distance( GeneDistanceDeltaCache *deltaCache, const GenomeRef &x, const GenomeRef &y ) {
	return compute_distance( deltaCache, x.genes(), y.genes() );
}

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION create_distance_deltaCache
// ---
// --------------------------------------------------------------------------------
GeneDistanceDeltaCache *create_distance_deltaCache( float *certainty, float *stddev2 ) {
	GeneDistanceDeltaCache *deltaCache = new GeneDistanceDeltaCache[GENES];

	for( int igene = 0; igene < GENES; igene++ ) {
		for( int delta = 0; delta < 256; delta++ ) {
			float result;

			int tmp = delta;

			if (tmp) {
				tmp *= tmp;
				result = ( powf(certainty[igene], cliParms.certaintyPower) * tmp ) / stddev2[igene];
			} else {
				result = 0;
			}

			deltaCache[igene].deltaValue[delta] = result;
		}
	}

	return deltaCache;
}

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION sort_distance_deltaCache
// ---
// --------------------------------------------------------------------------------
void sort_distance_deltaCache( GeneDistanceDeltaCache *deltaCache, vector<int> &order ) {
	GeneDistanceDeltaCache buf[GENES];
	memcpy( buf, deltaCache, sizeof(buf) );

	for( int i = 0; i < GENES; i++ ) {
		deltaCache[i] = buf[ order[i] ];
	}
}

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION compute_distances
// ---
// --- Computes distances between index_i and [index_j, index_j_end), where result
// --- is placed in parameter <dists> like:
// ---
// ---    {dist(i,j), dist(i,j+1)... dist(i,j_end - 1)}
// ---
// --- Note this function's complexity is primarily due to minimizing misses in
// --- the CPU's cache.
// --------------------------------------------------------------------------------
void compute_distances( GeneDistanceDeltaCache *deltaCache,
						GenomeCache *genomeCache,
						AgentIdVector &ids,
						int index_i,
						int index_j,
						int index_j_end,
						float *dists ) {
	// Create a reference to i's genome so it won't be evicted from the cache.
	__GenomeCache::GenomeCacheSlot *sloti = genomeCache->refslot( ids[index_i] );
	unsigned char *genomei = sloti->genes;

	// Init state of result.
	memset( dists, 0, sizeof(float) * (index_j_end - index_j) );

	// Iterate through the genomes, operating on 32 at a time.
	// Note that the number 32 was found to do well on an i7 2600k, which
	// effectively has a 16k L1 cache for each hyper-thread. We can increase
	// this number in the future.
	for( int j = index_j; j < index_j_end; j += 32 ) {
		// Determine if we have a full batch of 32 remaining.
		int ngenomes_batch = min( 32, index_j_end - j );

		// For each of the genomes in the batch, create a reference to in the cache
		// so they aren't evicted from underneath us.
		__GenomeCache::GenomeCacheSlot *slots[ngenomes_batch];
		unsigned char *genomes[ngenomes_batch];
		for( int k = 0; k < ngenomes_batch; k++ ) {
			slots[k] = genomeCache->refslot( ids[j + k] );
			genomes[k] = slots[k]->genes;
		}

		// Compute distance of single gene, updating result state.
		#define genedist(GENOME_INDEX, GENE_INDEX)						\
			dists[GENOME_INDEX+(j-index_j)] +=							\
				deltaCache[GENE_INDEX].deltaValue[ abs(genomei[GENE_INDEX] - genomes[GENOME_INDEX][GENE_INDEX]) ];
		// Compute distance of 4 genes. Performance optimization.
		#define genedist4(GENOME_INDEX,GENE_INDEX)						\
			genedist(GENOME_INDEX, GENE_INDEX);							\
			genedist(GENOME_INDEX, GENE_INDEX+1);						\
			genedist(GENOME_INDEX, GENE_INDEX+2);						\
			genedist(GENOME_INDEX, GENE_INDEX+3);
						
		if( ngenomes_batch < 32 ) {
			// We don't have a full batch, so use a less efficient rolled loop.

			// Note that we want to iterate through genes in the outer loop, since
			// the size of the delta cache for a single gene is 1024 bytes. We want to
			// keep that data in CPU's L1 cache, and use it for several genomes before
			// moving onto the next gene.

			// This is an unrolled loop that operates on 4 genes at a time.
			for( int igene = 0; igene < GENESN4; igene +=4 ) {
				for( int k = 0; k < ngenomes_batch; k++ ) {
					genedist4(k,igene);
				}
			}

			// Less efficient loop that operates on 1 gene at a time.
			for( int igene = GENESN4; igene < GENES; igene++ ) {
				for( int k = 0; k < ngenomes_batch; k++ ) {
					genedist(k,igene);
				}
			}
		} else {
			// We're operating on a full batch. We use macros to unroll the loops operating on the genomes.

			// Note that we want to iterate through genes in the outer loop, since
			// the size of the delta cache for a single gene is 1024 bytes. We want to
			// keep that data in CPU's L1 cache, and use it for several genomes before
			// moving onto the next gene.

			// This is an unrolled loop that operates on 4 genes at a time.
			for( int igene = 0; igene < GENESN4; igene +=4 ) {
				#define genedist4_genome(GENOME_INDEX) genedist4(GENOME_INDEX,igene);
				#define genedist4_genome4(GENOME_INDEX)					\
					genedist4_genome(GENOME_INDEX);						\
					genedist4_genome(GENOME_INDEX+1);					\
					genedist4_genome(GENOME_INDEX+2);					\
					genedist4_genome(GENOME_INDEX+3);
				#define genedist4_genome16(GENOME_INDEX)				\
					genedist4_genome4(GENOME_INDEX);					\
					genedist4_genome4(GENOME_INDEX+4);					\
					genedist4_genome4(GENOME_INDEX+8);					\
					genedist4_genome4(GENOME_INDEX+12);

				// Compute distances for 32 genomes.
				genedist4_genome16(0);
				genedist4_genome16(16);
			}
			// Less efficient loop that operates on 1 gene at a time.
			for( int igene = GENESN4; igene < GENES; igene++ ) {
				#define genedist_genome(GENOME_INDEX) genedist(GENOME_INDEX,igene);
				#define genedist_genome4(GENOME_INDEX)					\
					genedist_genome(GENOME_INDEX);						\
					genedist_genome(GENOME_INDEX+1);					\
					genedist_genome(GENOME_INDEX+2);					\
					genedist_genome(GENOME_INDEX+3);
				#define genedist_genome16(GENOME_INDEX)					\
					genedist_genome4(GENOME_INDEX);						\
					genedist_genome4(GENOME_INDEX+4);					\
					genedist_genome4(GENOME_INDEX+8);					\
					genedist_genome4(GENOME_INDEX+12);

				// Compute distances for 32 genomes.
				genedist_genome16(0);
				genedist_genome16(16);
			}
		}

		// Release our references so the cache can evict them if necessary.
		for( int k = 0; k < ngenomes_batch; k++ ) {
			genomeCache->unrefslot( slots[k] );
		}
	}

	genomeCache->unrefslot( sloti );
}

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION create_distanceCache
// ---
// --------------------------------------------------------------------------------
float **create_distanceCache( GeneDistanceDeltaCache *deltaCache, PopulationPartition *partition ) {
	int numGenomes = partition->members.size();

	// allocate distance cache
	float **distanceCache = new float*[numGenomes];
	for( int i = 0; i < numGenomes; i++ ) {
		distanceCache[i] = new float[numGenomes];
	}

	#pragma omp parallel for schedule(dynamic)
	for( int i = 0; i < numGenomes; i++ ) {
		float *D = distanceCache[i];

		compute_distances( deltaCache,
						   partition->genomeCache,
						   partition->members,
						   i, i+1, numGenomes,
						   D + (i+1) );
	}

	#pragma omp parallel for
	for( int i = 0; i < numGenomes; i++ ) {
		float *D = distanceCache[i];

		D[i] = 0.0;
        
		for( int j = 0; j < i; j++ ) {
			float d = distanceCache[j][i];
			D[j] = d;
		}
	}

	return distanceCache;
}

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION dispose_distanceCache
// ---
// --------------------------------------------------------------------------------
void dispose_distanceCache( float **distanceCache, PopulationPartition *population ) {
	for( int i = 0; i < (int)population->members.size(); i++ ) {
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
// --- CLASS GeneCertaintyCalculator
// ---
// --------------------------------------------------------------------------------
class GeneCertaintyCalculator {
public:
	GeneCertaintyCalculator() {
		numGenomes = 0;

		bins = new int*[NUM_BINS];
		for( int i = 0; i < NUM_BINS; i++ ) {
			bins[i] = new int[GENES];
			memset( bins[i], 0, GENES * sizeof(int) );
		}
	}

	~GeneCertaintyCalculator() {
		for( int i = 0; i < NUM_BINS; i++ ) {
			delete bins[i];
		}
		delete bins;
	}

	void addGenome( GenomeRef &genome ) {
		unsigned char *genes = genome.genes();
		numGenomes++;

		for( int igene = 0; igene < GENES; igene++ ) {
			// TODO: this bin calculation only works because 16 * 16 = 256
			int bin = genes[igene] / NUM_BINS;
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

	void addGenome( GenomeRef &genome ) {
		unsigned char *genes = genome.genes();
		numGenomes++;

		for( int i = 0; i < GENES; i++ ) {
			sum[i] += genes[i];
			sum2[i] += genes[i] * genes[i];
		}
	}

	float *getMean() {
		float *mean = new float[GENES];
		for( int i = 0; i < GENES; i++ ) {
			mean[i] = sum[i] / numGenomes;
		}
		return mean;
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
// --- FUNCTION create_distance_metrics
// ---
// --------------------------------------------------------------------------------
struct DistanceMetrics {
	GeneDistanceDeltaCache *deltaCache;
	float *certainty;
	float *stddev2;
};

DistanceMetrics create_distance_metrics( PopulationPartitionVector &partitions ) {
	assert( GENES > 0 );

	double startTime = hirestime();

	GeneCertaintyCalculator certaintyCalculator;
	GeneStdDev2Calculator stddev2Calculator;

	itfor( PopulationPartitionVector, partitions, it ) {
		PopulationPartition *partition = *it;

		itfor( AgentIdVector, partition->members, it ) {
			AgentId id = *it;
			GenomeRef genome = partition->getGenomeById( id );

			certaintyCalculator.addGenome( genome );
			stddev2Calculator.addGenome( genome );
		}
	}

	DistanceMetrics result;

	result.certainty = certaintyCalculator.getResult();
	result.stddev2 = stddev2Calculator.getResult();

	THRESH = 0;
    for( int i=0; i<GENES; i++ )
        THRESH += powf( result.certainty[i], cliParms.certaintyPower );
    THRESH *= cliParms.threshFact;

	result.deltaCache = create_distance_deltaCache( result.certainty, result.stddev2 );

	double endTime = hirestime();
	printf( "distance metrics time=%f seconds\n", endTime - startTime );

	return result;
}

// ================================================================================
// ================================================================================
// ================================================================================
// ================================================================================
// ===
// === CLASS Cluster
// ===
// ================================================================================
// ================================================================================
// ================================================================================
// ================================================================================

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
			GenomeRef memberGenome = population->getGenomeById( *it );
			unsigned char *memberGenes = memberGenome.genes();
			
			for( int igene = 0; igene < GENES; igene++ ) {
				sum[igene] += memberGenes[igene];
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

	static bool sort__id_ascending( const Cluster *x, const Cluster *y ) {
		return x->id < y->id;
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
// --- FUNCTION create_centroids
// ---
// --------------------------------------------------------------------------------
void create_centroids( float **distance_deltaCache,
					   PopulationPartition *population,
					   ClusterVector &clusters ) {

	#pragma omp parallel for
	for( int i = 0; i < (int)clusters.size(); i++ ) {
		clusters[i]->createCentroid();
	}

#if SANITY_CHECKS
	itfor( ClusterVector, clusters, it_cluster ) {
		Cluster *cluster = *it_cluster;
		itfor( AgentIdVector, cluster->members, it_member ) {
			GenomeRef memberGenome = population->getGenomeById(*it_member);
			float dist = compute_distance(distance_deltaCache,
										  memberGenome.genes(),
										  cluster->centroidGenome);
			errif( dist > THRESH,
				   "cluster=%d, member=%d, nmembers=%lu, dist=%f, THRESH=%f\n",
				   cluster->id, *it_member, cluster->members.size(), dist, THRESH );
		}
	}
#endif
}


// ================================================================================
// ================================================================================
// ================================================================================
// ================================================================================
// ===
// === WRITE RESULTS
// ===
// ================================================================================
// ================================================================================
// ================================================================================
// ================================================================================

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
// --- FUNCTION write_results
// ---
// --------------------------------------------------------------------------------
void write_results( ClusterVector &clusters ) {
	char subdir_prefix[1024];
	if( cliParms.mode == "subcluster" ) {
		sprintf( subdir_prefix, "%s/cluster%d_",
				 cliParms.subdir.c_str(),
				 cliParms.clusterNumber );
	} else {
		subdir_prefix[0] = 0;
	}

	char subdir[1024 * 2];
	sprintf( subdir, "%sp%d_m%d_d%d_f%g_s%d_n%s",
			 subdir_prefix,
			 cliParms.certaintyPower,
			 cliParms.clusterPartitionModulus,
			 cliParms.clusterStrideDivisor,
			 cliParms.threshFact,
			 cliParms.neighborCandidateStride,
			 cliParms.neighborAlgorithmName );

	{
		char cmd[1024 * 4];
		sprintf( cmd, "mkdir -p %s", get_results_dir(subdir) );
		errif( 0 != system(cmd), "Failed executing '%s' (%s)\n", cmd, strerror(errno) );
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
		FOPEN(f, "thresh");

		fprintf( f, "%f\n", THRESH );

		fclose(f);
	}
	{
		FOPEN(f, "certaintyPower");

		fprintf( f, "%d\n", cliParms.certaintyPower );

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

	cout << "results written to " << get_results_dir(subdir) << endl;

#undef FOPEN
}


// ================================================================================
// ================================================================================
// ================================================================================
// ================================================================================
// ===
// === CLUSTERING FUNCTIONS
// ===
// ================================================================================
// ================================================================================
// ================================================================================
// ================================================================================

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION create_candidate_cluster
// ---
// --- Build a candidate cluster from a given starting agent.
// ---
// --------------------------------------------------------------------------------
namespace __create_candidate_cluster {
	struct MaxDist {
		AgentIndex index;
		float dist;
	};
}

AgentIdVector *create_candidate_cluster( float **distanceCache,
										 PopulationPartition *partition,
										 AgentIndex startAgent,
										 AgentIndexSet &allAgents ) {
	using namespace __create_candidate_cluster;

	AgentIndex clusterAgents[ allAgents.size() ];
	size_t numClusterAgents = 0;
#define ADD_CLUSTER_AGENT( ID ) clusterAgents[numClusterAgents++] = ID;

	ADD_CLUSTER_AGENT( startAgent );

	if( allAgents.size() > 1 ) {
		ListBuffer<MaxDist> max_dists( allAgents.size() - 1 );
		{
			MaxDist *node = max_dists.head();
			itfor( AgentIndexSet, allAgents, it ) {
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

	AgentIndexVector *result = partition->createAgentIdVector( clusterAgents, numClusterAgents );

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
						 PopulationPartition *partition,
						 AgentIndexSet &remainingAgents,
						 ClusterId clusterId,
						 int stride_divisor = -1 ) {

	AgentIdVector *biggestMembers = NULL;
	AgentIndex biggestStartAgent;

	AgentIndexSet::iterator it = remainingAgents.begin();
	AgentIndexSet::iterator it_end = remainingAgents.end();

	int stride = max( 1, int(remainingAgents.size() / stride_divisor) );
	
	#pragma omp parallel
	{
		AgentIndexSet::iterator it_threadLocal = it_end;

		do {
			#pragma omp critical( cluster_single__it )
			{
				it_threadLocal = it;
				if( it != it_end ) {
					for( int i = 0; (i < stride) && (it != it_end); i++ ) {
						++it;
					}
				}
			}

			if( it_threadLocal != it_end ) {
				AgentIndex startAgent = *it_threadLocal;
				AgentIdVector *members = create_candidate_cluster( distanceCache,
																   partition,
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

	sort( biggestMembers->begin(), biggestMembers->end() );

	Cluster *result = new Cluster( partition,
								   clusterId,
								   biggestMembers );

	return result;
}

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION create_clusters
// ---
// --- Assigns all agents in partition to a cluster.
// ---
// --------------------------------------------------------------------------------
void create_clusters( GeneDistanceDeltaCache *distance_deltaCache,
					  PopulationPartition *population,
					  ClusterVector &allClusters ) {
	printf("calculating distances...\n");
	float **distanceCache;
	{
		double startTime = hirestime();

		distanceCache = create_distanceCache( distance_deltaCache, population );

		double endTime = hirestime();
		printf( "distance time=%f seconds\n", endTime - startTime );
	}


	double startTime = hirestime();

	// ---
	// --- Create set of agent indexes to be clustered
	// ---
	AgentIndexSet remainingAgents;
	for( int i = 0; i < (int)population->members.size(); i++) {
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
										   clusters.size() + allClusters.size(),
										   cliParms.clusterStrideDivisor );
		clusters.push_back( cluster );

		// remove cluster members from agents needing processing
		for( int i = 0; i < (int)cluster->members.size(); i++ ) {
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
// --- FUNCTION find_valid_neighbors__measureNeighbors
// ---
// --------------------------------------------------------------------------------
void find_valid_neighbors__measureNeighbors( Cluster *cluster,
											 AgentIdVector &clusterNeighborCandidates,
											 GeneDistanceDeltaCache *distance_deltaCache,
											 PopulationPartition *neighborPartition ) {
	AgentIdSet validNeighbors( clusterNeighborCandidates.begin(), clusterNeighborCandidates.end() );

	typedef map<AgentId, AgentIdSet> ViolationMap;
	ViolationMap violations;

	typedef map<AgentId, double> DistMap;
	DistMap distsTotal;

	for( int i = 0; i < (int)clusterNeighborCandidates.size(); i++ ) {
		distsTotal[ clusterNeighborCandidates[i] ] = 0;
	}

	#pragma omp parallel for
	for( int i = 0; i < (int)clusterNeighborCandidates.size(); i++ ) {
		AgentId i_id = clusterNeighborCandidates[i];

		float dists[clusterNeighborCandidates.size() - (i+1)];

		compute_distances( distance_deltaCache,
						   neighborPartition->genomeCache,
						   clusterNeighborCandidates,
						   i, i+1, clusterNeighborCandidates.size(),
						   dists );

		for( int j = i + 1; j < (int)clusterNeighborCandidates.size(); j++ ) {
			AgentId j_id = clusterNeighborCandidates[j];
			float dist = dists[j - (i+1)];

			double &i_dist = distsTotal[i_id];
			#pragma omp atomic
			i_dist += dist;

			double &j_dist = distsTotal[j_id];
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
			if( distsTotal[first] < distsTotal[second] ) {
				validNeighbors.erase( second );
			} else {
				validNeighbors.erase( first );
			}
		}
	}

	cluster->neighbors.resize( validNeighbors.size() );
	copy( validNeighbors.begin(), validNeighbors.end(), cluster->neighbors.begin() );
}

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION find_valid_neighbors__cluster
// ---
// --------------------------------------------------------------------------------
void find_valid_neighbors__cluster( Cluster *cluster,
									AgentIdVector &clusterNeighborCandidates,
									GeneDistanceDeltaCache *distance_deltaCache,
									PopulationPartition *neighborPartition ) {
	AgentIdSet candidatesSet;
	for( size_t i = 0; i < clusterNeighborCandidates.size(); i++ ) {
		candidatesSet.insert( (int)i );
	}

	PopulationPartition partition( new AgentIdVector(clusterNeighborCandidates),
								   neighborPartition->genomeCache );

	float **distanceCache = create_distanceCache( distance_deltaCache, &partition );

	Cluster *neighborCluster = create_cluster( distanceCache,
											   &partition,
											   candidatesSet,
											   0,
											   cliParms.clusterStrideDivisor );
	
	cluster->neighbors.resize( neighborCluster->members.size() );
	copy( neighborCluster->members.begin(), neighborCluster->members.end(),
		  cluster->neighbors.begin() );	

	delete neighborCluster;
	dispose_distanceCache( distanceCache, &partition );
}

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION find_neighbors
// ---
// --------------------------------------------------------------------------------
void find_neighbors( GeneDistanceDeltaCache *distance_deltaCache,
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

	for( int icluster = 0; icluster < (int)clusters.size(); icluster++ ) {
		Cluster *cluster = clusters[icluster];

		neighborCandidatesVector.resize( neighborCandidates.size() );
		copy( neighborCandidates.begin(), neighborCandidates.end(), neighborCandidatesVector.begin() );

		// ---
		// --- Determine neighbor candidates for cluster by finding agents that don't violate THRESH
		// --- for any of cluster's members.
		// ---
		AgentIdVector clusterNeighborCandidates;

		#pragma omp parallel for
		for( int ineighbor = 0; ineighbor < (int)neighborCandidatesVector.size(); ineighbor++ ) {
			AgentId neighborId = neighborCandidatesVector[ ineighbor ];
			GenomeRef neighborGenome = neighborPartition->getGenomeById( neighborId );
			bool isNeighbor = true;

			for( int iclusterMember = 0;
				 isNeighbor && (iclusterMember < (int)cluster->members.size());
				 iclusterMember += cliParms.neighborCandidateStride )
			{
				AgentId clusterId = cluster->members[ iclusterMember ];
				GenomeRef clusterGenome = clusterPartition->getGenomeById( clusterId );
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
		if( clusterNeighborCandidates.size() > 1000 ) {
			printf("  cluster %d neighbor_candidates=%lu\n", cluster->id, clusterNeighborCandidates.size() );
		}
		switch( cliParms.neighborAlgorithm ) {
		case CliParms::NA_MEASURE_MEMBERS:
			find_valid_neighbors__measureNeighbors( cluster,
													clusterNeighborCandidates,
													distance_deltaCache,
													neighborPartition );
			break;
		case CliParms::NA_CLUSTER:
			find_valid_neighbors__cluster( cluster,
										   clusterNeighborCandidates,
										   distance_deltaCache,
										   neighborPartition );
			break;
		default:
			assert( false );
		}

		itfor( AgentIdVector, cluster->neighbors, it ) {
			neighborCandidates.erase( *it );
		}
	} // for each cluster

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
// --- FUNCTION sort_genes
// ---
// --------------------------------------------------------------------------------
namespace __sort_genes {
	class Sorter {
	public:
		float *certainty;
		float *stddev2;
		Sorter( float *certainty, float *stddev2 ) {this->certainty = certainty; this->stddev2=stddev2;}
		bool operator()( const int &a, const int &b ) {
			return certainty[a] > certainty[b];
		}
	};
};

void sort_genes( DistanceMetrics distanceMetrics, GenomeCache &genomeCache ) {
	using namespace __sort_genes;

	vector<int> &geneOrder = *(new vector<int>());
	for( int i = 0; i < GENES; i++ ) {
		geneOrder.push_back(i);
	}
	
	Sorter sorter( distanceMetrics.certainty, distanceMetrics.stddev2 );
	std::sort( geneOrder.begin(), geneOrder.end(), sorter );

	genomeCache.sort( geneOrder );
	sort_distance_deltaCache( distanceMetrics.deltaCache, geneOrder );
}

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION compute_clusters
// ---
// --------------------------------------------------------------------------------
void compute_clusters() {
	printf("RUN: %s\n", cliParms.path_run);

	set_genome_size();

    printf("GENES: %d\n", GENES); 

	AgentIdVector *ids_global = get_agent_ids();
	int genomeCacheCapacity = cliParms.genomeCacheCapacity;
	if( genomeCacheCapacity == -1 ) {
		genomeCacheCapacity = (int)ids_global->size();
	}
	printf( "POPSIZE: %lu\n", ids_global->size() );
	GenomeCache genomeCache( genomeCacheCapacity, ids_global );
	printf( "GENOME CACHE CAPACITY: %d\n", genomeCacheCapacity );

	// ---
	// --- DEFINE PARTITIONS
	// ---
    STAGE("defining partitions...\n");

	PopulationPartition *clusterPartition;
	PopulationPartition *neighborPartition;

	define_partitions( ids_global, &genomeCache, &clusterPartition, &neighborPartition );

	PopulationPartitionVector partitions;
	partitions.push_back( neighborPartition );
	// We want the cluster partition processed last in create_distance_metrics
	// so that its genomes are still in the genome cache for creating the distance cache.
	partitions.push_back( clusterPartition );

	printf( "NUM CLUSTERED AGENTS: %lu\n", clusterPartition->members.size() );
	printf( "NUM NEIGHBORED AGENTS: %lu\n", neighborPartition->members.size() );

	if( genomeCacheCapacity < (int)clusterPartition->members.size() ) {
		cerr << "Warning! GenomeCacheCapacity (-g) is less than number of clustered agents. Execution time will be poor." << endl;
	}

	// ---
    // --- CREATE DISTANCE METRICS
	// ---
    STAGE("creating distance metrics...\n");

	DistanceMetrics distanceMetrics = create_distance_metrics( partitions );

	cout << "num genome cache misses = " << genomeCache.nmisses << endl;

    printf("THRESH: %f\n", THRESH); 

	if(false)
	{
		const int ngenomes = ids_global->size();
		//float dists[ngenomes][ngenomes];
		float **dists = new float*[ngenomes];
		for( int i = 0; i < ngenomes; i++ ) {
			dists[i] = new float[ngenomes];
		}
		
		/*
		unsigned char *genomes[ngenomes];
		for( int i = 0; i < ngenomes; i++ ) {
			genomes[i] = genomeCache.get( ids_global->at(i) ).genes();
		}
		*/

		vector<double> times;

		for( int bar = 0; bar < 5; bar++ ) {
			double startTime = hirestime();
			for( int foo = 0; foo < 1; foo++ ) {

#if 0
				for( int i = 0; i < ngenomes; i++ ) {
					GenomeRef iref = genomeCache.get( ids_global->at(i) );
					#pragma omp parallel for
					for( int j = i+1; j < ngenomes; j++ ) {
						GenomeRef jref = genomeCache.get( ids_global->at(j) );
						dists[i][j] = compute_distance( distanceMetrics.deltaCache, iref, jref );
					}
				}
#else				

				#pragma omp parallel for schedule(dynamic)
				for( int i = 0; i < ngenomes; i++ )  {
					compute_distances( distanceMetrics.deltaCache,
									   clusterPartition->genomeCache,
									   clusterPartition->members,
									   i, i+1, ids_global->size(),
									   dists[i] + (i+1) );
				}
#endif

			}
			double endTime = hirestime();
			printf( "\ttime=%lf seconds\n", endTime - startTime );
			times.push_back( endTime - startTime );
		}

		std::sort( times.begin(), times.end() );
		printf( "\tmin time=%lf\n", times[ 0 ] );

		for( int i = 0; i < ngenomes; i++ ) {
			for( int j = i+1; j < ngenomes; j++ ) {
				float d = dists[i][j];
				float cd = compute_distance( distanceMetrics.deltaCache,
											 genomeCache.get( ids_global->at(i) ),
											 genomeCache.get( ids_global->at(j) ) );
				if( dists[i][j] != cd ) {
					printf("failed for (%d,%d). d=%f, cd=%f\n", i, j, d, cd );
					exit(0);
				}
			}
		}

		printf("OK\n");

		exit(0);
	}
	

	/*
	// ---
	// --- SORT GENES
	// ---
	sort_genes( distanceMetrics, genomeCache );

	GENES = int(Genes * 0.95);
	GENESN4 = int(GENES / 4) * 4;
	if( GENESN4 < GENES ) GENESN4 -= 4;
	distanceMetrics = create_distance_metrics( partitions );
	printf("THRESH: %f\n", THRESH); 
	*/

	// ---
	// --- CREATE CLUSTERS
	// ---
	STAGE( "creating clusters...\n" );

	bool neighbors = !neighborPartition->members.empty();

	ClusterVector clusters; 
	create_clusters( distanceMetrics.deltaCache,
					 clusterPartition,
					 clusters );

	cout << "num genome cache misses = " << genomeCache.nmisses << endl;

	if( neighbors ) {
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

		cout << "num genome cache misses = " << genomeCache.nmisses << endl;

		if( !orphans.empty() ) {
			// ---
			// --- CLUSTER ORPHANS
			// ---
			STAGE( "clustering orphans...\n" );

			PopulationPartition orphanPartition( new AgentIdVector(orphans), &genomeCache );

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

	// ---
	// --- WE'RE DONE!
	// ---
	cout << "num genome cache misses = " << genomeCache.nmisses << endl;
	STAGE( "successfully performed clustering.\n" );
}


// ================================================================================
// ================================================================================
// ================================================================================
// ================================================================================
// ===
// === POST-CLUSTERING UTILITIES
// ===
// ================================================================================
// ================================================================================
// ================================================================================
// ================================================================================

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION parse_gene_indexes
// ---
// --------------------------------------------------------------------------------
void parse_gene_indexes( map<int,string> *index2name, map<string,int> *name2index = NULL ) {
	char *path = get_run_path("genome/meta/geneindex.txt");

	errif( !is_regular_file(path), "Invalid file: %s\n", path );
		
	ifstream fin( path );
	errif( !fin.is_open(), "Failed opening file %s\n", path );

	while( fin ) {
		int index;
		string name;
		fin >> index;
		fin >> name;

		if( index2name )
			(*index2name)[index] = name;
		if( name2index )
			(*name2index)[name] = index;
	}

	free( path );
}

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION parse_births_deaths
// ---
// --------------------------------------------------------------------------------
class BirthsDeathsEntry {
public:
	AgentId id;
	int birth;
	int death;
	AgentId parent1;
	AgentId parent2;

	BirthsDeathsEntry() {
		id = 0;
		birth = death = -1;
		parent1 = parent2 = 0;
	}
};

typedef map<AgentId, BirthsDeathsEntry> BirthsDeathsMap;

void parse_births_deaths( BirthsDeathsMap &birthsDeaths ) {
	char *path = get_run_path("BirthsDeaths.log");

	errif( !is_regular_file(path), "Invalid file: %s\n", path );
		
	ifstream fin( path );
	errif( !fin.is_open(), "Failed opening file %s\n", path );

	{
		char header[4096];
		fin.getline( header, sizeof(header) );
		errif( string(header) != "% Timestep Event Agent# Parent1 Parent2", "Unexpected header: '%s'\n", header );
	}

	while( fin ) {
		int timestep;
		string event;
		AgentId agent;

		fin >> timestep;
		fin >> event;
		fin >> agent;

		if( event == "BIRTH" ) {
			AgentId parent1;
			AgentId parent2;
			fin >> parent1;
			fin >> parent2;

			BirthsDeathsEntry entry;

			entry.id = agent;
			entry.birth = timestep;
			entry.parent1 = parent1;
			entry.parent2 = parent2;
			
			birthsDeaths[agent] = entry;
		} else if( event == "CREATION" ) {
			BirthsDeathsEntry entry;

			entry.id = agent;
			entry.birth = timestep;
			
			birthsDeaths[agent] = entry;
		} else if( event == "DEATH" ) {
			BirthsDeathsEntry &entry = birthsDeaths[agent];
			entry.death = timestep;
			if( entry.id == 0 ) {
				// must have been created
				entry.id = agent;
			}
		} else {
			err( "Unexpected birth/death event '%s'\n", event.c_str() );
		}

		while( fin.peek() == '\n' ) {
			fin.get();
		}
		if( fin.peek() == EOF ) {
			break;
		}
	}

	free( path );
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

	int genomeCacheCapacity = cliParms.genomeCacheCapacity;
	if( genomeCacheCapacity == -1 ) {
		genomeCacheCapacity = (int)ids->size();
	}
	GenomeCache *genomeCache = new GenomeCache( genomeCacheCapacity, ids );
	PopulationPartition *partition = new PopulationPartition(ids, genomeCache);
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
	set_genome_size();
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
// --- FUNCTION util__zscore
// ---
// --------------------------------------------------------------------------------
namespace __util__zscore {
	class ZScore {
	public:
		float value;
		int igene;

		static bool sort( const ZScore &x, const ZScore &y ) {
			return fabs(y.value) < fabs(x.value);
		}
	};
}

void util__zscore( const char *subdir ) {
	using namespace __util__zscore;

	const int NGENES_SHOW = 10;
	const char *subdirs[] = {subdir};

	map<int,string> geneNames;
	parse_gene_indexes( &geneNames );

	LoadedClusters loadedClusters = load_clusters( true, subdirs, 1 );
	load_parms( subdirs, 1 );

	PopulationPartition *partition = loadedClusters.partitions->at(0);
	ClusterVector &clusters = *(loadedClusters.clusterMatrix->at(0));
	int nclusters = cliParms.nclusters == -1 ? (int)clusters.size() : min(cliParms.nclusters, (int)clusters.size());

	std::sort( clusters.begin(), clusters.end(), Cluster::sort__member_size_descending );
	std::sort( clusters.begin(), clusters.begin() + 10, Cluster::sort__id_ascending );

	float *mean[nclusters];
	float *stddev[nclusters];

	// Compute per-cluster gene mean and stddev
	for( int i = 0; i < nclusters; i++ ) {
		Cluster *cluster = clusters[i];
		GeneStdDev2Calculator calc;

		itfor( AgentIdVector, cluster->members, it ) {
			GenomeRef genome = partition->getGenomeById( *it );
			calc.addGenome( genome );
		}

		float *stddev2 = calc.getResult();
		for( int j = 0; j < GENES; j++ ) {
			stddev2[j] = sqrt( stddev2[j] );
		}

		stddev[i] = stddev2;
		mean[i] = calc.getMean();
	}


	ZScore *zscore[nclusters][nclusters];
	
	// Compute z-score
	for( int i = 0; i < nclusters; i++ ) {
		for( int j = 0; j < nclusters; j++ ) {
			if( i == j ) continue;

			zscore[i][j] = new ZScore[GENES];
			for( int igene = 0; igene < GENES; igene++ ) {
				zscore[i][j][igene].igene = igene;
				zscore[i][j][igene].value = (mean[i][igene] - mean[j][igene]) / stddev[j][igene];
			}
		}
	}

	// Sort z-score from hi to lo
	for( int i = 0; i < nclusters; i++ ) {
		for( int j = i+1; j < nclusters; j++ ) {
			std::sort( zscore[i][j], zscore[i][j] + GENES, ZScore::sort );
		}
	}

	cout << "=== TOP " << NGENES_SHOW << " ===" << endl;

	for( int i = 0; i < nclusters; i++ ) {
		for( int j = 0; j < nclusters; j++ ) {
			if( i == j ) continue;

			int iid = clusters[i]->id;
			int jid = clusters[j]->id;

			printf( "cluster %d relative to cluster %d :\n", iid, jid );
			printf( "    %-40s  zscore  mean(%3d)  mean(%3d)  stddev(%3d)  stddev(%3d)\n", "", iid, jid, iid, jid );
			for( int z = 0; z < NGENES_SHOW; z++ ) {
				int igene = zscore[i][j][z].igene;
				const char *name = geneNames[igene].c_str();
				printf( "    %-40s  %6.2f  %9.2f  %9.2f  %11.2f  %11.2f\n",
						name, zscore[i][j][z].value, mean[i][igene], mean[j][igene], stddev[i][igene], stddev[j][igene] );
			}
			printf( "\n" );
		}
	}

	cout << "=== MEAN OF TOP " << NGENES_SHOW << " ===" << endl;

	for( int i = 0; i < nclusters; i++ ) {
		for( int j = 0; j < nclusters; j++ ) {
			if( i == j ) continue;

			float total = 0;
			for( int z = 0; z < NGENES_SHOW; z++ ) {
				total += fabs( zscore[i][j][z].value );
			}
			float mean = total / NGENES_SHOW;
			printf( "%3d relative to %3d = %8.3f\n", clusters[i]->id, clusters[j]->id, mean );
		}
	}
	printf( "\n" );

	cout << "=== UNIQUENESS ===" << endl;

	for( int i = 0; i < nclusters; i++ ) {
		float uniqueness = 0;
		for( int j = 0 ; j < nclusters; j++ ) {
			if( i == j ) continue;

			float total = 0;
			for( int z = 0; z < NGENES_SHOW; z++ ) {
				total += fabs( zscore[i][j][z].value );
			}
			float mean = total / NGENES_SHOW;
			uniqueness += mean;
		}
		uniqueness /= nclusters - 1;
		printf( "%3d relative to X: %8.3f\n", clusters[i]->id, uniqueness );
	}

	for( int i = 0; i < nclusters; i++ ) {
		float uniqueness = 0;
		for( int j = 0 ; j < nclusters; j++ ) {
			if( i == j ) continue;

			float total = 0;
			for( int z = 0; z < NGENES_SHOW; z++ ) {
				total += fabs( zscore[j][i][z].value );
			}
			float mean = total / NGENES_SHOW;
			uniqueness += mean;
		}
		uniqueness /= nclusters - 1;
		printf( "X relative to %3d: %8.3f\n", clusters[i]->id, uniqueness );
	}

}

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION util__metabolism
// ---
// --------------------------------------------------------------------------------
#define interp(x,ylo,yhi) ((ylo)+(x)*((yhi)-(ylo)))

int interpolate( unsigned char raw, int smin, int smax ) {
	static const float OneOver255 = 1. / 255.;

	// temporarily cast to double for backwards compatibility
	double ratio = float(raw) * OneOver255;

					return min( (int)interp( ratio, int(smin), int(smax) + 1 ),
							int(smax) );

}

void util__metabolism( const char *subdir ) {
	const char *subdirs[] = {subdir};

	LoadedClusters loadedClusters = load_clusters( true, subdirs, 1 );

	load_parms( subdirs, 1 );

	BirthsDeathsMap birthsDeaths;
	parse_births_deaths( birthsDeaths );

	PopulationPartition *partition = loadedClusters.partitions->at(0);
	ClusterVector &clusters = *(loadedClusters.clusterMatrix->at(0));
	int nclusters = cliParms.nclusters == -1 ? (int)clusters.size() : min(cliParms.nclusters, (int)clusters.size());

	std::sort( clusters.begin(), clusters.end(), Cluster::sort__member_size_descending );

// 	float *mean[nclusters];
// 	float *stddev[nclusters];

	// Compute per-cluster gene mean and stddev
	for( int i = 0; i < nclusters; i++ ) {
		Cluster *cluster = clusters[i];
		GeneStdDev2Calculator calc;

		itfor( AgentIdVector, cluster->members, it ) {
			GenomeRef genome = partition->getGenomeById( *it );
			calc.addGenome( genome );
		}

// 		float *stddev2 = calc.getResult();
// 		for( int j = 0; j < GENES; j++ ) {
// 			stddev2[j] = sqrt( stddev2[j] );
// 		}
// 
// 		stddev[i] = stddev2;
// 		mean[i] = calc.getMean();
	}

	int nmetabolisms = 2;
	fprintf( stderr, "Warning! Hard-coded nmetabolisms\n" );
	int imetabolism = 8;
	fprintf( stderr, "Warning! Hard-coded imetabolism\n" );

	char *path_result = get_results_path(subdir, "metabolism");
	printf( "writing results to %s\n", path_result );
	FILE *f = fopen( path_result, "w" );
	errif( f == NULL, "Failed opening result file\n" );

	for( int i = 0; i < nclusters; i++ ) {
		Cluster *cluster = clusters[i];
		GeneStdDev2Calculator calc;

		fprintf( f, "cluster %2d (n=%6lu): ", cluster->id, cluster->members.size() );

		int count[nmetabolisms];
		memset( count, 0, sizeof(count) );

		int begin = -1, end = -1;

		itfor( AgentIdVector, cluster->members, it ) {
			GenomeRef genome = partition->getGenomeById( *it );
			unsigned char raw = genome.genes()[imetabolism];
			int index = interpolate( raw, 0, nmetabolisms - 1 );
			assert( index >= 0 && index < nmetabolisms );
			count[index]++;

			BirthsDeathsEntry &entry = birthsDeaths[*it];
			if( (begin == -1) || ((entry.birth != -1) && (entry.birth < begin)) ) {
				begin = entry.birth;
			}
			if( (end == -1) || ((entry.death != -1) && (entry.death > end)) ) {
				end = entry.death;
			}
				
		}

		for( int j = 0; j < nmetabolisms; j++ ) {
			fprintf( f, "nmet%d=%5d (%5.1f%%) ", j, count[j], (double)100 * count[j] / cluster->members.size() );
		}
		fprintf( f, "T=[%6d,%6d]", begin, end );
		fprintf( f,"\n");
	}
	fclose( f );
}

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION util__ancestry
// ---
// --------------------------------------------------------------------------------
namespace __util__ancestry {
	class TimeRange {
	public:
		int start;
		int end;

		TimeRange() {
			start = -1;
			end = -1;
		}

		void update( int t ) {
			if( start == -1 ) {
				assert(end == -1);
				start = end = t;
			} else if( t != -1 ) {
				start = min(start, t);
				end = max(end, t);
			}
		}
	};

	class AncestorInfo {
	public:
		int count;
		TimeRange timeRange;

		AncestorInfo() {
			count = 0;
		}
	};

	typedef pair<ClusterId,ClusterId> AncestorClusterIds;
	typedef map<AncestorClusterIds, AncestorInfo> AncestorInfoMap;

	class ClusterInfo {
	public:
		size_t nmembers;
		TimeRange timeRange;
		AncestorInfoMap ancestorInfos;

		ClusterInfo() {
			nmembers = 0;
		}
	};

	typedef map<ClusterId, ClusterInfo> ClusterInfoMap;

	bool sort_cluster_time( const pair<ClusterId,ClusterInfo> &a, const pair<ClusterId,ClusterInfo> &b ) {
		return a.second.timeRange.start > b.second.timeRange.start;
	}

	bool sort_ancestor_time( const pair<AncestorClusterIds,AncestorInfo> &a, const pair<AncestorClusterIds,AncestorInfo> &b ) {
		return a.second.timeRange.start < b.second.timeRange.start;
	}
}

void util__ancestry( const char *subdir ) {
	using namespace __util__ancestry;

	BirthsDeathsMap birthsDeaths;
	parse_births_deaths( birthsDeaths );

	ParsedClusterVector clusters;
	parse_clusters( get_results_path(subdir, "members_neighbors"), clusters );

	typedef map<AgentId, ClusterId> ClusterIdMap;
	ClusterIdMap clusterIdLookup;

	ClusterInfoMap clusterInfos;

	itfor( ParsedClusterVector, clusters, it_cluster ) {
		ParsedCluster &cluster = *it_cluster;

		ClusterInfo &clusterInfo = clusterInfos[cluster.id];
		clusterInfo.nmembers = cluster.members.size();

		itfor( AgentIdVector, cluster.members, it ) {
			clusterIdLookup[*it] = cluster.id;
			BirthsDeathsEntry &entry = birthsDeaths[*it];
			clusterInfo.timeRange.update( entry.birth );
			clusterInfo.timeRange.update( entry.death );
		}
	}

	itfor( BirthsDeathsMap, birthsDeaths, it ) {
		BirthsDeathsEntry &entry = it->second;

		if( entry.parent1 > 0 ) {
			assert( entry.parent2 > 0 );
			assert( entry.parent1 != entry.parent2 );

			ClusterId childClusterId = clusterIdLookup[entry.id];
			ClusterId parent1ClusterId = clusterIdLookup[entry.parent1];
			ClusterId parent2ClusterId = clusterIdLookup[entry.parent2];

			AncestorClusterIds ancestorIds = make_pair( min(parent1ClusterId, parent2ClusterId),
														max(parent1ClusterId, parent2ClusterId) );
			clusterInfos[childClusterId].ancestorInfos[ancestorIds].count++;
			if( entry.birth > -1 ) {
				clusterInfos[childClusterId].ancestorInfos[ancestorIds].timeRange.update( entry.birth );
			}
		}
	}

	char *path_result = get_results_path(subdir, "ancestry");
	printf( "writing results to %s\n", path_result );
	FILE *f = fopen( path_result, "w" );
	errif( f == NULL, "Failed opening result file\n" );

	typedef vector<pair<ClusterId,ClusterInfo> > ClusterInfoVector;
	ClusterInfoVector clusterInfoVector( clusterInfos.begin(), clusterInfos.end() );
	std::sort( clusterInfoVector.begin(), clusterInfoVector.end(), sort_cluster_time );

	itfor( ClusterInfoVector, clusterInfoVector, it_cluster ) {
		ClusterId clusterId = it_cluster->first;
		ClusterInfo &clusterInfo = it_cluster->second;

		if( clusterInfo.nmembers > 500 ) {
			fprintf( f,"cluster %d (n=%lu): T=[%d,%d]\n",
					 clusterId, clusterInfo.nmembers, clusterInfo.timeRange.start, clusterInfo.timeRange.end );

			typedef vector<pair<AncestorClusterIds,AncestorInfo> > AncestorInfoVector;
			AncestorInfoVector ancestorInfoVector( clusterInfo.ancestorInfos.begin(), clusterInfo.ancestorInfos.end() );
			std::sort( ancestorInfoVector.begin(), ancestorInfoVector.end(), sort_ancestor_time );

			itfor( AncestorInfoVector, ancestorInfoVector, it_ancestor ) {
				AncestorClusterIds ids = it_ancestor->first;
				AncestorInfo &ancestorInfo = it_ancestor->second;
				fprintf( f, "    (%d,%d): n=%d T=[%d,%d]\n",
						 ids.first, ids.second,
						 ancestorInfo.count,
						 (ancestorInfo.timeRange.start - clusterInfo.timeRange.start),
						 (ancestorInfo.timeRange.end - clusterInfo.timeRange.start) );
						
			}
		}
	}

	fclose( f );
}

// --------------------------------------------------------------------------------
// ---
// --- FUNCTION util__genedist
// ---
// --------------------------------------------------------------------------------
namespace __util__genedist {
	class GeneDist {
	public:
		float value;
		int igene;

		GeneDist( float value, int igene ) {
			this->value = value;
			this->igene = igene;
		}

		static bool sort( const GeneDist &x, const GeneDist &y ) {
			return fabs(y.value) < fabs(x.value);
		}
	};
}

void util__genedist( const char *subdir ) {
	using namespace __util__genedist;

	const int NGENES_SHOW = 10;
	const char *subdirs[] = {subdir};

	map<int,string> geneNames;
	parse_gene_indexes( &geneNames );

	LoadedClusters loadedClusters = load_clusters( true, subdirs, 1 );
	load_parms( subdirs, 1 );

	DistanceMetrics distanceMetrics = create_distance_metrics( *(loadedClusters.partitions) );

	PopulationPartition *partition = loadedClusters.partitions->at(0);
	ClusterVector &clusters = *(loadedClusters.clusterMatrix->at(0));
	int nclusters = cliParms.nclusters == -1 ? (int)clusters.size() : min(cliParms.nclusters, (int)clusters.size());

	std::sort( clusters.begin(), clusters.end(), Cluster::sort__member_size_descending );

	typedef vector<GeneDist> GeneDistVector;
	GeneDistVector geneDists[nclusters][nclusters];

	for( int i = 0; i < nclusters; i++ ) {
		Cluster *clusteri = clusters[i];

		for( int j = i+1; j < nclusters; j++ ) {
			Cluster *clusterj = clusters[j];
			
			double geneDistTotals[ GENES ];
			memset( geneDistTotals, 0, sizeof(geneDistTotals) );

			for( int imember = 0; imember < (int)clusteri->members.size(); imember++ ) {
				GenomeRef genomei = partition->getGenomeById( clusteri->members[imember] );

				for( int jmember = 0; jmember < (int)clusterj->members.size(); jmember++ ) {
					GenomeRef genomej = partition->getGenomeById( clusterj->members[jmember] );

					#pragma omp parallel for
					for( int igene = 0; igene < GENES; igene++ ) {
						geneDistTotals[igene] += distanceMetrics.deltaCache[igene].deltaValue[ abs(genomei.genes()[igene] - genomej.genes()[igene]) ];
					}
				}
			}

			int ndists = clusteri->members.size() * clusterj->members.size();
			for( int igene = 0; igene < GENES; igene++ ) {
				geneDists[i][j].push_back( GeneDist( float(geneDistTotals[igene] / ndists), igene ) );
			}

			std::sort( geneDists[i][j].begin(), geneDists[i][j].end(), GeneDist::sort );

			printf( "%d --> %d:\n", clusters[i]->id, clusters[j]->id );

			for( int k = 0; k < NGENES_SHOW; k++ ) {
				GeneDist &dist = geneDists[i][j][k];
				printf( "    [ %d : %s ] = %f\n", dist.igene, geneNames[dist.igene].c_str(), dist.value );
			}
		}
	}
}

