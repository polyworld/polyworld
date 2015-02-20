#ifndef COMPLEXITY_ALGORITHM_H
#define COMPLEXITY_ALGORITHM_H

#include <gsl/gsl_blas.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_linalg.h>
#include <math.h>
// #include <list>
#include <dirent.h>
#include <stdlib.h>
#include <gsl/gsl_statistics_double.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <assert.h>

#define DEFAULT_SEED 42

bool n_choose_k_le_s( int n, int k, int s );
void setGaussianize( bool gaussianize );

gsl_rng *create_rng( int seed );
void dispose_rng( gsl_rng *rng );

void print_matrix( gsl_matrix* );
void print_matrix_row(gsl_matrix*, int );
void print_matrix_column(gsl_matrix*, int );
bool is_matrix_square( gsl_matrix* );
gsl_matrix* matrix_crosssection( gsl_matrix* m, int[], int );
gsl_matrix* matrix_subset_col( gsl_matrix* mInput, int* columns, int numColumns );

double determinant( gsl_matrix* );
double CalcI( gsl_matrix* COV, double det );
double CalcI_k( gsl_matrix*, int* , int );
// double calcC_nm1( gsl_matrix*, double ); // requires a square covariance Matrix
double calcC_k( gsl_matrix * COV, double I_n, int k );
double calcC_k_exact( gsl_matrix* COV, double I_n, int k );

gsl_matrix* calcCOV( gsl_matrix* );					// function overloaded for simpler, faster COV calculation
gsl_matrix* calcCOV( gsl_matrix* CIJ, double r );	// function overloaded for when we don't need to compute COR
void rescaleCOV( gsl_matrix* COV, double det );

void gsamp( gsl_matrix_view x );
int qsort_compare_double( const void *a, const void *b );
int qsort_compare_rows0( const void *a, const void *b );

double CalcComplexityWithMatrix( gsl_matrix* data );
double CalcComplexityWithVector( gsl_vector* vector, size_t blockDuration, size_t blockOffset );
double CalcApproximateFullComplexityWithMatrix( gsl_matrix* data, int numPoints );
double CalcApproximateFullComplexityWithVector( gsl_vector* vector, size_t blockDuration, size_t blockOffset, int numPoints );

#endif // COMPLEXITY_ALGORITHM_H
