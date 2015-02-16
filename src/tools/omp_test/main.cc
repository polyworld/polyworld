#include <omp.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
    unsigned n = omp_get_max_threads();
    bool called[n];
    for(unsigned i = 0; i < n; i++) called[i] = false;
  
#pragma omp parallel for
    for(unsigned i = 0; i < n; i++) {
        called[omp_get_thread_num()] = true;
    }

    for(unsigned i = 0; i < n; i++) {
        if(!called[i]) {
            fprintf(stderr, "Not called by thread %u\n", i);
            exit(1);
        }
    }
}
