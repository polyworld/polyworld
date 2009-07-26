#ifndef COMPLEXITY_ALGORITHM_H
#define COMPLEXITY_ALGORITHM_H

#include <gsl/gsl_blas.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_linalg.h>
#include <math.h>
#include <list>
#include <dirent.h>
#include <stdlib.h>
#include <gsl/gsl_statistics_double.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <assert.h>

#define DEFAULT_SEED 42

gsl_rng *create_rng(int seed);
void dispose_rng(gsl_rng *rng);

double calcI_det2( gsl_matrix * );
gsl_matrix * matrix_crosssection( gsl_matrix * COR, int[], int );
void print_matrix( gsl_matrix * );
double calcC_det3( gsl_matrix * ); // COR is a square Matrix
double calcC_det3__optimized( gsl_matrix * foreignCOR );		// this one needs to remade.
gsl_matrix * identity_matrix( int );
void print_matrix_row(gsl_matrix *, int);
void print_matrix_column(gsl_matrix *, int);
gsl_matrix * mget_matrix_diagonal( gsl_matrix *);

gsl_matrix * calcCOV( gsl_matrix * CIJ, double r, gsl_matrix * COR );
gsl_matrix * calcCOV( gsl_matrix * CIJ, double r );			//function overloaded for when we don't need to compute COR
gsl_matrix * COVtoCOR( gsl_matrix * COV );			//function overloaded for when we don't need to compute COR
bool is_matrix_square( gsl_matrix * );
gsl_matrix * mCOV( gsl_matrix * );

gsl_matrix * gsamp( gsl_matrix_view x );
int qsort_compare_double( const void *a, const void *b);
int qsort_compare_rows0( const void *a, const void *b );
int qsort_compare_rows1( const void *a, const void *b );

#endif // COMPLEXITY_ALGORITHM_H
