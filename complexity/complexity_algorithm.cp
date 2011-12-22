#include "complexity_algorithm.h"
#include "next_combination.h"

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
// n_choose_k_le_s()
//
// This is an efficient way to determine if <n choose k> is <= s.
// It multiplies the terms from the largest to the smallest and stops
// once s is exceeded, which will usually happen within a term or two.
// It is used to determine whether to sample or exhaustively compute C_k.
//
// WARNING:  This is only accurate out to about <N choose k> < 10^13 or so,
// but this should be more than adequate for our purposes.
//---------------------------------------------------------------------------
bool n_choose_k_le_s( int n, int k, int s )
{
	bool less_equal = true;
	double product = 1.0;

	for( int i = 1; i < k+1; i++ )
	{
		product *= (n - (k-i)) / (double) i;
		if( product > s )
		{
			less_equal = false;
			break;
		}
	}

	return( less_equal );
}


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


//---------------------------------------------------------------------------
// sort_compare_double()
//
// Sorts a one-dimensional (or higher) array of doubles in ascending order
// by the only (or first) element in the array
//---------------------------------------------------------------------------
int sort_compare_double( const void* a, const void* b )
{
	if( *(double*)a < *(double*)b )
		return -1;
	else if( *(double*)a > *(double*)b )
		return 1;
	else
		return 0;
}


//---------------------------------------------------------------------------
// gsamp() function, a re-implementation of gsamp.m
// 
// Description: Creates a Gaussian timeseries with same rank order as the input matrix
// 
// Note: For efficiency, this gsamp() function is slightly different from gsamp.m.
// It receives a tranpose and outputs a transpose of the matrix received/outputted
// by the MATLAB gsamp(). I.e. for  A = some matrix  B = gsamp(A)
// 	 MATLAB: B' = gsamp(A')
// 	    C++: B  = gsamp(A)
//---------------------------------------------------------------------------
void gsamp( gsl_matrix* m )
{
	int r = m->size1;
	int c = m->size2;

	if( r < c )
	{
		cerr << "gsamp: There are more cols than rows.  gsamp() requires that each neuron be a column.  We could do a tranpose here and continue, but considering the rest of the code this situation should be impossible as the matrix is already aligned.  So, there's probably something deeply wrong -- exiting.";
		exit(1);
	}

	gsl_rng *randNumGen = create_rng( DEFAULT_SEED );

	double data[r][2];	// array of structs to do the sorting (raw data, indexes)
	double gauss[r];
	
	// replace each column (neuron) of raw data with equivalent rank-ordered Gaussian series
	for( int j = 0; j < c; j++ )
	{
		// put the original, raw data into the first element of data
		// and the index into the second element of data
		for( int i = 0; i < r; i++ )
		{
			data[i][0] = gsl_matrix_get( m, i, j );
			data[i][1] = i;
		}

		// sort data based on the data (first element), which also shuffles indexes
		qsort( data, r, 2*sizeof(double), sort_compare_double );
		
		// create a random Gaussian series and sort it
		for( int i = 0; i < r; i++ )
			gauss[i] = gsl_ran_ugaussian( randNumGen );
			
		qsort( gauss, r, sizeof(double), sort_compare_double );

		// store the rank-ordered Gaussian series back into the current column of matrix
		// using the shuffled indexes to restore original series data order
		for( int i = 0; i < r; i++ )
			gsl_matrix_set( m, (int)round(data[i][1]), j, gauss[i] );	// round() is only to be super safe
	}
	
	dispose_rng( randNumGen );
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


//---------------------------------------------------------------------------
// matrix_subset_col() takes 3 arguments:
// 	1. The input matrix.
// 	2. The array of column indexes you wish to retain (must be in ascending order).
// 	   All values in the array must be between [0,N-1] where N is the number of
//     columns in the input matrix (->size2).
// 	3. The length of array from #2
// 
// 	The returned matrix will be of size mInput->size1 x numColumns.
//---------------------------------------------------------------------------
gsl_matrix* matrix_subset_col( gsl_matrix* mInput, int* columns, int numColumns )
{
	gsl_matrix* mOutput = gsl_matrix_alloc( mInput->size1, numColumns );
	gsl_vector* column = gsl_vector_alloc( mInput->size1 );

	for( int col = 0; col < numColumns; col++ )
	{
		gsl_matrix_get_col( column, mInput, columns[col] );
		gsl_matrix_set_col( mOutput, col, column );
	}
	
	gsl_vector_free( column );

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
// of all I_k_i subsets of size k (or, usually, some number
// of samples of I_k_i).
//---------------------------------------------------------------------------
double calcC_k( gsl_matrix* COV, double I_n, int k )
{
	#define NumSamples 1000
	
	int n = COV->size1;

	if( k == n-1 )	// next to last term, which is the usual "simplified TSE"
		return( calcC_k_exact( COV, I_n, k ) );
	else if( k == 1 )	// second term
		return( I_n / n );
	else if( k == 0 || k == n )	// first or last term
		return( 0.0 );
	else if( n_choose_k_le_s( n, k, NumSamples ) )	// < NumSamples terms, so calculate exactly
		return( calcC_k_exact( COV, I_n, k ) );
	
	// If we reach here, we are dealing with the general case (k > 1 and k < n-1 and many terms)

	// Calculate the linear, unstructured subset of I_n
	double LI_k = I_n * k / n;
	
	// We will not even try to compute the true expected value of I_k, <I(X_k)>,
	// as the number of subsets, N_choose_k, grows to astronomical values.
	// Instead we approximate it with a modest number of samples.

	gsl_rng *randNumGen = create_rng( DEFAULT_SEED );

	int indexes[k];
	double EI_k = 0.0;
	
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
		
		EI_k += CalcI_k( COV, indexes, k );
	}
	
	dispose_rng( randNumGen );

	EI_k /= NumSamples;
	
	return( LI_k - EI_k );
}


//---------------------------------------------------------------------------
// Calculate C_k for k = n - 1
//---------------------------------------------------------------------------
// double calcC_nm1( gsl_matrix* COV, double I_n )
// {
// 	int n = COV->size1;
// 
// 	double sumI_nm1 = 0;
// 
// 	for( int i = 0; i < n; i++ )
// 	{
// 		int b[n-1];
// 		int b_length = n-1;
// 
// 		for( int j = 0; j < n; j++ )
// 		{
// 			if( j < i )
// 				b[j] = j;		
// 			else if( j > i )
// 				b[j-1] = j;
// 		
// 		}
// 
// 		//Technically we don't have to store this array, but for now lets stay consistent with the MATLAB code
// 		gsl_matrix* xCOV =  matrix_crosssection( COV, b, b_length );
// 		double det = determinant( xCOV );
// 		sumI_nm1 += CalcI( xCOV, det );
// 		gsl_matrix_free( xCOV );		// this should solve the big memory leak problem
// 	}
// 
// 	return( I_n - (I_n + sumI_nm1)/n );
// }


//---------------------------------------------------------------------------
// Calculate C_k exactly
//
// Uses all subsets of size k from set of size n.
// next_combination() does the work of shuffling indexes so that the first
// k indexes of the index[] array exhaustively cycle through all subsets.
//---------------------------------------------------------------------------
double calcC_k_exact( gsl_matrix* COV, double I_n, int k )
{
	int n = COV->size1;
	int index[n];
	
	for( int i = 0; i < n; i++ )
		index[i] = i;

	double sumI_k = 0;
	int n_choose_k = 0;

	do
	{
		gsl_matrix* xCOV =  matrix_crosssection( COV, index, k );
		double det = determinant( xCOV );
		sumI_k += CalcI( xCOV, det );
		gsl_matrix_free( xCOV );
		
		n_choose_k++;
	}
	while( next_combination( index, index+k, index+n ) );
	
// 	printf( "Used %s, n=%d, k=%d, n_choose_k=%d, C_k=%g\n",
// 			__func__, n, k, n_choose_k, (I_n * k / n  -  sumI_k / n_choose_k) );

	return( I_n * k / n  -  sumI_k / n_choose_k );
}


//---------------------------------------------------------------------------
// CalcI
//
// Calculates Integration, I(X), from the covariance matrix and its determinant.
//---------------------------------------------------------------------------
double CalcI( gsl_matrix* COV, double det )
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
// Calculate I_k, the Integration of a specific k-sized subset of the full
// matrix of random variables X, based on the provided:
//    COV - the covariance matrix of the full X
//    indexes - the randomly selected subset of k random variables from X
//---------------------------------------------------------------------------
double CalcI_k( gsl_matrix* COV, int* indexes, int k )
{
	gsl_matrix* COV_k =  matrix_crosssection( COV, indexes, k );
	double det = determinant( COV_k );
	double I_k = CalcI( COV_k, det );
	gsl_matrix_free( COV_k );
	return( I_k );
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
    {
    	fprintf( stderr, "\n%s passed NULL data matrix\n", __func__ );
    	return complexity;
	}
	
	// Make room for a copy of the data, that we will add noise to and possibly Gaussianize
    gsl_matrix* m = gsl_matrix_alloc( data->size1, data->size2 );

	// Inject a little bit of noise into the data matrix
	gsl_rng *randNumGen = create_rng( DEFAULT_SEED );
	
	double noise_scale = 0.00001;
	for( size_t i = 0; i < data->size1; i++ )
		for( size_t j = 0; j < data->size2; j++ )
			gsl_matrix_set( m, i, j, gsl_matrix_get( data, i, j ) + noise_scale*gsl_ran_ugaussian( randNumGen ) );

	// If we're GSAMP'ing, do that now.
    if( Gaussianize )
		gsamp( m );	// replaces original columns by rank-equivalent Gaussian distributions

	gsl_matrix* COV;
	size_t n;
	double det;
	
	do
	{
		// We calculate the covariance matrix and use it to compute Complexity.
		COV = calcCOV( m );
		n = COV->size1;	// same as size2
	
		det = determinant( COV );
		
		// If the determinant is zero, add more noise
		//
		// Being done after gsamp makes this somewhat inconsistent, but it happens extremely rarely,
		// and then only for event-filtered complexities, in my experience so far.
		// Something like this is needed to eliminate those very rare zero determinants.
		
		if( det == 0.0 )
		{
			if( noise_scale*10.0 < 1.1 )
			{
				noise_scale *= 10.0;
				//cout << "Adding extra noise at scale " << noise_scale << endl;
				for( size_t i = 0; i < data->size1; i++ )
					for( size_t j = 0; j < data->size2; j++ )
						gsl_matrix_set( m, i, j, gsl_matrix_get( m, i, j ) + noise_scale*gsl_ran_ugaussian( randNumGen ) );
			}
			else
			{
				//cerr << "Adding noise with scale " << noise_scale << " was insufficient to eliminate zero determinant" << endl;
				det = 1.e-250;
			}
		}
	}
	while( det == 0.0 );

	double I_n = CalcI( COV, det );
	
#if RescaleCOV
	rescaleCOV( COV, det );
#endif

	gsl_matrix_free( m );
	dispose_rng( randNumGen );
	
	if( numPoints <= 0 || (size_t) numPoints >= n )
		numPoints = n-1;		// zero (or invalid value) means use all (non-zero) points

	if( numPoints == 1 )	// use just the k=N-1 point, which is the original simplified TSE complexity
	{
		complexity = calcC_k_exact( COV, I_n, n-1 );
	}
	else if( (size_t) numPoints < n )
	{
		double delta_k = (float)(n-2) / (numPoints - 1);
		calcC_k_print( "----- n=%ld, np=%d, dk=%g -----\n", n, numPoints, delta_k );
		
		// The zero term, C_k where k = 0, is always 0.0.
		calcC_k_print( "%d: I_%d = %g, C_%d = %g, dk = %d, dc = %g, c = %g\n", 0, 0, 0.0, 0, 0.0, 0, 0.0, 0.0 );
		
		// The first non-zero term, C_k where k = 1, has zero EI_k, so is purely determined by I_n / n.
		// It contributes to the integral through the initial triangular area defined by it and the k=0 term.
		int k = 1;
		int dk = 1;
		double c_k = I_n / n;						// first non-zero term (c_1)
		double delta_c = dk * 0.5 * c_k;			// area of left-most triangle
		complexity = delta_c;
		calcC_k_print( "%d: I_%d = %g, C_%d = %g, dk = %d, dc = %g, c = %g\n", 1, k, I_n/n, k, c_k, dk, complexity, complexity );
		
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
			calcC_k_print( "%d: I_%d = %g, C_%d = %g, dk = %d, dc = %g, c = %g\n", i, k, I_n*i/n, k, c_k, dk, delta_c, complexity );
			k_prev = k;
			c_prev = c_k;
		}
		
		// The (n-1) term, C_k where k = n-1, is calculated specially, accurately, and is the
		// same as the original simplified TSE (which is Olbrich's version of excess entropy).
		k = n - 1;
		dk = k - k_prev;
		c_k = calcC_k_exact( COV, I_n, k );			// last non-zero term
		delta_c = dk * 0.5 * (c_k + c_prev);		// area of final quadrilateral
		complexity += delta_c;
		calcC_k_print( "%d: I_%d = %g, C_%d = %g, dk = %d, dc = %g, c = %g\n", numPoints, k, I_n*(n-1)/n, k, c_k, dk, delta_c, complexity );
		k_prev = k;
		c_prev = c_k;
		
		// The final, n term, C_k where k = n, is always 0.0, but still contributes to the
		// integral through the final triangular area defined by it and the (n-1) term.
		k = n;
		c_k = 0.0;
		dk = k - k_prev;
		delta_c = dk * 0.5 * (c_k + c_prev);		// area of right-most triangle
		complexity += delta_c;
		calcC_k_print( "%d: I_%d = %g, C_%d = %g, dk = %d, dc = %g, c = %g\n", numPoints+1, k, I_n, k, c_k, dk, delta_c, complexity );
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

