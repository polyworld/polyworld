#include "complexity.h"

#include <iostream>

using namespace std;

//---------------------------------------------------------------------------
// create_rng()
//
// constructs a random number generator and seeds it
//---------------------------------------------------------------------------
gsl_rng *create_rng(int seed)
{
	gsl_rng *rng = gsl_rng_alloc(gsl_rng_mt19937);
	gsl_rng_set(rng, seed);

	return rng;
}

//---------------------------------------------------------------------------
// dispose_rng()
//
// disposes a random number generator
//---------------------------------------------------------------------------
void dispose_rng(gsl_rng *rng)
{
	gsl_rng_free(rng);
}


int qsort_compare_double( const void *a, const void *b )
{
	if( *(double*)a < *(double*)b ) { return -1; }
	else if( *(double*)a == *(double*)b ) { return 0; }
	else { return 1; }
}

int qsort_compare_rows0( const void *a, const void *b )
{
/* Sorts gsl_vectors in ascending order by their first entry (entry 0) */
	if(  gsl_vector_get( *(gsl_vector**)a, 0 )  <  gsl_vector_get( *(gsl_vector**)b, 0 )  ) { return -1; }
	else if( gsl_vector_get( *(gsl_vector**)a, 0) == gsl_vector_get( *(gsl_vector**)b, 0) ) { return 0; }
	else { return 1; }
}

int qsort_compare_rows1( const void *a, const void *b )
{
/* Sorts gsl_vectors in ascending order by their second entry (entry 1) */
	if(  gsl_vector_get( *(gsl_vector**)a, 1 )  <  gsl_vector_get( *(gsl_vector**)b, 1 )  ) { return -1; }
	else if( gsl_vector_get( *(gsl_vector**)a, 1) == gsl_vector_get( *(gsl_vector**)b, 1) ) { return 0; }
	else { return 1; }
}

/*
gsamp() function, a re-implementation of gsamp.m

Description: Creates a timeseries with same rank order as the input but with Gaussian amp.

Note: For efficiency, this gsamp() function is slightly different from gsamp.m.  It receives a tranpose and outputs a transpose of the matrix received/outputted by the MATLAB gsamp(). I.e.     A = some matrix      B = gsamp(A)
	MATLAB: B' = gsamp(A')
	C++: B = gsamp(A)
*/
gsl_matrix * gsamp( gsl_matrix_view x )
{

/* MATLAB:	[r,c] = size(x);	*/
	int r = (&x.matrix)->size1;
	int c = (&x.matrix)->size2;


/* MATLAB:
	if r < c
		x = x.';
	end;
*/

	if( r < c )
	{
		cerr << "gsamp: There are more cols than rows.  gsamp() requires that each neuron be a column.  We could do a tranpose here and continue, but considering the rest of the code this situation should be impossible as the matrix is already aligned.  So, there's probably something deeply wrong -- exiting.";
		exit(1);
	}


/* MATLAB: [n, cc] = size(x); */
	int n  = (&x.matrix)->size1;
	int cc = (&x.matrix)->size2;

/* MATLAB: m = 2^nextpow2(n)*/
//	int m = int( pow(2,  int(ceil(log2(n)))  ) );

/* MATLAB: yy=zeros(n,cc); 		Here there is a bug in the MATLAB code.  It should say 'y', not 'yy'*/
	gsl_matrix * y = gsl_matrix_calloc( n, cc );

/* MATLAB:
	for i=1:cc	%create a gaussian timeseries with the same rank-order of x
		z=zeros(n,3); gs = sortrows( randn(n,1) , 1 );
		z(:,1)=x(:,i); z(:,2)=[1:n]'; z=sortrows(z,1);
		z(:,3)=gs; z=sortrows(z,2); y(:,i)=z(:,3);
	end
*/
	gsl_rng *randNumGen = create_rng(DEFAULT_SEED);
	
	// this is nessecary for switching the columns around among the x, z, and y matrices
	gsl_vector * tempcol = gsl_vector_alloc(n);

	for( int i=0; i<cc; i++ )
	{
//MATLAB	z=zeros(n,3);
		gsl_matrix * z = gsl_matrix_calloc( n, 3 );

//MATLAB	gs = sortrows( randn(n,1) , 1 );
		double gs[n]; for( int j=0; j<n; j++ ) { gs[j] = gsl_ran_ugaussian(randNumGen); }
		qsort( gs, n, sizeof(double), qsort_compare_double);

//MATLAB	z(:,1)=x(:,i); z(:,2)=[1:n]'; z=sortrows(z,1);
		gsl_matrix_get_col( tempcol, &x.matrix, i );			// get i'th column of X
		gsl_matrix_set_col( z, 0, tempcol );				// put i'th column of X into z's first column

		// there may be an error here.  In Matlab it's supposed to be [1:n], but this is [0:n-1]
		for( int j=1; j<=n; j++ ) { gsl_matrix_set( z, j-1, 1, j ); }	// z(:,2)=[1:n]';

//		print_matrix_column( z, 0 );

		// now we must sort the rows of z by the first (0th) column.  First must pull out all of the rows, and put them into an array of vectors.
		gsl_vector * zrows[n];
 		for( int j=0; j<n; j++ ) { zrows[j] = gsl_vector_calloc(3); }		// all rows are initially zero.

		for( int j=0; j<n; j++ ) { gsl_matrix_get_row( zrows[j], z, j ); }	// get the rows
//			for( int j=0; j<n; j++ ) { cout << "Row " << j << ", element 0: " << gsl_vector_get( zrows[j], 0 ) << endl; }
		qsort( zrows, n, sizeof(zrows[0]), qsort_compare_rows0 );		// sort the rows by column 0
//			for( int j=0; j<n; j++ ) { cout << "Row " << j << ", element 0: " << gsl_vector_get( zrows[j], 0 ) << endl; }
		for( int j=0; j<n; j++ ) { gsl_matrix_set_row( z, j, zrows[j] ); }	// overwrite z with the sorted rows


//MATLAB        z(:,3)=gs; z=sortrows(z,2); y(:,i)=z(:,3);
		for( int j=0; j<n; j++ ) { gsl_matrix_set( z, j, 2, gs[j] ); }		// z(:,3) = gs;


		// now we must sort matrix z by the second (1'th) column.  As before, pull out all of the rows, call qsort, them put them back in.
		for( int j=0; j<n; j++ ) { gsl_matrix_get_row( zrows[j], z, j ); }	// get the rows
		qsort( zrows, n, sizeof(zrows[0]), qsort_compare_rows1 );		// sort the rows by column 1
		for( int j=0; j<n; j++ ) { gsl_matrix_set_row( z, j, zrows[j] ); }	// overwrite z with the sorted rows

		for( int j=0; j<n; j++ ) { gsl_vector_free( zrows[j] ); }		// done with the temp sorting rows.  set them free.

//		y(:,i)=z(:,3);
		gsl_matrix_get_col( tempcol, z, 2);	// get column 2 from matrix 'z'...
		gsl_matrix_set_col( y, i, tempcol );	// and put the 2nd column of 'z' into the i'th column of matrix 'y'

		gsl_matrix_free( z );			// we're done with our temp matrix z now.
	}
	gsl_vector_free( tempcol );

	dispose_rng(randNumGen);

	if( r < c )
	{
		cout << "end of gsamp(): r < c -- this should be impossible." << endl;
		exit(1);
	}

	return y;
}


gsl_matrix * COVtoCOR( gsl_matrix * COV )
{
/* MATLAB CODE: COR = COV ./ ( sqrt(diag(COV)) * sqrt(diag(COV))'); */

	int N = COV->size1;
	assert( COV->size1 == COV->size2 );		// make sure COV is square.
	
	gsl_matrix * COR = gsl_matrix_alloc( COV->size1, COV->size2 );

	// We use our own function here because GSL's diag returns a vector
	gsl_matrix * sqrt_diag_COV =  mget_matrix_diagonal( COV );	
	gsl_matrix * sqrt_diag_COVtic = gsl_matrix_alloc( N , 1 );

	for( int i=0; i<N; i++ )
	{
		gsl_matrix_set( sqrt_diag_COV, 0, i, sqrt(gsl_matrix_get(sqrt_diag_COV, 0, i)) );	//sqrt() the diagonal
	}

	gsl_matrix_transpose_memcpy( sqrt_diag_COVtic, sqrt_diag_COV );

	
	// To do the matrix multiplication we have to do the whole MatrixView mess again.
	gsl_matrix_view MV_sqrt_diag_COV    = gsl_matrix_submatrix( sqrt_diag_COV   , 0, 0, 1, N);
	gsl_matrix_view MV_sqrt_diag_COVtic = gsl_matrix_submatrix( sqrt_diag_COVtic, 0, 0, N, 1);
	gsl_matrix * product = gsl_matrix_alloc( N, N );

	gsl_blas_dgemm( CblasTrans, CblasTrans, 1.0, &MV_sqrt_diag_COV.matrix, &MV_sqrt_diag_COVtic.matrix, 0.0, product ); //product = sqrt(diag(COV)) * sqrt(diag(COV))'

	gsl_matrix_memcpy( COR, COV );

	gsl_matrix_div_elements( COR, product );

	gsl_matrix_free( sqrt_diag_COV );
	gsl_matrix_free( sqrt_diag_COVtic );
	gsl_matrix_free( product );
	
	return COR;
}


gsl_matrix * mCOV( gsl_matrix * M )
{
/*
The input matrix may not be square, but the output will always be square NxN matrix where N is the number of columns in the input matrix. 
*/
	gsl_matrix * COV = gsl_matrix_alloc( M->size2, M->size2);

	double array_col_i[M->size1];	// The GSL covariance function takes arrays
	double array_col_j[M->size1];	// The GSL covariance function takes arrays

	for( unsigned int i=0; i<M->size2; i++ )
	{
		for( unsigned int j=0; j<=i; j++ )	// We only goto <= i because we need to only calculate covariance for half of the matrix.
		{
			gsl_vector_view col_i = gsl_matrix_column( M, i );
			gsl_vector_view col_j = gsl_matrix_column( M, j );

//DEBUG			cout << "::Size of column " << i << " = " << (&col_i.vector)->size << endl;
//DEBUG			cout << "::Size of column " << j << " = " << (&col_j.vector)->size << endl;

			for( unsigned int count=0; count<M->size1; count++ )	// Time to convert vectors to arrays
			{
				array_col_i[count] = gsl_vector_get( &col_i.vector, count);
				array_col_j[count] = gsl_vector_get( &col_j.vector, count);
			}

//DEBUG			cout << "Covariance[" << i << "," << j	<< "] = " << gsl_stats_covariance( array_col_i, 1, array_col_j, 1, M->size1 ) << endl;
			gsl_matrix_set( COV, i, j, gsl_stats_covariance( array_col_i, 1, array_col_j, 1, M->size1 ) ); 
		}
	}


	for( unsigned int i=0; i<M->size2; i++ )
	{
		// Many values in the matrix are repeated so we can just fill those in instead of recalculating them.
		for( unsigned int j=i+1; j<M->size2; j++ )
		{
			gsl_matrix_set( COV, i, j, gsl_matrix_get(COV, j, i) );
		}
	}

	return COV;
}


bool is_matrix_square( gsl_matrix * M )
{
	if( M->size1 == M->size2 ) { return true; }
	else                       { return false; }
}

gsl_matrix * calcCOV( gsl_matrix * CIJ, double r )
/*
Calculates a COVariance matrix based on these assumptions:
1) linearity
2) stationarity
3) Gaussian multivariate process

Intrinsic (uncorrelated) noise level is set to r.
*/
{
/* Some basic checking not done in the MATLAB Code */

	if( ! is_matrix_square(CIJ) )
	{
		cerr << "Error in calcCOV()! Input matrix must be square!  Dimenions are: " << CIJ->size1 << "x" << CIJ->size2<< endl;
		exit(-1);
	}
	

/* MATLAB CODE: N = size(CIJ,1); */
	unsigned int N = CIJ->size1;

/* MATLAB CODE: 
	if( nnz( sum(CIJ)>1 ) >0 )
	    disp(['Warning: column sum of connection weights exceeds 1']);
	end;
*/
	for( unsigned int col=0; col<N; col++)
	{
        	double sum_col=0;
		gsl_vector_view col_i = gsl_matrix_column( CIJ, col );
		for( unsigned int temp=0; temp < N; temp++)
		{
			sum_col += gsl_vector_get( &col_i.vector, temp);
		}

		if( sum_col > 1 )
		{
			cerr << "Warning: Sum of connection weights in column " << col << "(0-based) exceeds 1.0 (sum=" << sum_col << ").  This may be true for other columns as well." << endl;
			break ;
		}
	}

/* MATLAB CODE: Q = inv(eye(N) - CIJ); */
	gsl_matrix * eye_N = identity_matrix(N);        // Identity matrix of size N
	gsl_matrix_sub( eye_N, CIJ );                   // Note that eye_N has now been overwritten with the result!!

	//Now to get the inverse...
	gsl_permutation * P = gsl_permutation_alloc(N);
	int signum=1;
	gsl_linalg_LU_decomp( eye_N, P, &signum );	// Note that eye_N has now been (re)overwritten with the LU decomposition!!

	gsl_matrix * Q = gsl_matrix_alloc(N,N);
	gsl_linalg_LU_invert( eye_N, P, Q );		// Q = inv(eye(N) - CIJ)
	gsl_matrix_free( eye_N );

/* Not in the original MATLAB Code */
	gsl_matrix * R = identity_matrix(N);	// In the MATLAB code matrix R is passed.  Here we generate R from r.
	gsl_matrix_scale(R,r);			// R = Identity Matrix * r

/* MATLAB CODE: COV = Q'*R'*R*Q; % aka: (Q')*(R')*R*Q  */
	gsl_matrix * Qtic = gsl_matrix_alloc(N,N);
	gsl_matrix_transpose_memcpy( Qtic, Q) ;		// Qtic = Q'

	gsl_matrix * Rtic = gsl_matrix_alloc(N,N);
	gsl_matrix_transpose_memcpy( Rtic, R );		// Rtic = R'

	gsl_matrix * COV = gsl_matrix_alloc(N,N);

	// To do matrix multiplication, we have to make some gsl_matrix_view's ...
	gsl_matrix_view MV_Qtic = gsl_matrix_submatrix( Qtic, 0, 0, N, N);
	gsl_matrix_view MV_Rtic = gsl_matrix_submatrix( Rtic, 0, 0, N, N);
	gsl_matrix_view MV_R = gsl_matrix_submatrix( R, 0, 0, N, N);
	gsl_matrix_view MV_Q = gsl_matrix_submatrix( Q, 0, 0, N, N);
	gsl_matrix_view MV_COV = gsl_matrix_submatrix( COV, 0, 0, N, N);

	gsl_blas_dgemm( CblasNoTrans, CblasNoTrans, 1.0, &MV_Qtic.matrix, &MV_Rtic.matrix, 0.0, &MV_COV.matrix ); //COV = Q'*R'
	gsl_blas_dgemm( CblasNoTrans, CblasNoTrans, 1.0, &MV_COV.matrix, &MV_R.matrix, 0.0, &MV_Qtic.matrix );    //Qtic = Q'*R'*R
	gsl_blas_dgemm( CblasNoTrans, CblasNoTrans, 1.0, &MV_Qtic.matrix, &MV_Q.matrix, 0.0, &MV_COV.matrix );    //COV = Q'*R'*R*Q

	gsl_matrix_free(Q);
	gsl_matrix_free(Qtic);
	gsl_matrix_free(R);
	gsl_matrix_free(Rtic);

/* MATLAB CODE: COR = COV ./ ( sqrt(diag(COV)) * sqrt(diag(COV))'); */

	// In this version of calcCOV() we do not compute the COR matrix.

	return COV;
}


gsl_matrix * calcCOV( gsl_matrix * CIJ, double r, gsl_matrix * COR )
/*
Calculates a COVariance and CORrelation matrix based on these assumptions:
1) linearity
2) stationarity
3) Gaussian multivariate process

Intrinsic (uncorrelated) noise level is set to r.
*/
{
/* Some basic checking not done in the MATLAB Code */

	if( ! is_matrix_square(CIJ) )
	{
		cerr << "Error in calcCOV()! Matrix CIJ (first parameter) is not square!  Dimenions are: " << CIJ->size1 << "x" << CIJ->size2<< endl;
		exit(-1);
	}

	if( ! is_matrix_square(COR) )
	{

		cerr << "Error in calcCOV()! Matrix COR (first parameter) is not square!  Dimenions are: " << COR->size1 << "x" << COR->size2<< endl;
		exit(-1);
	}

	// We have already checked that the matrices are square so we only need to check size1
	if( CIJ->size1 != COR->size1 )
	{
		cerr << "Matrix CIJ and COR must be of the same size" << endl;
		exit(-1);
	}
	

/* MATLAB CODE: N = size(CIJ,1); */
	unsigned int N = CIJ->size1;

/* MATLAB CODE: 
	if( nnz( sum(CIJ)>1 ) >0 )
	    disp(['Warning: column sum of connection weights exceeds 1']);
	end;
*/
	for( unsigned int col=0; col<N; col++)
	{
        	double sum_col=0;
		gsl_vector_view col_i = gsl_matrix_column( CIJ, col );
		for( unsigned int temp=0; temp < N; temp++)
		{
			sum_col += gsl_vector_get( &col_i.vector, temp);
		}

		if( sum_col > 1 )
		{
			cerr << "Warning: Sum of connection weights in column " << col << "(0-based) exceeds 1.0 (sum=" << sum_col << ").  This may be true for other columns as well." << endl;
			break ;
		}
	}

/* MATLAB CODE: Q = inv(eye(N) - CIJ); */
	gsl_matrix * eye_N = identity_matrix(N);        // Identity matrix of size N
	gsl_matrix_sub( eye_N, CIJ );                   // Note that eye_N has now been overwritten with the result!!

	//Now to get the inverse...
	gsl_permutation * P = gsl_permutation_alloc(N);
	int signum=1;
	gsl_linalg_LU_decomp( eye_N, P, &signum );	// Note that eye_N has now been (re)overwritten with the LU decomposition!!

	gsl_matrix * Q = gsl_matrix_alloc(N,N);
	gsl_linalg_LU_invert( eye_N, P, Q );		// Q = inv(eye(N) - CIJ)
	gsl_matrix_free( eye_N );

/* Not in the original MATLAB Code */
	gsl_matrix * R = identity_matrix(N);	// In the MATLAB code matrix R is passed.  Here we generate R from r.
	gsl_matrix_scale(R,r);			// R = Identity Matrix * r

/* MATLAB CODE: COV = Q'*R'*R*Q; % aka: (Q')*(R')*R*Q  */
	gsl_matrix * Qtic = gsl_matrix_alloc(N,N);
	gsl_matrix_transpose_memcpy( Qtic, Q) ;		// Qtic = Q'

	gsl_matrix * Rtic = gsl_matrix_alloc(N,N);
	gsl_matrix_transpose_memcpy( Rtic, R );		// Rtic = R'

	gsl_matrix * COV = gsl_matrix_alloc( N, N );
	// To do matrix multiplication, we have to make some gsl_matrix_view's ...
	gsl_matrix_view MV_Qtic = gsl_matrix_submatrix( Qtic, 0, 0, N, N);
	gsl_matrix_view MV_Rtic = gsl_matrix_submatrix( Rtic, 0, 0, N, N);
	gsl_matrix_view MV_R = gsl_matrix_submatrix( R, 0, 0, N, N);
	gsl_matrix_view MV_Q = gsl_matrix_submatrix( Q, 0, 0, N, N);
	gsl_matrix_view MV_COV = gsl_matrix_submatrix( COV, 0, 0, N, N);

	gsl_blas_dgemm( CblasNoTrans, CblasNoTrans, 1.0, &MV_Qtic.matrix, &MV_Rtic.matrix, 0.0, &MV_COV.matrix ); //COV = Q'*R'
	gsl_blas_dgemm( CblasNoTrans, CblasNoTrans, 1.0, &MV_COV.matrix, &MV_R.matrix, 0.0, &MV_Qtic.matrix );    //Qtic = Q'*R'*R
	gsl_blas_dgemm( CblasNoTrans, CblasNoTrans, 1.0, &MV_Qtic.matrix, &MV_Q.matrix, 0.0, &MV_COV.matrix );    //COV = Q'*R'*R*Q

	gsl_matrix_free(Q);
	gsl_matrix_free(Qtic);
	gsl_matrix_free(R);
	gsl_matrix_free(Rtic);

/* MATLAB CODE: COR = COV ./ ( sqrt(diag(COV)) * sqrt(diag(COV))'); */

	// We use our own function here because GSL's diag returns a vector
	gsl_matrix * sqrt_diag_COV = mget_matrix_diagonal( COV );	
	gsl_matrix * sqrt_diag_COVtic = gsl_matrix_alloc(N,1);

	for( unsigned int i=0; i<N; i++ )
	{
		gsl_matrix_set( sqrt_diag_COV, 0, i, sqrt(gsl_matrix_get(sqrt_diag_COV, 0, i)) );	//sqrt() the diagonal
	}

	gsl_matrix_transpose_memcpy( sqrt_diag_COVtic, sqrt_diag_COV );

	// To do the matrix multiplication we have to do the whole MatrixView mess again.
	gsl_matrix_view MV_sqrt_diag_COV    = gsl_matrix_submatrix( sqrt_diag_COV   , 0, 0, 1, N);
	gsl_matrix_view MV_sqrt_diag_COVtic = gsl_matrix_submatrix( sqrt_diag_COVtic, 0, 0, N, 1);
	gsl_matrix * product = gsl_matrix_alloc( N, N );

	gsl_blas_dgemm( CblasTrans, CblasTrans, 1.0, &MV_sqrt_diag_COV.matrix, &MV_sqrt_diag_COVtic.matrix, 0.0, product ); //product = sqrt(diag(COV)) * sqrt(diag(COV))'

	gsl_matrix_memcpy( COR, COV );
	gsl_matrix_div_elements( COR, product );

	gsl_matrix_free( sqrt_diag_COV );
	gsl_matrix_free( sqrt_diag_COVtic );
	gsl_matrix_free( product );

	return COV;
}


gsl_matrix * mget_matrix_diagonal( gsl_matrix * M )
{
	int N;

	if( M->size1 < M->size2 ) { N = M->size1; }	// in case M is not square
	else                      { N = M->size2; }

	gsl_matrix * diagonal = gsl_matrix_alloc(1,N);	// 1 row, N-columns.

	for( int i=0; i<N; i++ )
	{
		gsl_matrix_set(diagonal, 0, i, gsl_matrix_get(M, i, i) );
	}

	return diagonal;
}

void print_matrix_row( gsl_matrix * M, int row )
{

	gsl_vector_view row_i = gsl_matrix_row( M, row );
	int row_length = (&row_i.vector)->size;

	cout << "Length of row " << row << ": " << row_length << endl;
	cout << "Values in row: ";

	for(int temp=0; temp < row_length; temp++)
	{
		cout << gsl_vector_get( &row_i.vector, temp) << "  ";
	}
	cout << endl;

}

void print_matrix_column( gsl_matrix * M, int col )
{

	gsl_vector_view col_i = gsl_matrix_column( M, col );
	int col_length = (&col_i.vector)->size;

	cout << "Length of col: " << col << ": " << col_length << endl;
	cout << "Values in col: ";

	for(int temp=0; temp < col_length; temp++)
	{
		cout << gsl_vector_get( &col_i.vector, temp) << "  ";
	}
	cout << endl;

}



gsl_matrix * identity_matrix( int N )
{
	gsl_matrix * z = gsl_matrix_calloc( N, N );	// set all elements to zero

	for(int i=0; i<N; i++)
	{
		gsl_matrix_set( z,i,i,1 );		// sets the diagonal to 1's
	}

	return z;
}

double calcC_det3( gsl_matrix * foreignCOR )
{
	gsl_matrix * COR = gsl_matrix_alloc( foreignCOR->size1, foreignCOR->size2 );	// We do this so we don't overwrite the passed matrix COR
	gsl_matrix_memcpy( COR, foreignCOR );

/* MATLAB CODE:
	N = size(COR,1);
*/

	int N = COR->size1;


/* MATLAB CODE:
	d1 = det( COR );
*/

	// These next few lines are nessecary to compute the Determinant of COR.
	gsl_matrix * ludecomp = gsl_matrix_alloc( N, N );
	gsl_matrix_memcpy( ludecomp, COR );		//make a copy so we don't overwrite COR when we decomp
	gsl_permutation * P = gsl_permutation_alloc(N);
	int signum = 1;
	gsl_linalg_LU_decomp( ludecomp, P, &signum );
	double d1 = fabs( gsl_linalg_LU_det( ludecomp, 1 ) );		//fabs() not in MATLAB code, but is nessecary sometimes.
	gsl_permutation_free(P);


/* MATLAB CODE:
	while ~ d1
 	   COR = COR.*1.3;
 	   d1 = det(COR);
	end
*/

	while( d1 == 0 )
	{
		gsl_matrix_scale( COR, 1.3 );
		d1 = fabs( gsl_linalg_LU_det( COR, 1) );	// fabs() not in MATLAB code, but is nessecary sometimes.
	}


	gsl_matrix_scale( COR, 1 / exp(log(d1)/N) );	// MATLAB: COR = COR./exp(log(d1)/N);


/* MATLAB CODE:
	% calculate I at level N
	I_n = calcI_det2(COR);

	% calculate I at level N-1
	I_n1 = zeros(1,N);
	for i=1:N
		vv = ones(1,N);
		vv(i) = 0;
		[a b c] = find(vv==1);
		I_n1(i) = calcI_det2(COR(b,b));
	end;
*/


	double I_n = calcI_det2(COR);
	double I_n1[N];

	for( int i=0;i<N;i++ )
	{
		int b[N-1];
		int b_length = N-1;

//DEBUG		cout << endl << "Value of i: " << i << "\t";

		for( int j=0;j<N;j++ )
		{
			if( j < i )
				b[j] = j;		
			else if( j > i )
				b[j-1] = j;
		
		}

//DEBUG		cout << "b: "; for( int j=0;j<b_length;j++ ) { cout << b[j] << " "; } cout << endl;


		//Technically we don't have to store this array, but for now lets stay consistent with the MATLAB code
		gsl_matrix * Xed_COR =  matrix_crosssection( COR, b, b_length );
		I_n1[i] = calcI_det2( Xed_COR );
		gsl_matrix_free( Xed_COR );		//this should solve the big memory leak problem
	}

	double sumI_n1=0;
	for( int i=0;i<N;i++) { sumI_n1 += I_n1[i]; }


	double C = I_n - I_n/N - sumI_n1/N;
	return C;
}

double calcC_det3__optimized( gsl_matrix * foreignCOR )
{
//	int status;
//	gsl_set_error_handler_off();


	gsl_matrix * COR = gsl_matrix_alloc( foreignCOR->size1, foreignCOR->size2 );	// We do this so we don't overwrite the passed matrix COR
	gsl_matrix_memcpy( COR, foreignCOR );

/* MATLAB CODE:
	N = size(COR,1);
*/

	int N = COR->size1;


/* MATLAB CODE:
	d1 = det( COR );
*/

	// These next few lines are nessecary to compute the Determinant of COR.
	gsl_matrix * ludecomp = gsl_matrix_alloc( N, N );
	gsl_matrix_memcpy( ludecomp, COR );		//make a copy so we don't overwrite COR when we decomp
	gsl_permutation * P = gsl_permutation_alloc(N);
	int signum = 1;
	gsl_linalg_LU_decomp( ludecomp, P, &signum );
	double d1 = fabs( gsl_linalg_LU_det( ludecomp, 1 ) );		//fabs() not in MATLAB code, but is nessecary sometimes.
	gsl_permutation_free(P);
	gsl_matrix_free( ludecomp );					// don't need you anymore


/* MATLAB CODE:
	while ~ d1
 	   COR = COR.*1.3;
 	   d1 = det(COR);
	end
*/
	while( d1 == 0 )
	{
		gsl_matrix_scale( COR, 1.3 );
		d1 = fabs( gsl_linalg_LU_det( COR, 1) );	// fabs() not in MATLAB code, but is nessecary sometimes.
	}


	gsl_matrix_scale( COR, 1 / exp(log(d1)/N) );	// MATLAB: COR = COR./exp(log(d1)/N);


/* MATLAB CODE:
	% calculate I at level N
	I_n = calcI_det2(COR);

	% calculate I at level N-1
	I_n1 = zeros(1,N);
	for i=1:N
		vv = ones(1,N);
		vv(i) = 0;
		[a b c] = find(vv==1);
		I_n1(i) = calcI_det2(COR(b,b));
	end;
*/


	double I_n = calcI_det2(COR);
	double sumI_n1=0;

	for( int i=0;i<N;i++ )
	{
		int b[N-1];
		int b_length = N-1;

//DEBUG		cout << endl << "Value of i: " << i << "\t";

		for( int j=0;j<N;j++ )
		{
			if( j < i )
				b[j] = j;		
			else if( j > i )
				b[j-1] = j;
		
		}

//DEBUG		cout << "b: "; for( int j=0;j<b_length;j++ ) { cout << b[j] << " "; } cout << endl;



		gsl_matrix * Xed_COR =  matrix_crosssection( COR, b, b_length );
		sumI_n1 += calcI_det2( Xed_COR );
		gsl_matrix_free( Xed_COR );		// this should solve the big memory leak problem
	}

	gsl_matrix_free( COR );

	return( I_n - (I_n + sumI_n1)/N );
}


void print_matrix( gsl_matrix * M )
{
	int end  = M->size2;	//These may need to be flipped, not sure.
	int end2 = M->size1;	//These may need to be flipped, not sure.

	for(int i=0; i<end2; i++)
	{
		for(int j=0; j<end; j++)
		{
			cout << "[" << i << "," << j << "]: " << gsl_matrix_get(M, i, j) << "\t" ;
		}
		cout << endl;
	}
}


double calcI_det2(gsl_matrix * COR)
{
/* MATLAB CODE:
	function [I] = calcI_det2(COR)
	% ------------------------------------------------------------------------------
	% COV:      covariance matrix of X
	% I:        integration
	% ------------------------------------------------------------------------------
	% Returns the integration for a system X.
	% System dynamics is characterised by its covariance matrix COV.
	% Computes integration from the determinant of the covariance matrix.
	% Olaf Sporns, Indiana University, 2003
	% ------------------------------------------------------------------------------

	%N = size(COR,1);
	%pie1 = 2*pi*exp(1);
	%I = (sum(log(pie1*diag(COV))) - (N*log(pie1) + sum(log(diag(COV))) + log(det(COR))))/2;
	I = - log(det(COR))/2;
*/

//DEBUG	cout << endl << "COR: " << endl;
//DEBUG	print_matrix(COR);

	gsl_matrix * ludecomp = gsl_matrix_alloc( COR->size1, COR->size2 );
	gsl_matrix_memcpy( ludecomp, COR );		//make a copy so we don't overwrite COR when we decomp
	gsl_permutation * P = gsl_permutation_alloc(COR->size1);
	int signum = 1;
	gsl_linalg_LU_decomp( ludecomp, P, &signum );
	double det = gsl_linalg_LU_det( ludecomp, signum );

	gsl_permutation_free(P);
	gsl_matrix_free( ludecomp );
//	gsl_matrix_free( COR );		// this isn't the real COR (which we still need.  This is the cross_section'ed COR, which we no longer need.

	det = fabs(det);

	return( -1 * (double) log(det) / 2 );
}

/*
matrix_crosssection() takes 3 arguments:
	1. The initial matrix.
	2. An array of which indexes you want the crosssections among (must be in ascending order).
		All values in the array must be between [0,N-1] where N is the size of the matrix.
	3. The length of array from #2

	The returned matrix will be square.
*/
gsl_matrix * matrix_crosssection( gsl_matrix * Minput, int* thearray, int thearray_length )
{
	// Define our matrix to return
	gsl_matrix * Mnew = gsl_matrix_alloc( thearray_length, thearray_length ); // defines a matrix with rows/columns [0 ... thearray_length-1]


	for( int row=0; row<thearray_length; row++ )
	{
		gsl_vector_view row_i = gsl_matrix_row( Minput, thearray[row] );	

//		cout << "Length of Minput row_i" << "(" << i << "): " << (&row_i.vector)->size << " // Values in row_i: ";
//		for(int temp=0; temp < (&row_i.vector)->size; temp++ )
//		{
//			cout << gsl_vector_get( &row_i.vector,	temp) << " ";
//		}
//		cout << endl;

		for( int col=0; col<thearray_length; col++ )
		{
			gsl_matrix_set( Mnew, row, col, gsl_vector_get(&row_i.vector, thearray[col]) );
		}
	}

	return Mnew;
}

// eof

