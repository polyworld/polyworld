#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>
#include "readline.h"
#include <omp.h>

// Cluster C code goes in polyworld/tools/cluster
// Cluster animation code goes in polyworld/scripts

// TODO: Make pathname an argument
// TODO: Make constants GENES and POPSIZE determined from run directory
//       Implies dynamic allocation of G, std2, dists, etc
// TODO: Write cluster info to a file (in run/cluster/<std.dev.>)

// TODO: Number of genes needs to be determined from the run directory
#define GENES 2494
// TODO: Get population size from the run directory
#define POPSIZE 29564
#define OFFSET 1 

// Debug flags
#define PRINT_ELTS 1
#define DEBUG 0 
#define NUM_BINS 16

unsigned char G[POPSIZE][GENES];

float std2[GENES];
float dists[POPSIZE][POPSIZE];
float ents[GENES];
float THRESH;

float distance(int i, int j) {
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


int min_indexf(float a[], int ELTS) { 
	int i, max; max = 0;
	for (i = 1; i < ELTS; i++)
		if ((-1 * a[i]) > (-1* a[max])) {
			max = i;
        }
	return max;
}

bool clust[POPSIZE][POPSIZE];

int cluster_length(int n, int ELTS) { 
	int i, sum; sum = 0;
	for (i = 0; i < ELTS; i++)
		if (clust[n][i] == true) sum++;
	return sum;
}


void build_cluster(int i, int POP) {
    bool *AC = clust[i];
	int j, lastPick;
    float max_dist[POP]; 
	for (j = 0; j < POP; j++) {
		max_dist[j] = 0; AC[j] = false;
	}
	lastPick = i; AC[i] = true; max_dist[i] = THRESH + 1;
    if (DEBUG) printf("%dC ", i);
	
	bool flag, flag2; float d; int pick; flag = true;
	while (flag) {
		for (j = 0; j < POP; j++) {
			if (max_dist[j] < THRESH) {
				// d = distance(G[lastPick], G[j]);
                d = dists[lastPick][j];
                if (DEBUG) printf("%d -> %d: %f (cur: %f) \n", lastPick, j, d, max_dist[j]);
				if (d > max_dist[j]) max_dist[j] = d;
                if (DEBUG) printf("%d -> %d: %f (cur: %f) \n", i, j, d, max_dist[j]);
			}
		}

		pick = min_indexf(max_dist, POP);
		if (max_dist[pick] > THRESH) {
			flag = false;
		} else {
			lastPick = pick; AC[pick] = true; max_dist[pick] = THRESH + 1;
			
			flag2 = true;
			for (j = 0; j < POP; j++) {
				if (flag2 && (max_dist[j] < THRESH)) {
					flag2 = false; break;
				}
			}
			flag = !flag2;
		}
	}
    if (DEBUG) printf("%dC | (len %d)\n", i, cluster_length(i, POP));
}

int overlap(int c1, int c2, int POP) {
	int i;
    bool* clust1 = clust[c1];
    bool* clust2 = clust[c2];
	for (i=0; i < POP; i++){
		//printf("%d || %d & %d\n", i, x[i], y[i]);
		if (clust[c1][i] && clust[c2][i]) return 1;
	}	
	return 0;
}

int ids[POPSIZE];
int clusterId = 0;

void pick(int cid, int POP) {
    int i, k, j = 0;
    bool* bigclust = clust[cid];

	for (i = 0; i < POP; i++) {
        if (DEBUG) printf("moving agent %d -> %d\n", i, j);
        if (!bigclust[i]) {
            memmove(G[j], G[i], GENES*sizeof(char));
            memmove(dists[j], dists[i], POP*sizeof(float)); //double check
            ids[j] = ids[i];
            for (k=0; k < POP; k++) {
                dists[k][j] = dists[k][i];
                clust[k][j] = clust[k][i];
            }
            if (DEBUG) printf("success!\n");
            j++;
        }    
    }
    
    clusterId++;	

}

void print_cluster(int id, int POP) {
    int i;

	printf("cluster %d (%d elts)", clusterId, cluster_length(id, POP));
    if (PRINT_ELTS) {
        printf(" : ");
        for (i=0; i<POP; i++) {
            if (clust[id][i]) printf("%d ", ids[i]);
        }
    }
    printf("\n");
}

int cluster_all(int POP) {
	int i, j, size, candidates = POP, big = 0, bigi = 0;

    if (DEBUG) printf("beginning cluster %d. genome size: %d\n", clusterId, POP);
   
    char candidate[POP];
    #pragma omp parallel for shared(candidate,POP) private(i) schedule(dynamic)
	for (i = 0; i < POP; i++) {
		build_cluster(i, POP);
        candidate[i] = true;
	}

    while ((candidates > 0) && (POP > 0)) {
        bigi = 0;
        big = 0;
        // select biggest cluster
        for (i=0; i < POP; i++) {
            if (candidate[i]) {
    		    size = cluster_length(i, POP);
    		    if (size > big) {
    			    bigi = i;
    			    big = size;
    		    }
            }
        }


        if (DEBUG) printf("selecting cluster %d (%d/%d agts)\n", 
                              bigi, cluster_length(bigi, POP), POP);
        if (PRINT_ELTS) print_cluster(bigi, POP);
    
        // reduce candidate pool
        j=candidates;
        candidates=0;
        #pragma omp parallel for shared(j,candidate,bigi,POP) private(i) reduction(+: candidates) schedule(dynamic)
	    for (i = 0; i < j; i++) {
		    if (candidate[i] && !overlap(bigi, i, POP) && (cluster_length(i, POP) > 0))
                candidates += 1;
            else candidate[i] = false;
            if (DEBUG) printf("%d (%d) -> %d (%d) | %d, %d (numCan: %d)\n", 
                                bigi, big, i, cluster_length(i, POP),
                                overlap(bigi, i, POP), candidate[i], candidates);
	    }	
    	
        // remove biggest cluster's elements from the pool of genomes
        pick(bigi, POP);

        //move candidates up in cluster queue
        j=0;
        for (i=0; i < POP; i++) {
            if (candidate[i]) {
                memmove(clust[j], clust[i], POP*sizeof(bool));
                candidate[j] = candidate[i];
                j++;
            }    
        }

        // reset POP size for next iteration
        POP = POP-big;


        if (1 && candidates) printf("numCan: %d\n", candidates);
        if (DEBUG) printf("POP: %d\n", POP);
        //candidates = 0;
    }
    printf("done with clustering\n");
    return POP;
}




void load_genome(int id, int ind) {
    FILE *fp;
    char filename[100];
	// TODO: Make pathname an argument
    sprintf(filename, "/home/jaimie/polyworld/run/genome/genome_%d.txt", id);

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
    print_float_vector(ents, GENES);


    int *avg = mean(POP);
    float *std = stddev(POP, avg);
    for (i =0; i < GENES; i++)
        std2[i] = pow(std[i],2);
    print_vector(avg, GENES);
    print_float_vector(std2, GENES);

    // set threshold
    for (i=0; i<GENES; i++)
        THRESH += ents[i];
    THRESH *= THRESH_FACT;
    printf("THRESH: %f\n", THRESH); 

    // calculate distances
    printf("calculating distances...\n");
    float d; float* D;
    for (i = 0; i < POPSIZE; i++) {
        D = dists[i];
        
        #pragma omp parallel for shared(D, i, dists) private(d, j)
	    for (j = i+1; j < POPSIZE; j++) {
            d = distance(i, j);
            D[j] = d;
            if (DEBUG) printf("%d -> %d: %f\n", i, j, d);
        }
        
        D[i] = 0.0;
        
        #pragma omp parallel for shared(D, i, dists) private(d, j)
        for (j = 0; j < i; j++) {
            d = dists[j][i];
            D[j] = d;
            if (DEBUG) printf("%d -> %d: %f\n", i, j, d);
        }

    }

    if (DEBUG)
        for (i = 0; i < POPSIZE; i++)
            print_float_vector(dists[i], POPSIZE);
   
    // cluster until all elts are clustered
    printf("starting clusters...\n");
    while (POP > 0) {
        POP = cluster_all(POP);
        printf("to cluster: %d\n", POP);
    }

    return POP;
}
