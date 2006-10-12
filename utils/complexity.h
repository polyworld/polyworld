#include <gsl/gsl_blas.h>
#include <gsl/gsl_matrix.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_linalg.h>
#include <math.h>
#include <list.h>
#include <vector.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <gsl/gsl_statistics_double.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>
#include <assert.h>

#define IgnoreCrittersThatLivedLessThan_N_Timesteps 150
#define MaxNumTimeStepsToComputeComplexityOver 0		// set this to a positive value to only compute Complexity over the final N timestesps of an agent's life.

using namespace std;

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
vector<string> get_list_of_brainfunction_logfiles( string );
vector<string> get_list_of_brainanatomy_logfiles( string );
gsl_matrix * readin_brainfunction( const char* , int& );
gsl_matrix * readin_brainfunction__optimized( const char* , int& );
gsl_matrix * readin_brainanatomy( const char* );
gsl_matrix * mCOV( gsl_matrix * );

gsl_matrix * gsamp( gsl_matrix_view x );
int qsort_compare_double( const void *a, const void *b);
int qsort_compare_rows0( const void *a, const void *b );
int qsort_compare_rows1( const void *a, const void *b );
double CalcComplexity( char * , char );

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
	// first must setup the random number generator
	const gsl_rng_type * T;
	gsl_rng * rr;
	gsl_rng_env_setup();
	T = gsl_rng_mt19937;
	rr = gsl_rng_alloc(T);
	time_t seed;
	time(&seed);
	gsl_rng_set(rr, seed);

	// this is nessecary for switching the columns around among the x, z, and y matrices
	gsl_vector * tempcol = gsl_vector_alloc(n);

	for( int i=0; i<cc; i++ )
	{
//MATLAB	z=zeros(n,3);
		gsl_matrix * z = gsl_matrix_calloc( n, 3 );

//MATLAB	gs = sortrows( randn(n,1) , 1 );
		double gs[n]; for( int j=0; j<n; j++ ) { gs[j] = gsl_ran_ugaussian(rr); }
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
        gsl_rng_free( rr );


	if( r < c )
	{
		cout << "end of gsamp(): r < c -- this should be impossible." << endl;
		exit(1);
	}

	return y;
}

double CalcComplexity( char * fnameAct, char part )
{

	part = toupper( part );			// capitalize it
	int numinputneurons = 0;		// this value will be defined by readin_brainfunction()

	gsl_matrix * activity = readin_brainfunction__optimized( fnameAct, numinputneurons );


    // If critter lived less timesteps than it has neurons, return Complexity = 0.0.
    if( activity->size2 > activity->size1 || activity->size1 < IgnoreCrittersThatLivedLessThan_N_Timesteps ) { return 0; }


     const gsl_rng_type * T;
     gsl_rng * r;
     gsl_rng_env_setup();
     T = gsl_rng_mt19937;
     r = gsl_rng_alloc(T);
     time_t seed;
     time(&seed);
     gsl_rng_set(r, seed);


	for( unsigned int i=0; i<activity->size1; i++)
	{
		for( unsigned int j=0; j<activity->size2; j++)
			gsl_matrix_set(activity, i, j, gsl_matrix_get(activity, i,j) + 0.00001*gsl_ran_ugaussian(r));	// we can do smaller values
	}

	gsl_rng_free( r );


/*	MATLAB CODE:
	o = gsamp( activity( size(activity,1) - min(500,size(activity,1)) + 1:size(activity,1) ,:)');
*/

	int numrows = int(activity->size1);
	int numcols = int(activity->size2);
	int beginning_timestep = 0;

//	cout << "size(Activity) = " << activity->size1 << " x " << activity->size2 << endl;
	// this next line puts a cap on the max number of timesteps to compute complexity over.  

	// If we have a cap on the maximum number of timesteps to compute complexity over, run this:
	if( MaxNumTimeStepsToComputeComplexityOver > 0 )
	{
		if( numrows > MaxNumTimeStepsToComputeComplexityOver )
			beginning_timestep = numrows - MaxNumTimeStepsToComputeComplexityOver;
	}

//	cout << "We going to allocate a matrix_view with size: " << activity->size1 - beginning_timestep << " x " << numcols << endl;
	gsl_matrix_view subset_of_activity = gsl_matrix_submatrix( activity, beginning_timestep, 0, numrows - beginning_timestep , numcols );
//debug	print_matrix_row( &subset_of_activity.matrix, (&subset_of_activity.matrix)->size1 - 1);

//	cout << "size(subset_of_activity) [before gsamp()] = " << (&subset_of_activity.matrix)->size1 << " x " << (&subset_of_activity.matrix)->size2 << endl;

//	gsl_matrix * o = activity;

/*
	gsl_matrix * o = gsl_matrix_alloc( (&subset_of_activity.matrix)->size1, (&subset_of_activity.matrix)->size2 );
	gsl_matrix_memcpy( o, (&subset_of_activity.matrix) );
	gsl_matrix_free( activity );
*/
//	print_matrix_row( (&subset_of_activity.matrix), 4 );


	gsl_matrix * o = gsamp( subset_of_activity );
	gsl_matrix_free( activity );


//	cout << "size(o) = " << o->size1 << " x " << o->size2 << endl;
//debug	cout << "Array Size = " << array_size << endl;
//debug	cout << "Beginning Timestep = " << beginning_timestep << endl;




	// We just calculate the covariance matrix and compute the Complexity of that instead of using the correlation matrix.  It uses less cycles and the results are identical.
	gsl_matrix * COVwithbias = mCOV( o );
	gsl_matrix_free( o );			// don't need this anymore


	// Now time to through away the bias neuron from the COVwithbias matrix.
	int numNeurons = COVwithbias->size1 - 1;
	int Neurons[numNeurons];
	for( int i=0;i<numNeurons;i++ ) { Neurons[i] = i; }
	gsl_matrix * COV = matrix_crosssection( COVwithbias, Neurons, numNeurons );	// no more bias now!
	gsl_matrix_free( COVwithbias );


	if( part == 'A' )	// All
	{
		double Complexity = calcC_det3__optimized( COV );
		gsl_matrix_free( COV );
		return Complexity;
	}
	else if( part == 'P' )	// Processing
	{
//		cout << "size of COV Matrix: " << COV->size1 << " x " << COV->size2 << endl;
		int Pro = COV->size1 - numinputneurons;
		int Pro_id[Pro];
		for(int i=0; i<Pro; i++ ) { Pro_id[i] = numinputneurons+i; }

//		cout << "Pro = " << Pro << ":\t";
//		for(int i=0; i<Pro; i++) { cout << Pro_id[i] << " "; }
//		cout << endl;

		gsl_matrix * Xed_COR = matrix_crosssection( COV, Pro_id, Pro );
	     	gsl_matrix_free( COV );

//		cout << "Processing: size of Xed_COR: " << Xed_COR->size1 << " x " << Xed_COR->size2 << endl;

		float Cplx_Pro = calcC_det3__optimized( Xed_COR );
		gsl_matrix_free( Xed_COR );

		return Cplx_Pro;
	}
	else if( part == 'I' )	// Input
	{
		int Inp = numinputneurons;
		int Inp_id[Inp];
		for(int i=0; i< Inp; i++ ) { Inp_id[i] = i; }

//		cout << "Inp = " << Inp << ":\t";
//		for(int i=0; i<Inp; i++) { cout << Inp_id[i] << " "; }
//		cout << endl;

		gsl_matrix * Xed_COR = matrix_crosssection( COV, Inp_id, Inp );
		gsl_matrix_free( COV );

//		cout << "Input: size of Xed_COR: " << Xed_COR->size1 << " x " << Xed_COR->size2 << endl;
		float Cplx_Inp = calcC_det3__optimized( Xed_COR );

		gsl_matrix_free( Xed_COR );

		return Cplx_Inp;
	}
//========
	else
	{
		cerr << "Do not know value for CalcComplexity() argument 2 '" << part << "'\n";
		exit(1);
	}

}

gsl_matrix * COVtoCOR( gsl_matrix * COV )
{
/* WARNING: COV may not be square!!! */

/* MATLAB CODE: COR = COV ./ ( sqrt(diag(COV)) * sqrt(diag(COV))'); */

	int N = COV->size1;

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


vector<string> get_list_of_brainanatomy_logfiles( string directory_name )
{

	char* Function_string = "_brainAnatomy_";        //  _brainFunction_
	vector<string> z;
	struct dirent *entry;

//DEBUG	cout << "Directory: " << directory_name << endl;

	DIR * DIRECTORY;
	if( (DIRECTORY = opendir(directory_name.c_str())) == NULL )
	{
		cerr << "Could not opendir('" << directory_name << "') -- Terminating." << endl;
		exit(1);
	}


	while((entry = readdir(DIRECTORY)) != NULL)
	{
//		cout << "File: " << buffer << endl;

	// Get all brainfunction filenames, but exclude those that are matlab files.
		if( (strstr(entry->d_name, Function_string) != NULL) && (strstr(entry->d_name, ".txt.mat") == NULL) )
		{
//			cout << "***File: " << entry->d_name << "***" << endl;
			z.push_back( directory_name + entry->d_name );
		}
	}

//	cout << "My List: " << endl;

//	for(int i=0; i<z.size(); i++)
//	{
//		cout << z[i] << endl;
//	}

	if( (closedir(DIRECTORY)) == -1)
	{
	cerr << "Could not closedir('" << directory_name << "').  -- Terminating." << endl;
	exit(1);
	}

	return z;
}



gsl_matrix * readin_brainanatomy( const char* fname )
{
/* MATLAB CODE:
	% strip fnames of spaces at the end
	fname = fname(fname~=' ');

	% open file
	fid = fopen(fname,'r');
	if (fid==-1) fname = fname(1:end-1); fid = fopen(fname,'r'); end;
*/

	FILE * AnatomyFile;
	if( (AnatomyFile = fopen(fname, "rt")) == NULL )
	{
		cerr << "Could not open file '" << fname << "' for reading. -- Terminating." << endl;
		exit(1);
	}

//DEBUG	fseek( AnatomyFile, 0, SEEK_END );
//DEBUG	int filesize = ftell( AnatomyFile );
//DEBUG	cout << "filesize = " << filesize << endl;
//DEBUG	rewind( AnatomyFile );


/* MATLAB CODE:
	% read first line
	tline = fgetl(fid);

	% readin filenumber
	filnum = tline(7:15);
	while isempty(str2num(filnum))
	    filnum = filnum(1:length(filnum)-1);
	end;
	filnum = str2num(filnum);
*/
	char tline[200];
	fgets( tline, 200, AnatomyFile );
//DEBUG cout << "First Line: " << tline << " // Length: " << strlen(tline) << endl;

    string str_tline = tline;
	str_tline.erase(0,6);		//Remove the beginning "brain "
//	int filnum = atoi( (str_tline.substr(0, str_tline.find(" ",0)).c_str()) );
//	cout << "filnum = " << filnum << endl;

	str_tline.erase( 0, str_tline.find(" ",0)+1);	// We're done with the filnum, lets move on.

/* MATLAB CODE:
	% readin numneurons
	numneu = tline(feq(2)+1:feq(2)+9);
	while isempty(str2num(numneu))
	    numneu = numneu(1:length(numneu)-1);
	end;
	numneu = str2num(numneu);
*/
//DEBUG	cout << "str_tline = " << str_tline;

	string str_fitness = str_tline.substr(str_tline.find("=",0)+1, str_tline.find(" ",0) );

	//fitness maybe useful later.  But for now it's not.
//	float fitness = atof( str_fitness.substr(0, str_fitness.find(" ",0)).c_str() );
//DEBUG	cout << "fitness = " << fitness << endl;
	str_tline.erase( 0, str_tline.find(" ",0)+1);	// We're done with the fitness, lets move on.


//DEBUG	cout << "\n\n" << "str_tline = " << str_tline << endl;
	str_tline.erase( 0, str_tline.find("=",0)+1);	// get rid of the numneurons=

	int numneu = atoi( (str_tline.substr(0, str_tline.find(" ",0))).c_str() ) ;
//DEBUG	cout << "numneu = " << numneu << endl;
	str_tline.erase( 0, str_tline.find(" ",0)+1);	// We're done with the numneu, lets move on.
//DEBUG	cout << "\n\n" << "str_tline = " << str_tline << endl;

/* MATLAB CODE:
	% readin maxWeight
	maxwei = tline(feq(3)+1:length(tline));
	while isempty(str2num(maxwei))
	    maxwei = maxwei(1:length(maxwei)-1);
	end;
	maxwei = str2num(maxwei);
*/

	str_tline.erase( 0, str_tline.find("=",0)+1);	// get rid of the maxWeight=

//  maxwei may be useful later, but for now it's not.
//	int maxwei = atoi( (str_tline.substr(0, str_tline.find(" ",0))).c_str() ) ;
//DEBUG	cout << "maxwei = " << maxwei << endl;
//	str_tline.erase( 0, str_tline.find(" ",0)+1);	// We're done with the maxwei, lets move on.  (not needed)


/* MATLAB CODE:
	% start reading in the cij's
	% get next line
	cijline = fgetl(fid);
	lcij = length(cijline); cijline = cijline(1:lcij-1); cijs = length(cijline)/8;
	for j=1:cijs
	    for i=1:cijs
	        cij(i,j) = str2num(cijline((i-1)*8+1:i*8));
	        

		cij(i,j) = str2num(cijline(  (i-1)*8+1:i*8   ));


	    end;
	    cijline = fgetl(fid);
	end;
	cij = cij';
*/

	gsl_matrix * cij = gsl_matrix_alloc( numneu, numneu );		//Allocate our connection matrix

	char cijline[5000];					//The length of a line could be quite long, so lets be generous.

	for(int i=0; i<numneu; i++)
	{
		fgets( cijline, 5000, AnatomyFile );      		// get the next line
		string str_cijline = cijline;				// !!! This sort of thing could probably be optimized, but I'm not sure how.
	
//DEBUG		cout << "str_cijline[" << i << "]: " << str_cijline;	
		for(int j=0; j<numneu; j++)
		{
			gsl_matrix_set( cij, i, j, atof( str_cijline.substr(j*8,(j+1)*8).c_str() ) );
		}

	}

	// We don't need to tranpose the matrix because we reversed the i's and j's from the MATLAB code.


/* MATLAB CODE:
	eval(['save M',fname,'.mat']);
	fclose(fid);
	Mname = ['M',fname,'.mat'];
*/

	fclose( AnatomyFile );

	return cij;

}


gsl_matrix * readin_brainfunction__optimized( const char* fname, int& numinputneurons )
{
/* This function largely replicates the function of the MATLAB file readin_brainfunction.m (Calculates, but does not return filnum, fitness) */

/* MATLAB CODE:
        % strip fnames of spaces at the end
        fname = fname(fname~=' ');                      // We don't need to do this.

        % open file
        fid = fopen(fname,'r');
        if (fid==-1) fname = fname(1:end-1); fid = fopen(fname,'r'); end;
*/

	FILE * FunctionFile;
	if( (FunctionFile = fopen(fname, "rt")) == NULL )
	{
		cerr << "Could not open file '" << fname << "' for reading. -- Terminating." << endl;
		exit(1);
	}


/* MATLAB CODE: 
        % read first line
        tline = fgetl(fid);
        % readin filenumber and cijsize
        params = tline(15:length(tline));
        [params] = str3num(params);
        filnum = params(1);
        numneu = params(2);
*/
	char tline[100];
	fgets( tline, 100, FunctionFile );
//	cout << "First Line: " << tline << " // Length: " << strlen(tline) << endl;

	string params = tline;
	params = params.substr(14, params.length());

//	string filnum = params.substr( 0, params.find(" ",0));				//filnum actually isn't used anywhere
	params.erase( 0, params.find(" ",0)+1 );						//Remove the filnum

	string numneu = params.substr( 0 , params.find(" ",0) );
	params.erase( 0, params.find(" ",0)+1 );						//Remove the numneu

	numinputneurons = atoi( (params.substr(0,params.find(" ",0))).c_str() );


/* MATLAB CODE:
        % start reading in the timesteps
        nextl = fgetl(fid);
        tstep = str2num(nextl);  tcnt = 1;
        while ~isempty(tstep)
                activity(tstep(1)+1,ceil(tcnt/numneu)) = tstep(2);
                tcnt = tcnt+1;
        nextl = fgetl(fid);
        tstep = str2num(nextl);
        end;
*/

	list<string> FileContents;

	char nextl[200];
	while( fgets(nextl, 200, FunctionFile) )	// returns null when at end of file
	{
		FileContents.push_back( nextl );
	}

	fclose( FunctionFile );                         // Don't need this anymore.

	FileContents.pop_back();                        // get rid of the last line (it's useless).


	int numcols = atoi(numneu.c_str());
	int numrows = int(round( FileContents.size() / numcols ));

	if( float(numrows) - (FileContents.size() / numcols) != 0.0 )
	{
		cerr << "Warning: #lines (" << FileContents.size() << ") in brainFunction file '" << fname << "' is not an even multiple of #neurons (" << numcols << ").  brainFunction file may be corrupt." << endl;
	}

	// Make sure the matrix isn't invalid.
	if( numcols <= 0 )
	{
		cerr << "brainFunction file '" << fname << "' is corrupt.  Number of columns is zero." << endl;
		exit(1);
	}
	if( numrows <= 0 )
	{
		cerr << "brainFunction file '" << fname << "' is corrupt.  Number of rows is zero." << endl;
		exit(1);
	}

	assert( numcols > 0 && numrows > 0 );		// double checking that we're not going to allocate an impossible matrix.
	gsl_matrix * activity = gsl_matrix_alloc(numrows, numcols);


	int tcnt=0;

	list<string>::iterator FileContents_it;
	for( FileContents_it = FileContents.begin(); FileContents_it != FileContents.end(); FileContents_it++)
	{
		int thespace = (*FileContents_it).find(" ",0);

		int    tstep1 = atoi( ((*FileContents_it).substr( 0, thespace)).c_str() );
		double  tstep2 = atof( ((*FileContents_it).substr( thespace, (*FileContents_it).length())).c_str() );

		gsl_matrix_set( activity, tcnt/numcols, tstep1, tstep2);
		tcnt++;
	}


/* MATLAB CODE:
        fitness = str2num(nextl(15:end));
        eval(['save M',fname,'.mat']);
        fclose(fid);
        Mname = ['M',fname,'.mat'];
*/


	return activity;
}

vector<string> get_list_of_brainfunction_logfiles( string directory_name )
{

	char* Function_string = "_brainFunction_";        //  _brainFunction_
	vector<string> z;
	struct dirent *entry;

	DIR * DIRECTORY;
	if( (DIRECTORY = opendir(directory_name.c_str())) == NULL )
	{
		cerr << "Could not opendir('" << directory_name << "') -- Terminating." << endl;
		exit(1);
	}


	while((entry = readdir(DIRECTORY)) != NULL)
	{

	// Get all brainfunction filenames, but exclude those that are matlab files.
		if( (strstr(entry->d_name, Function_string) != NULL) && (strstr(entry->d_name, ".txt.mat") == NULL) )
		{
//			cout << "***File: " << entry->d_name << "***" << endl;
			z.push_back( directory_name + entry->d_name );
		}
	}

	if( (closedir(DIRECTORY)) == -1)
	{
		cerr << "Could not closedir('" << directory_name << "').  -- Terminating." << endl;
		exit(1);
	}

	return z;
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
		gsl_matrix_free( Xed_COR );		//!!! this should solve the big memory leak problem
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

// !!! This could be made faster simply by taking the Determinant of COR as an input instead of COR itself. !!!
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


gsl_matrix * readin_brainfunction( const char* fname, int& numinputneurons )
{
/* This function largely replicates the function of the MATLAB file readin_brainfunction.m (Calculates, but does not return filnum, fitness) */

/* MATLAB CODE:
        % strip fnames of spaces at the end
        fname = fname(fname~=' ');                      // We don't need to do this.

        % open file
        fid = fopen(fname,'r');
        if (fid==-1) fname = fname(1:end-1); fid = fopen(fname,'r'); end;
*/

	FILE * FunctionFile;
	if( (FunctionFile = fopen(fname, "rt")) == NULL )
	{
		cerr << "Could not open file '" << fname << "' for reading. -- Terminating." << endl;
		exit(1);
	}

	fseek( FunctionFile, 0, SEEK_END );
	int filesize = ftell( FunctionFile );
//	cout << "filesize = " << filesize << endl;
	rewind( FunctionFile );

/* MATLAB CODE: 
        % read first line
        tline = fgetl(fid);
        % readin filenumber and cijsize
        params = tline(15:length(tline));
        [params] = str3num(params);
        filnum = params(1);
        numneu = params(2);
*/
	char tline[100];
	fgets( tline, 100, FunctionFile );

	string params = tline;
	params = params.substr(14, params.length());

	string filnum = params.substr( 0, params.find(" ",0));				//filnum actually isn't used anywhere
	params.erase( 0, params.find(" ",0)+1 );						//Remove the filnum

	string numneu = params.substr( 0 , params.find(" ",0) );
	params.erase( 0, params.find(" ",0)+1 );						//Remove the numneu

	numinputneurons = atoi( (params.substr(0,params.find(" ",0))).c_str() );

//	cout << "filnum = " << filnum << endl;
//	cout << "numneu = " << numneu << endl;
//	cout << "numinputneurons = " << numinputneurons << endl;

/* MATLAB CODE:
        % start reading in the timesteps
        nextl = fgetl(fid);
        tstep = str2num(nextl);  tcnt = 1;
        while ~isempty(tstep)
                activity(tstep(1)+1,ceil(tcnt/numneu)) = tstep(2);
                tcnt = tcnt+1;
        nextl = fgetl(fid);
        tstep = str2num(nextl);
        end;
*/

	list<string> FileContents;

	char nextl[200];
	while( ftell(FunctionFile) < filesize )
	{
		fgets( nextl, 200, FunctionFile );      // get the next line
		FileContents.push_back( nextl );
	}

	fclose( FunctionFile );                         // Don't need this anymore.

	// We'll use this in the next MATLAB CODE Section
	string LastLine = FileContents.back();
	// End we'll use this later in the next MATLAB CODE Section

	FileContents.pop_back();                        // get rid of the last line.

	int numrows = atoi(numneu.c_str());
	int numcols = int(round( FileContents.size() / numrows ));      // the minus 1 ignores the last line of the file

//DEBUG	cout << "num rows = " << numrows;
//DEBUG	cout << "num cols = " << numcols;


	gsl_matrix * activity = gsl_matrix_alloc(numrows, numcols);

	int tcnt=0;

	list<string>::iterator FileContents_it;
	for( FileContents_it = FileContents.begin(); FileContents_it != FileContents.end(); FileContents_it++)
	{
		int    tstep1 = atoi( ((*FileContents_it).substr( 0, (*FileContents_it).find(" ",0))).c_str() );
		double tstep2 = atof( ((*FileContents_it).substr( (*FileContents_it).find(" ",0), (*FileContents_it).length())).c_str() );

		//Recall that numrows = int(numneu), and that gsl_matrices start with zero, not 1.
		gsl_matrix_set( activity, tstep1, tcnt/numrows, tstep2);
		tcnt++;
	}


/* MATLAB CODE:
        fitness = str2num(nextl(15:end));
        eval(['save M',fname,'.mat']);
        fclose(fid);
        Mname = ['M',fname,'.mat'];
*/

//	fitness may be useful later, but right now it's not.
//	float fitness = atof( LastLine.substr( 14, LastLine.length()).c_str() );		// Nothing uses this.

	return activity;
}
