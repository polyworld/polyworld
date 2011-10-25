#include "complexity_algorithm.h"

#include <iostream>

// RescaleCOV and Fix_I can go away, as RescaleCOV should always be off
// and Fix_I should always be on.  I am only leaving them around for one
// CVS check-in, so it is easy to provide backward compatibility (which
// corresponds to RescaleCOV on and Fix_I off).
#define RescaleCOV 0
#define Fix_I 1

// c_log should be the logarithm function we want to use to calculate
// entropy and integration.  Normally I would prefer to stick with log2,
// which lets us think in bits and is consistent with Cover & Thomas,
// but the old code used log, the natural logarithm, so c_log has been
// invented to make it easy to provide backward compatibility (which
// would correspond to making c_log equal to log instead of log2).
#define c_log log2

#define DebugCalcC_k false

#if DebugCalcC_k
	#define calcC_k_print( format, data... ) printf( format, data )
#else
	#define calcC_k_print( format, data... )
#endif

static bool Gaussianize = true;

using namespace std;


//---------------------------------------------------------------------------
// setGaussianize()
//
// allows client code to enable or disable the Gaussianization of data
// prior to computing complexity
//---------------------------------------------------------------------------
void setGaussianize( bool gaussianize )
{
	Gaussianize = gaussianize;
}


//---------------------------------------------------------------------------
// create_rng()
//
// constructs a random number generator and seeds it
//---------------------------------------------------------------------------
gsl_rng *create_rng( int seed )
{
	gsl_rng *rng = gsl_rng_alloc( gsl_rng_mt19937 );
	gsl_rng_set( rng, seed );

	return rng;
}

//---------------------------------------------------------------------------
// dispose_rng()
//
// disposes a random number generator
//---------------------------------------------------------------------------
void dispose_rng( gsl_rng *rng )
{
	gsl_rng_free( rng );
}


int qsort_compare_double( const void* a, const void* b )
{
	if( *(double*)a < *(double*)b )
		return -1;
	else if( *(double*)a == *(double*)b )
		return 0;
	else
		return 1;
}

int qsort_compare_rows0( const void* a, const void* b )
{
	// Sorts gsl_vectors in ascending order by their first entry (entry 0)
	if(  gsl_vector_get( *(gsl_vector**)a, 0 )  <  gsl_vector_get( *(gsl_vector**)b, 0 )  )
		return -1;
	else if( gsl_vector_get( *(gsl_vector**)a, 0 ) == gsl_vector_get( *(gsl_vector**)b, 0 ) )
		return 0;
	else
		return 1;
}

int qsort_compare_rows1( const void* a, const void* b )
{
	// Sorts gsl_vectors in ascending order by their second entry (entry 1)
	if(  gsl_vector_get( *(gsl_vector**)a, 1 )  <  gsl_vector_get( *(gsl_vector**)b, 1 )  )
		return -1;
	else if( gsl_vector_get( *(gsl_vector**)a, 1 ) == gsl_vector_get( *(gsl_vector**)b, 1 ) )
		return 0;
	else
		return 1;
}

//---------------------------------------------------------------------------
// gsamp() function, a re-implementation of gsamp.m
// 
// Description: Creates a timeseries with same rank order as the input but with Gaussian amp.
// 
// Note: For efficiency, this gsamp() function is slightly different from gsamp.m.
// It receives a tranpose and outputs a transpose of the matrix received/outputted
// by the MATLAB gsamp(). I.e. for  A = some matrix  B = gsamp(A)
// 	 MATLAB: B' = gsamp(A')
// 	    C++: B  = gsamp(A)
//---------------------------------------------------------------------------
gsl_matrix* gsamp( gsl_matrix_view x )
{
	int r = (&x.matrix)->size1;
	int c = (&x.matrix)->size2;

	if( r < c )
	{
		cerr << "gsamp: There are more cols than rows.  gsamp() requires that each neuron be a column.  We could do a tranpose here and continue, but considering the rest of the code this situation should be impossible as the matrix is already aligned.  So, there's probably something deeply wrong -- exiting.";
		exit(1);
	}

	int n  = (&x.matrix)->size1;
	int cc = (&x.matrix)->size2;

	gsl_matrix* y = gsl_matrix_calloc( n, cc );

	// create a gaussian timeseries with the same rank-order of x
	gsl_rng *randNumGen = create_rng( DEFAULT_SEED );
	
	// this is nessecary for switching the columns around among the x, z, and y matrices
	gsl_vector * tempcol = gsl_vector_alloc( n );

	for( int i = 0; i < cc; i++ )
	{
		gsl_matrix* z = gsl_matrix_calloc( n, 3 );

		double gs[n]; for( int j = 0; j < n; j++ ) { gs[j] = gsl_ran_ugaussian(randNumGen); }
		qsort( gs, n, sizeof(double), qsort_compare_double);

		gsl_matrix_get_col( tempcol, &x.matrix, i );		// get i'th column of X
		gsl_matrix_set_col( z, 0, tempcol );				// put i'th column of X into z's first column

		// there may be an error here.  In Matlab it's supposed to be [1:n], but this is [0:n-1]
		for( int j = 1; j <= n; j++ )
			gsl_matrix_set( z, j-1, 1, j );

		// now we must sort the rows of z by the first (0th) column.  First must pull out all of the rows, and put them into an array of vectors.
		gsl_vector * zrows[n];
 		for( int j = 0; j < n; j++ ) { zrows[j] = gsl_vector_calloc(3); }		// all rows are initially zero.

		for( int j = 0; j < n; j++ ) { gsl_matrix_get_row( zrows[j], z, j ); }	// get the rows
		qsort( zrows, n, sizeof(zrows[0]), qsort_compare_rows0 );			// sort the rows by column 0
		for( int j = 0; j < n; j++ ) { gsl_matrix_set_row( z, j, zrows[j] ); }	// overwrite z with the sorted rows

		for( int j = 0; j < n; j++ ) { gsl_matrix_set( z, j, 2, gs[j] ); }		// z(:,3) = gs;

		// now we must sort matrix z by the second (1'th) column.  As before, pull out all of the rows, call qsort, them put them back in.
		for( int j = 0; j < n; j++ ) { gsl_matrix_get_row( zrows[j], z, j ); }	// get the rows
		qsort( zrows, n, sizeof(zrows[0]), qsort_compare_rows1 );		// sort the rows by column 1
		for( int j = 0; j < n; j++ ) { gsl_matrix_set_row( z, j, zrows[j] ); }	// overwrite z with the sorted rows

		for( int j = 0; j < n; j++ ) { gsl_vector_free( zrows[j] ); }		// done with the temp sorting rows.  set them free.

		gsl_matrix_get_col( tempcol, z, 2);		// get column 2 from matrix 'z'...
		gsl_matrix_set_col( y, i, tempcol );	// and put the 2nd column of 'z' into the i'th column of matrix 'y'

		gsl_matrix_free( z );			// we're done with our temp matrix z now.
	}
	gsl_vector_free( tempcol );

	dispose_rng( randNumGen );

	if( r < c )
	{
		cout << "end of gsamp(): r < c -- this should be impossible." << endl;
		exit(1);
	}

	return y;
}


bool is_matrix_square( gsl_matrix* m )
{
	if( m->size1 == m->size2 )
		return true;
	else
		return false;
}


gsl_matrix* calcCOV( gsl_matrix* m )
{
	// The input matrix may not be square, but the output will always be a square NxN matrix
	// where N is the number of columns in the input matrix. 

	gsl_matrix* COV = gsl_matrix_alloc( m->size2, m->size2 );

	double array_col_i[m->size1];	// The GSL covariance function takes arrays
	double array_col_j[m->size1];	// The GSL covariance function takes arrays

	for( unsigned int i = 0; i < m->size2; i++ )
	{
		for( unsigned int j = 0; j <= i; j++ )	// We only goto <= i because we need to only calculate covariance for half of the matrix.
		{
			gsl_vector_view col_i = gsl_matrix_column( m, i );
			gsl_vector_view col_j = gsl_matrix_column( m, j );

			for( unsigned int count = 0; count < m->size1; count++ )	// Time to convert vectors to arrays
			{
				array_col_i[count] = gsl_vector_get( &col_i.vector, count);
				array_col_j[count] = gsl_vector_get( &col_j.vector, count);
			}

			gsl_matrix_set( COV, i, j, gsl_stats_covariance( array_col_i, 1, array_col_j, 1, m->size1 ) ); 
		}
	}

	// Many values in the matrix are repeated so we can just fill those in instead of recalculating them.
	for( unsigned int i = 0; i < m->size2; i++ )
	{
		for( unsigned int j = i+1; j < m->size2; j++ )
			gsl_matrix_set( COV, i, j, gsl_matrix_get( COV, j, i ) );
	}
	
	return COV;
}


void rescaleCOV( gsl_matrix* COV, double det )
{
	// MATLAB CODE:
	// d1 = det( COR );
	// while ~ d1
	//    COR = COR.*1.3;
	//    d1 = det(COR);
	// end
	// COR = COR./exp(log(d1)/N);

	int n = COV->size1;
	double d1 = det;
	while( d1 == 0 )
	{
		gsl_matrix_scale( COV, 1.3 );
		d1 = determinant( COV );
	}
	gsl_matrix_scale( COV, 1.0 / exp(c_log(d1)/n) );
}


void print_matrix_row( gsl_matrix* m, int row )
{

	gsl_vector_view row_i = gsl_matrix_row( m, row );
	int row_length = (&row_i.vector)->size;

	cout << "Length of row " << row << ": " << row_length << endl;
	cout << "Values in row: ";

	for( int temp = 0; temp < row_length; temp++ )
		cout << gsl_vector_get( &row_i.vector, temp) << "  ";
	cout << endl;

}

void print_matrix_column( gsl_matrix* m, int col )
{

	gsl_vector_view col_i = gsl_matrix_column( m, col );
	int col_length = (&col_i.vector)->size;

	cout << "Length of col: " << col << ": " << col_length << endl;
	cout << "Values in col: ";

	for(int temp = 0; temp < col_length; temp++ )
		cout << gsl_vector_get( &col_i.vector, temp) << "  ";
	cout << endl;

}


void print_matrix( gsl_matrix* m )
{
	int end  = m->size2;	//These may need to be flipped, not sure.
	int end2 = m->size1;	//These may need to be flipped, not sure.

	for( int i = 0; i < end2; i++ )
	{
		for( int j = 0; j < end; j++ )
			cout << "[" << i << "," << j << "]: " << gsl_matrix_get( m, i, j ) << "\t" ;
		cout << endl;
	}
}


//---------------------------------------------------------------------------
// matrix_crosssection() takes 3 arguments:
// 	1. The input matrix.
// 	2. An array of which indexes you want the crosssections among (must be in ascending order).
// 	   All values in the array must be between [0,N-1] where N is the size of the input matrix.
// 	3. The length of array from #2
// 
// 	The returned matrix will be square of size indexArrayLength x indexArrayLength.
//---------------------------------------------------------------------------
gsl_matrix* matrix_crosssection( gsl_matrix* mInput, int* indexArray, int indexArrayLength )
{
	// Allocate our matrix to return
	gsl_matrix* mOutput = gsl_matrix_alloc( indexArrayLength, indexArrayLength );

	for( int row = 0; row < indexArrayLength; row++ )
	{
		gsl_vector_view row_i = gsl_matrix_row( mInput, indexArray[row] );	

		for( int col = 0; col < indexArrayLength; col++ )
			gsl_matrix_set( mOutput, row, col, gsl_vector_get( &row_i.vector, indexArray[col] ) );
	}

	return mOutput;
}


double determinant( gsl_matrix* m )
{
	int n = m->size1;
	int signum;
	
	// Allocate the LU decomposition matrix so we don't overwrite m
	gsl_matrix* ludecomp = gsl_matrix_alloc( n, n );
	gsl_matrix_memcpy( ludecomp, m );
	
	gsl_permutation* p = gsl_permutation_alloc( n );
	
	gsl_linalg_LU_decomp( ludecomp, p, &signum );
	double det = fabs( gsl_linalg_LU_det( ludecomp, signum ) );
	
	// Free our allocations
	gsl_permutation_free( p );
	gsl_matrix_free( ludecomp );
	
	return( det );
}


//---------------------------------------------------------------------------
// Calculate C_k (linear I - actual I for subset size k)
// For any but the edge cases, an approximation is calculated
// based on NumSamples rather than using all possible subsets of size k.
//
//  C_k(X)   =  (k/N) I(X)  -  <I(X_k)>
// <I(X_k)>  =  1/(n_choose_k)  Sum[i=1,n_choose_k] I(X_k_i)
// I(X_k_i)  =  the i-th subset of X of size k
//
// May also be written:
//
//  C_k(X)   =  LI_k  -  EI_k
//
// where LI_k is the (k/N) linear portion of I(X)
// and EI_k is the expected value of actual, calculated values
// of all I_k_i subsets of size k (or, in practice, some number
// of samples of I_k_i).
//---------------------------------------------------------------------------
double calcC_k( gsl_matrix* COV, double I_n, int k )
{
	#define NumSamples 100
	
	int n = COV->size1;

	if( k == n-1 )	// next to last term, which is the usual "simplified TSE"
		return( calcC_nm1( COV, I_n ) );
	else if( k == 1 )	// second term
		return( I_n / n );
	else if( k == 0 || k == n )	// first or last term
		return( 0.0 );
	
	// If we reach here, we are dealing with the general case (k > 1 and k < n-1)

	// Calculate the linear, unstructured subset of I_n
	double LI_k = I_n * k / n;
	
	// We will not even try to compute the true expected value of I_k, <I(X_k)>,
	// as the number of subsets, N_choose_k, grows to astronomical values.
	// Instead we approximate it with a modest number of samples.

	gsl_rng *randNumGen = create_rng( DEFAULT_SEED );

	int indexes[k];
	double EI_k = 0.0;
	
	// printf( "---- n=%d, k=%d, s=%d ----\n", n, k, NumSamples );
	for( int i = 0; i < NumSamples; i++ )
	{
		// Choose a random subset of size k out of the n random variables
		int numChosen = 0;
		int numVisited = 0;
		for( int j = 0; j < n; j++ )
		{
			double prob = ((double) (k - numChosen)) / (n - numVisited);
			if( gsl_rng_uniform(randNumGen) < prob )
				indexes[numChosen++] = j;
			numVisited++;
		}
		
		EI_k += calcI_k( COV, indexes, k );
		
		// printf( "%d: EI_%d = %g\n", i, k, EI_k / (i+1) );
	}
	
	dispose_rng( randNumGen );

	EI_k /= NumSamples;
	
	return( LI_k - EI_k );
}


//---------------------------------------------------------------------------
// Calculate C_k for k = n - 1
//---------------------------------------------------------------------------
double calcC_nm1( gsl_matrix* COV, double I_n )
{
	int n = COV->size1;

	double sumI_n1=0;

	for( int i = 0; i < n; i++ )
	{
		int b[n-1];
		int b_length = n-1;

		for( int j = 0; j < n; j++ )
		{
			if( j < i )
				b[j] = j;		
			else if( j > i )
				b[j-1] = j;
		
		}

		//Technically we don't have to store this array, but for now lets stay consistent with the MATLAB code
		gsl_matrix* Xed_COV =  matrix_crosssection( COV, b, b_length );
		double det = determinant( Xed_COV );
		sumI_n1 += calcI( Xed_COV, det );
		gsl_matrix_free( Xed_COV );		// this should solve the big memory leak problem
	}

	return( I_n - (I_n + sumI_n1)/n );
}


//---------------------------------------------------------------------------
// calcI
//
// Calculates Integration, I(X), from the covariance matrix and its determinant.
//---------------------------------------------------------------------------
double calcI( gsl_matrix* COV, double det )
{
#if Fix_I
	double sum_Hxi = 0.0;
	for( size_t i = 0; i < COV->size1; i++ )
		sum_Hxi += c_log( gsl_matrix_get( COV, i, i ) );
	return( 0.5 * (sum_Hxi  -  c_log( det )) );
#else
	return( -0.5 * c_log( det ) );
#endif
}


//---------------------------------------------------------------------------
// Calculate I_k, the Integration of a k-sized subset of the full matrix
// of random variables X, based on the provided:
//    COV - the covariance matrix of the full X
//    indexes - the randomly selected subset of k random variables from X
//---------------------------------------------------------------------------
double calcI_k( gsl_matrix* COV, int* indexes, int k )
{
	gsl_matrix* COV_k =  matrix_crosssection( COV, indexes, k );
	double det = determinant( COV_k );
#if Fix_I
	double sum_Hxi = 0.0;
	for( size_t i = 0; i < COV_k->size1; i++ )
		sum_Hxi += c_log( gsl_matrix_get( COV_k, i, i ) );
	gsl_matrix_free( COV_k );
	
	return( 0.5 * (sum_Hxi  -  c_log( det )) );
#else
	return( -0.5 * c_log( det ) );
#endif
}


//---------------------------------------------------------------------------
// CalcComplexityWithMatrix
//---------------------------------------------------------------------------
double CalcComplexityWithMatrix( gsl_matrix* data )
{
	return( CalcApproximateFullComplexityWithMatrix( data, 1 ) );
}


//---------------------------------------------------------------------------
// CalcApproximateFullComplexityWithMatrix
//
// numPoints is the number of subset sizes k at which Ck(X) is calculated
//---------------------------------------------------------------------------
double CalcApproximateFullComplexityWithMatrix( gsl_matrix* data, int numPoints )
{
	double complexity = 0.0;
	
    // if have an invalid matrix return 0.
    if( data == NULL )
    	return complexity;

    gsl_matrix* o = data;

	// Inject a little bit of noise into the data matrix
	gsl_rng *randNumGen = create_rng( DEFAULT_SEED );
	
	for( size_t i = 0; i < data->size1; i++ )
		for( size_t j = 0; j < data->size2; j++ )
			gsl_matrix_set( data, i, j, gsl_matrix_get( data, i, j ) + 0.00001*gsl_ran_ugaussian( randNumGen ) );	// we can do smaller values

	dispose_rng( randNumGen );
	
	// If we're GSAMP'ing, do that now.
    if( Gaussianize )
    {
		size_t numrows = data->size1;
		size_t numcols = data->size2;
	
		// convert to gsl_matrix_view only for compatibility with gsamp()'s use in other complexity modules
		gsl_matrix_view data_view = gsl_matrix_submatrix( data, 0, 0, numrows, numcols );
		o = gsamp( data_view );	// allocates and returns new gsl_matrix
	}
	
	// We calculate the covariance matrix and use it to compute Complexity.
	gsl_matrix* COV = calcCOV( o );
	size_t n = COV->size1;	// same as size2

    if( Gaussianize )
		gsl_matrix_free( o );	// free this iff we allocated it (don't free data)

	double det = determinant( COV );
	double I_n = calcI( COV, det );
#if RescaleCOV
	rescaleCOV( COV, det );
#endif

	if( numPoints <= 0 || (size_t) numPoints >= n )
		numPoints = n-1;		// zero (or invalid value) means use all (non-zero) points

	if( numPoints == 1 )	// use just the k=N-1 point, which is the original simplified TSE complexity
	{
		complexity = calcC_nm1( COV, I_n );
	}
	else if( (size_t) numPoints < n )
	{
		double delta_k = (float)(n-2) / (numPoints - 1);
		calcC_k_print( "----- n=%ld, np=%d, dk=%g -----\n", n, numPoints, delta_k );
		
		// The zero term, C_k where k = 0, is always 0.0.
		calcC_k_print( "%d: C_%d = %g, dk = %d, dc = %g, c = %g\n", 0, 0, 0.0, 0, 0.0, 0.0 );
		
		// The first non-zero term, C_k where k = 1, has zero EI_k, so is purely determined by I_n / n.
		// It contributes to the integral through the initial triangular area defined by it and the k=0 term.
		int k = 1;
		int dk = 1;
		double c_k = I_n / n;						// first non-zero term (c_1)
		double delta_c = dk * 0.5 * c_k;			// area of left-most triangle
		complexity = delta_c;
		calcC_k_print( "%d: C_%d = %g, dk = %d, dc = %g, c = %g\n", 1, k, c_k, dk, complexity, complexity );
		
		// The second through (n-2) terms, C_k where k = 2,...,n-2, must be calculated from LI_k - EI_k.
		// Each term is integrated into C via a quadrilateral area between the current and previous points.
		// As explained in calcC_k(), LI_k is the linearly interpolated I(X) and EI_k is the expected
		// value of actual I_k.
		int k_prev = 1;
		double c_prev = c_k;
		double float_k = 1.0;
		for( int i = 2; i < numPoints; i++ )
		{
			float_k += delta_k;
			k = lround( float_k );
			c_k = calcC_k( COV, I_n, k );
			dk = k - k_prev;
			delta_c = dk * 0.5 * (c_k + c_prev);	// area of next quadrilateral
			complexity += delta_c;
			calcC_k_print( "%d: C_%d = %g, dk = %d, dc = %g, c = %g\n", i, k, c_k, dk, delta_c, complexity );
			k_prev = k;
			c_prev = c_k;
		}
		
		// The (n-1) term, C_k where k = n-1, is calculated specially, accurately, and is the
		// same as the original simplified TSE (which is Olbrich's version of excess entropy).
		k = n - 1;
		dk = k - k_prev;
		c_k = calcC_nm1( COV, I_n );				// last non-zero term (c_nm1) (nm1 == n-1)
		delta_c = dk * 0.5 * (c_k + c_prev);		// area of final quadrilateral
		complexity += delta_c;
		calcC_k_print( "%d: C_%d = %g, dk = %d, dc = %g, c = %g\n", numPoints, k, c_k, dk, delta_c, complexity );
		k_prev = k;
		c_prev = c_k;
		
		// The final, n term, C_k where k = n, is always 0.0, but still contributes to the
		// integral through the final triangular area defined by it and the (n-1) term.
		k = n;
		c_k = 0.0;
		dk = k - k_prev;
		delta_c = dk * 0.5 * (c_k + c_prev);		// area of right-most triangle
		complexity += delta_c;
		calcC_k_print( "%d: C_%d = %g, dk = %d, dc = %g, c = %g\n", numPoints+1, k, c_k, dk, delta_c, complexity );
		complexity /= n;	// based on Olbrich et al 2008, How should complexity scale with system size?, Eur. Phys. J. B
	}

	gsl_matrix_free( COV );

	return( complexity );
}


//---------------------------------------------------------------------------
// CalcComplexityWithVector
//
// Vector is broken up into blocks of length blockDuration points,
// separated by blockOffset points, to create a gsl_matrix
//---------------------------------------------------------------------------
double CalcComplexityWithVector( gsl_vector* vector, size_t blockDuration, size_t blockOffset )
{
	return( CalcApproximateFullComplexityWithVector( vector, blockDuration, blockOffset, 1 ) );
}


//---------------------------------------------------------------------------
// CalcApproximateFullComplexityWithVector
//
// Vector is broken up into blocks of length blockDuration points,
// separated by blockOffset points, to create a gsl_matrix
//
// numPoints is the number of subset sizes k at which Ck(X) is calculated
//---------------------------------------------------------------------------
double CalcApproximateFullComplexityWithVector( gsl_vector* vector, size_t blockDuration, size_t blockOffset, int numPoints )
{
    // if have an invalid vector, return 0.
    if( vector == NULL )
    	return 0.0;
    
    // Determine the number of blocks (columns; random variables)
	size_t numBlocks = (vector->size - blockDuration) / blockOffset;
	
	// Allocate the matrix
	gsl_matrix* m = gsl_matrix_alloc( blockDuration, numBlocks );
	
	// Populate the matrix, with each block as a random variable (column)
	for( size_t col = 0; col < numBlocks; col++ )
	{
		size_t i = col * blockOffset;
		
		for( size_t row = 0; row < blockDuration; row++, i++ )
		{
			gsl_matrix_set( m, row, col, vector->data[i] );
		}
	}

    double complexity = CalcApproximateFullComplexityWithMatrix( m, numPoints );

	gsl_matrix_free( m );

	return( complexity );
}


// eof

