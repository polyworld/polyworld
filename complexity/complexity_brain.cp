#include "complexity_brain.h"

#include <string.h>

#include <iostream>
#include <list>

#include "AbstractFile.h"
#include "complexity_algorithm.h"

using namespace std;

//===========================================================================
// compile-time options
//===========================================================================

// Turn on no more than one of the *Options flags
#define VirgilOptions 0
#define OlafOptions 0

// Note:  The bias neuron is not recorded in the brain function files, as it is always 1.0.
// So I don't think turning on FLAG_subtractBias is ever a good idea, unless the subroutine
// that reads brain function files in this code creates this constant entry.  -  larryy

#if VirgilOptions
	// Virgil Suggests the Following Options:
	#define FLAG_useGSAMP 1
	#define FLAG_subtractBias 1
	#define IgnoreAgentsThatLivedLessThan_N_Timesteps 200
	#define MaxNumTimeStepsToComputeComplexityOver 500		// set this to a positive value to only compute Complexity over the final N timestesps of an agent's life.
#elif OlafOptions
	// Estimated Options from Olaf's MATLAB Code
	#define FLAG_useGSAMP 1
	#define FLAG_subtractBias 0
	#define IgnoreAgentsThatLivedLessThan_N_Timesteps 0
	#define MaxNumTimeStepsToComputeComplexityOver 500		// set this to a positive value to only compute Complexity over the final N timestesps of an agent's life.
#else
	#define FLAG_useGSAMP 1
	#define FLAG_subtractBias 0
	#define IgnoreAgentsThatLivedLessThan_N_Timesteps 0
	#define MaxNumTimeStepsToComputeComplexityOver 500		// set this to a positive value to only compute Complexity over the final N timestesps of an agent's life.
#endif
/*
   If both CalcComplexity's ignore_timesteps_after AND MaxNumTimeStepsToComputeComplexityOver are defined, it will:
	First, reduce the matrix to only the first ignore_timesteps_after timesteps.
	Second, read only the first MaxNumTimeStepsToComputeComplexityOver timesteps
*/

#define DebugFiltering 0

#if DebugFiltering
	#define filprint( x... ) printf( x )
#else
	#define filprint( x... )
#endif

//===========================================================================
// Function Prototypes
//===========================================================================

void FilterActivity( gsl_matrix* activity, const char* filter_events, const long agent_number, const long agent_birth, const long lifespan, Events* events, long numinputneurons );

//===========================================================================
// Function Implementations
//===========================================================================

//---------------------------------------------------------------------------
// CalcComplexity_brainfunction
//---------------------------------------------------------------------------
CalcComplexity_brainfunction_result *
	CalcComplexity_brainfunction(CalcComplexity_brainfunction_parms *parms,
				     int nparms,
				     CalcComplexity_brainfunction_callback *callback)
{
	CalcComplexity_brainfunction_result *result = new CalcComplexity_brainfunction_result( parms, nparms );
	
	if( callback )
		callback->begin( parms, nparms );

#pragma omp parallel for
	for( int iparm = 0; iparm < nparms; iparm++ )
	{
		CalcComplexity_brainfunction_parms *parm = &parms[iparm];

		double complexity = CalcComplexity_brainfunction( parm->path,
								 parm->parts,
								 parm->events,
								 parm->ignore_timesteps_after,
								 result->agent_number + iparm,
								 result->lifespan + iparm,
								 result->num_neurons + iparm );
		result->complexity[iparm] = complexity;

		if( callback )
			callback->parms_result(result, iparm);
	}

	if(callback)
		callback->end(result);

	return result;
}


//---------------------------------------------------------------------------
// CalcComplexity_brainfunction
//---------------------------------------------------------------------------
double CalcComplexity_brainfunction(const char *fnameAct,
				    const char *part,
				    Events *events,
				    int ignore_timesteps_after,
				    long *agent_number,
				    long *lifespan,
				    long *num_neurons)
{
	double complexity;
	long numinputneurons = 0;		// this value will be defined by readin_brainfunction()
	long numoutputneurons = 0;
	long agent_birth = -1;
	long agent_num;
	long agent_lifespan;
	
	gsl_matrix * activity = readin_brainfunction(fnameAct,
												 ignore_timesteps_after,
												 MaxNumTimeStepsToComputeComplexityOver,
												 &agent_num,
												 &agent_birth,
												 &agent_lifespan,
												 num_neurons,
												 &numinputneurons,
												 &numoutputneurons);
	
	if( agent_number )
		*agent_number = agent_num;
	
	if( lifespan )
		*lifespan = agent_lifespan;
	
	// If the brain file was invalid or memory allocation failed, just return 0.0
	if( activity == NULL )
		return( 0.0 );

    // If agent lived fewer timesteps than it has neurons, or it hasn't lived long enough,
    // return Complexity = 0.0.
    if( activity->size2 > activity->size1 || activity->size1 < IgnoreAgentsThatLivedLessThan_N_Timesteps )
    	return( 0.0 );
    
    if( events )
    {
    	int size_filter_events = 8;
    	int num_filter_events = 0;
    	char filter_events[size_filter_events];
    	
    	for( unsigned int i = 0; i < strlen( part ); i++ )
    	{
    		if( islower( part[i] ) )
    		{
    			filter_events[num_filter_events] = part[i];
    			num_filter_events++;
    			if( num_filter_events >= size_filter_events )
    			{
					cerr << "Error: too many filter events specified (" << __func__ << ")" << endl;
					exit( 1 );
    			}
    		}
    	}
    	
    	if( num_filter_events > 0 )
	    	FilterActivity( activity, filter_events, agent_num, agent_birth, agent_lifespan, events, numinputneurons );
    }

	complexity = CalcComplexityWithMatrix_brainfunction(activity,
														part,
														numinputneurons,
														numoutputneurons);
	
// 	printf( "CalcComplexity for agent %ld part %s %s event filtering = %g\n", agent_num, part, events ? "with" : "without", complexity );

	gsl_matrix_free( activity );

	return( complexity );
}

//---------------------------------------------------------------------------
// CalcComplexityWithMatrix_brainfunction
//---------------------------------------------------------------------------
double CalcComplexityWithMatrix_brainfunction(gsl_matrix *activity,
											  const char *part,
											  long numinputneurons,
											  long numoutputneurons)
{
// 	printf( "\n------- part = %s --------\n", part );
	if( numinputneurons == 0 )
		return( -2 );
	
	setGaussianize( FLAG_useGSAMP );
	
	int flagAll = 0;
	int flagPro = 0;
	int flagInp = 0;
	int flagBeh = 0;
	int flagHea = 0;
	
	int startPro = numinputneurons;
	int numPro = activity->size2 - numinputneurons;	// size 2 is number of columns == number of neurons
	int indexPro[numPro];
	
	int startInp = 0;
	int numInp = numinputneurons;
	int indexInp[numInp];
	
	int startBeh = numinputneurons;
	int numBeh = numoutputneurons;
	int indexBeh[numBeh];
	
	int indexHea = 1;
	
	int num_points = 1;
	
	for( unsigned int j = 0; j < strlen( part ); j++ )
	{
		// lowercase characters indicate event filtering; ignore them here
		if( islower( part[j] ) )
			continue;
		
		// trailing digits define num_points used for integrating the area between the curves
		if( isdigit( part[j] ) )
		{
			const char* num_points_str = &(part[j]);
			num_points = atoi( num_points_str );
			break;
		}
		
		char complexityType = part[j];
		
		switch( complexityType )
		{
			case'A':
				flagAll = 1;
				break;
			case'P':
				flagPro = 1;
				for( int i = 0; i < numPro; i++ )
					indexPro[i] = startPro + i;
				break;
			case'I':
				flagInp = 1;
				for( int i = 0; i < numInp; i++ )
					indexInp[i] = startInp + i;
				break;
			case'B':
				flagBeh = 1;
				for( int i = 0; i < numBeh; i++ )
					indexBeh[i] = startBeh+i;
				break;
			case'H':
				flagHea = 1;
				break;
			default:
				fprintf( stderr, "%s: Invalid complexity type (%c)\n",
						__FUNCTION__, complexityType );
				exit( 1 );
		}
	}
		
	if (flagAll == 1)
	{
		double complexity = CalcApproximateFullComplexityWithMatrix( activity, num_points );
		return complexity;
	}
	
	// Accumulate the indexes of neurons related to the requested complexity type
	
	int columns[activity->size2];	// size 2 is number of columns == number of neurons
	int numColumns = 0;

	if( flagInp == 1 )
	{
		int j = 0;
		for( int i = numColumns; i < numInp; i++ )
			columns[i] = indexInp[j++];
		numColumns += numInp;
	}
	
	if( flagHea == 1 && flagInp == 0 )
	{
	    columns[0] = indexHea;
		numColumns = 1;
	}
	
	if( flagPro == 1 )
	{
		
		int j = 0;
		for( int i = numColumns; i < (numPro + numColumns); i++ )
			columns[i] = indexPro[j++];
		numColumns += numPro;
		
	}
			
	if( flagBeh == 1 && flagPro == 0 )
    {
		int j = 0;
		for( int i = numColumns; i < (numBeh + numColumns); i++)
			columns[i] = indexBeh[j++];
		numColumns += numBeh;
	}
	
	gsl_matrix* subset = matrix_subset_col( activity, columns, numColumns );
	
// 	for( size_t j = 0; j < subset->size2; j++ )
// 	{
// 		double mean = 0.0;
// 		for( size_t i = 0; i < subset->size1; i++ )
// 			mean += gsl_matrix_get( subset, i, j );
// 		mean /= subset->size1;
// 
// 		for( size_t i = 0; i < subset->size1/2; i++ )
// 			gsl_matrix_set( subset, i, j, mean );
// 	}
	
	double complexity = CalcApproximateFullComplexityWithMatrix( subset, num_points );
	
	gsl_matrix_free( subset );
	
	return complexity;				
}


//---------------------------------------------------------------------------
// get_list_of_brainfunction_logfiles
//---------------------------------------------------------------------------
vector<string> get_list_of_brainfunction_logfiles( string directory_name )
{

	const char* Function_string = "_brainFunction_";        //  _brainFunction_
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


//---------------------------------------------------------------------------
// get_list_of_brainanatomy_logfiles
//---------------------------------------------------------------------------
vector<string> get_list_of_brainanatomy_logfiles( string directory_name )
{

	const char* Function_string = "_brainAnatomy_";        //  _brainFunction_
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


//---------------------------------------------------------------------------
// readin_brainanatomy
//---------------------------------------------------------------------------
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
		string str_cijline = cijline;					//This sort of thing could probably be optimized, but I'm not sure how.
	
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


//---------------------------------------------------------------------------
// readin_brainfunction
//---------------------------------------------------------------------------
gsl_matrix * readin_brainfunction(const char* fname,
								  int ignore_timesteps_after,
								  int max_timesteps,
								  long *agent_number,
								  long *agent_birth,
								  long *lifespan,
								  long *num_neurons,
								  long *num_ineurons,
								  long *num_oneurons)
{
	long numneur;
	long numineur;
	long numoneur;
	
// 	printf( "\n*************readin_brainfunction****************\n" );
	
// 	printf( "\nmax_timesteps = %d\n", max_timesteps );
	assert( ignore_timesteps_after >= 0 );		// just to be safe.

	AbstractFile *FunctionFile;
	if( (FunctionFile = AbstractFile::open(fname, "r")) == NULL )
	{
		cerr << "Could not open file '" << fname << "' for reading. -- Terminating." << endl;
		exit(1);
	}

	char tline[100];
	FunctionFile->gets( tline, 100 );

	int version;
	if( 0 != strncmp(tline, "version ", 8) )
	{
		version = 0;
	}
	else
	{
		version = atoi(tline + 8);

		// read in line after version
		char *nlpos = strchr(tline, '\n');
		FunctionFile->seek( (nlpos - tline) + 1, SEEK_SET );
		FunctionFile->gets( tline, 100 );
	}

	string params = tline;
	params = params.substr(14, params.length());

	size_t indexSpace = params.find( " ", 0 );
	string agentNum = params.substr( 0, indexSpace );
	params.erase( 0, indexSpace + 1 );						//Remove the agentNum
	if( agent_number )
		*agent_number = atoi( agentNum.c_str() );

	indexSpace = params.find( " ", 0 );
	string numneu = params.substr( 0 , indexSpace );
	params.erase( 0, indexSpace + 1 );						//Remove the numneu
	numneur = atoi( numneu.c_str() );
	if( num_neurons )
		*num_neurons = numneur;

	indexSpace = params.find( " ", 0 );
	string numineu = params.substr( 0, indexSpace );
	params.erase( 0, indexSpace + 1 );						//Remove the numineu
	numineur = atoi( numineu.c_str() );
	if( num_ineurons )
		*num_ineurons = numineur;

	// Read (or set) number of output neurons
	if( version == 0 )
	{
		numoneur = 7;
	}
	else
	{
		indexSpace = params.find( " ", 0 );
		string numoneu = params.substr( 0, indexSpace );
		params.erase( 0, indexSpace + 1 );					//Remove the numoneu
		numoneur = atoi( numoneu.c_str() );
	}
	if( num_oneurons )
		*num_oneurons = numoneur;
	
	// Read number of synapses out of the way
	indexSpace = params.find( " ", 0 );
	string num_synapses = params.substr( 0, indexSpace );
	params.erase( 0, indexSpace + 1 );
	
	// Read birth time
	indexSpace = params.find( " ", 0 );
	string birth_time = params.substr( 0, indexSpace );
	params.erase( 0, indexSpace + 1 );
	if( agent_birth )
		*agent_birth = atol( birth_time.c_str() );
	
	vector<string> FileContents;

	char nextl[200];
	while( FunctionFile->gets(nextl, 200) )	// returns null when at end of file
		FileContents.push_back( nextl );

	delete FunctionFile;					// Don't need this anymore.

	int numcols = numneur;

	if( FileContents.size() % numcols )	// true iff we are dealing with a complete brainfunction file
		FileContents.pop_back();		// get rid of the last line (in complete files, it's fitness).
	
	int numrows = FileContents.size() / numcols;
	if( lifespan )
		*lifespan = numrows;	// actual lifespan, not accounting for max_timestpes

	// bogus? if( numcols == *num_ineurons ) { *num_ineurons = 0; }	// make sure there was a num_ineurons

	if( (float(numrows) - (FileContents.size() / numcols)) != 0.0 )
	{
		cerr << "Warning: #lines (" << FileContents.size() << ") in brainFunction file '" << fname << "' is not an even multiple of #neurons (" << numcols << ").  brainFunction file may be corrupt." << endl;
	}

	if( ignore_timesteps_after > 0 ) 	// if we are only looking at the first N timesteps of an agent's life...
	{
		numrows = min( numrows, ignore_timesteps_after ); //
		ignore_timesteps_after = numrows; //If ignore_timesteps is too big, make it small.
	}
	
	if( max_timesteps > 0 )	// if we are only looking at last max_timesteps of an agent's life
	{
		numrows = min( numrows, max_timesteps );
		max_timesteps = numrows;
	}
// 	printf( "numrows = %d\n", numrows );

	gsl_matrix * activity = NULL;

	// Make sure the matrix isn't invalid.  If it is, return NULL.
	if( numcols <= 0 || numrows <= 0)
	{
		cerr << "brainFunction file '" << fname << "' is corrupt; numcols=" << numcols << ", numrows=" << numrows << endl;
		activity = NULL;
		return activity;
	}

	activity = gsl_matrix_alloc( numrows, numcols );

	int tcnt = 0;

	vector<string>::iterator FileContents_it;
	vector<string>::iterator FileContents_begin;
	vector<string>::iterator FileContents_end;
	
	FileContents_begin = FileContents.begin();
	if( ignore_timesteps_after > 0 )
		FileContents_end = FileContents_begin  +  ignore_timesteps_after * numneur;
	else
		FileContents_end = FileContents.end();
	if( max_timesteps > 0 && max_timesteps < (FileContents_end - FileContents_begin) )
		FileContents_begin = FileContents_end  -  max_timesteps * numneur;

#define DebugReadBrainFunction 0
#if DebugReadBrainFunction
	static int fileCount = 0;
	fileCount++;
	char debugFilename[256];
	sprintf( debugFilename, "matrix_%d.txt", fileCount );
	FILE* debugFile = fopen( debugFilename, "w" );
	fprintf( debugFile, "Matrix size = %d rows x %d columns\n", numrows, numcols );
	fprintf( debugFile, "Original numLines = %lu\n", (unsigned long) (FileContents.end() - FileContents.begin()) );
	fprintf( debugFile, "firstLine = %lu\n", (unsigned long) (FileContents_begin - FileContents.begin() + 1) );
	fprintf( debugFile, "numLines = %lu\n", (unsigned long) (FileContents_end - FileContents_begin) );
#endif

	for( FileContents_it = FileContents_begin; FileContents_it != FileContents_end; FileContents_it++ )
	{
		int thespace = (*FileContents_it).find( " ", 0 );
		int    tstep1 = atoi( ((*FileContents_it).substr( 0, thespace )).c_str() );
		double  tstep2 = atof( ((*FileContents_it).substr( thespace, (*FileContents_it).length() )).c_str() );

		gsl_matrix_set( activity, int(tcnt/numcols), tstep1, tstep2);
#if DebugReadBrainFunction
		fprintf( debugFile, "set row=%d, col=%d = %g\n", int(tcnt/numcols), tstep1, tstep2 );
#endif
		tcnt++;
	}

#if DebugReadBrainFunction
	fclose( debugFile );
#endif
	return activity;
}

//---------------------------------------------------------------------------
// FilterActivity
//---------------------------------------------------------------------------
void FilterActivity( gsl_matrix* activity, const char* filter_events, const long agent_number, const long agent_birth, const long lifespan, Events* events, long numinputneurons )
{
	// #define just one of the following filter types to 1, others to 0
	#define FILTER_SUM 0
	#define FILTER_MAX 1
	#define FILTER_BOX 0
	#define FILTER_BOX_ZERO 0

// m9e4m1.0e0.5
// 	static int mate_filter_radius = 9;	// exclude center point
// 	static int eat_filter_radius = 4;	// exclude center point
// 	static double mate_filter[] = { 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0,
// 									0.9, 0.8, 0.7, 0.6, 0.5, 0.4, 0.3, 0.2, 0.1 };
// 	static double eat_filter[] = { 0.1, 0.2, 0.3, 0.4, 0.5, 0.4, 0.3, 0.2, 0.1 };

// m9.9e9.9e1.0e1.0 (aka m9e9m1.0e1.0)
// 	static int mate_filter_pre = 9;	// exclude center point
// 	static int mate_filter_post = 9;	// exclude center point
// 	static int eat_filter_pre = 9;		// exclude center point
// 	static int eat_filter_post = 9;		// exclude center point
// 	static double mate_filter[] = { 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0,
// 									0.9, 0.8, 0.7, 0.6, 0.5, 0.4, 0.3, 0.2, 0.1 };
// 	static double  eat_filter[] = { 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0,
// 									0.9, 0.8, 0.7, 0.6, 0.5, 0.4, 0.3, 0.2, 0.1 };

// m4e4m0.5e0.5
// 	static int mate_filter_radius = 4;	// exclude center point
// 	static int eat_filter_radius = 4;	// exclude center point
// 	static double mate_filter[] = { 0.1, 0.2, 0.3, 0.4, 0.5, 0.4, 0.3, 0.2, 0.1 };
// 	static double  eat_filter[] = { 0.1, 0.2, 0.3, 0.4, 0.5, 0.4, 0.3, 0.2, 0.1 };

// m4e4m1.0e1.0
// 	static int mate_filter_radius = 4;	// exclude center point
// 	static int eat_filter_radius = 4;	// exclude center point
// 	static double mate_filter[] = { 0.2, 0.4, 0.6, 0.8, 1.0, 0.8, 0.6, 0.4, 0.2 };
// 	static double  eat_filter[] = { 0.2, 0.4, 0.6, 0.8, 1.0, 0.8, 0.6, 0.4, 0.2 };

// m19.19e9.9m1.0e1.0 (aka m19e9m1.0e1.0)
// 	static int mate_filter_pre = 19;	// exclude center point
// 	static int mate_filter_post = 19;	// exclude center point
// 	static int eat_filter_pre = 9;		// exclude center point
// 	static int eat_filter_post = 9;		// exclude center point
// 	static double mate_filter[] = { 0.05, 0.1, 0.15, 0.2, 0.25, 0.3, 0.35, 0.4, 0.45, 0.5,
// 									0.55, 0.6, 0.65, 0.7, 0.75, 0.8, 0.85, 0.9, 0.95, 1.0,
// 									0.95, 0.9, 0.85, 0.8, 0.75, 0.7, 0.65, 0.6, 0.55, 0.5,
// 									0.45, 0.4, 0.35, 0.3, 0.25, 0.2, 0.15, 0.1, 0.05};
// 	static double  eat_filter[] = { 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0,
// 									0.9, 0.8, 0.7, 0.6, 0.5, 0.4, 0.3, 0.2, 0.1 };

// m19.19e19.19m1.0e1.0  (aka m19e19m1.0e1.0)
	static int mate_filter_pre = 19;	// exclude center point
	static int mate_filter_post = 19;	// exclude center point
	static int eat_filter_pre = 19;		// exclude center point
	static int eat_filter_post = 19;		// exclude center point
	static double mate_filter[] = { 0.05, 0.1, 0.15, 0.2, 0.25, 0.3, 0.35, 0.4, 0.45, 0.5,
									0.55, 0.6, 0.65, 0.7, 0.75, 0.8, 0.85, 0.9, 0.95, 1.0,
									0.95, 0.9, 0.85, 0.8, 0.75, 0.7, 0.65, 0.6, 0.55, 0.5,
									0.45, 0.4, 0.35, 0.3, 0.25, 0.2, 0.15, 0.1, 0.05};
	static double  eat_filter[] = { 0.05, 0.1, 0.15, 0.2, 0.25, 0.3, 0.35, 0.4, 0.45, 0.5,
									0.55, 0.6, 0.65, 0.7, 0.75, 0.8, 0.85, 0.9, 0.95, 1.0,
									0.95, 0.9, 0.85, 0.8, 0.75, 0.7, 0.65, 0.6, 0.55, 0.5,
									0.45, 0.4, 0.35, 0.3, 0.25, 0.2, 0.15, 0.1, 0.05};

// m19.4e9.4m1.0e1.0
// 	static int mate_filter_pre = 19;	// exclude center point
// 	static int mate_filter_post = 4;	// exclude center point
// 	static int eat_filter_pre = 9;		// exclude center point
// 	static int eat_filter_post = 4;		// exclude center point
// 	static double mate_filter[] = { 0.05, 0.1, 0.15, 0.2, 0.25, 0.3, 0.35, 0.4, 0.45, 0.5,
// 									0.55, 0.6, 0.65, 0.7, 0.75, 0.8, 0.85, 0.9, 0.95, 1.0,
// 									0.8, 0.6, 0.4, 0.2 };
// 	static double  eat_filter[] = { 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0,
// 									0.8, 0.6, 0.4, 0.2 };

// m19.9e9.9m1.0e1.0
// 	static int mate_filter_pre = 19;	// exclude center point
// 	static int mate_filter_post = 9;	// exclude center point
// 	static int eat_filter_pre = 9;		// exclude center point
// 	static int eat_filter_post = 9;		// exclude center point
// 	static double mate_filter[] = { 0.05, 0.1, 0.15, 0.2, 0.25, 0.3, 0.35, 0.4, 0.45, 0.5,
// 									0.55, 0.6, 0.65, 0.7, 0.75, 0.8, 0.85, 0.9, 0.95, 1.0,
// 									0.9, 0.8, 0.7, 0.6, 0.5, 0.4, 0.3, 0.2, 0.1 };
// 	static double  eat_filter[] = { 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0,
// 									0.9, 0.8, 0.7, 0.6, 0.5, 0.4, 0.3, 0.2, 0.1 };

// m19.9e19.9m1.0e1.0
// 	static int mate_filter_pre = 19;	// exclude center point
// 	static int mate_filter_post = 9;	// exclude center point
// 	static int eat_filter_pre = 19;		// exclude center point
// 	static int eat_filter_post = 9;		// exclude center point
// 	static double mate_filter[] = { 0.05, 0.1, 0.15, 0.2, 0.25, 0.3, 0.35, 0.4, 0.45, 0.5,
// 									0.55, 0.6, 0.65, 0.7, 0.75, 0.8, 0.85, 0.9, 0.95, 1.0,
// 									0.9, 0.8, 0.7, 0.6, 0.5, 0.4, 0.3, 0.2, 0.1 };
// 	static double  eat_filter[] = { 0.05, 0.1, 0.15, 0.2, 0.25, 0.3, 0.35, 0.4, 0.45, 0.5,
// 									0.55, 0.6, 0.65, 0.7, 0.75, 0.8, 0.85, 0.9, 0.95, 1.0,
// 									0.9, 0.8, 0.7, 0.6, 0.5, 0.4, 0.3, 0.2, 0.1 };

// m9.4e9.4m1.0e1.0
// 	static int mate_filter_pre = 9;	// exclude center point
// 	static int mate_filter_post = 4;	// exclude center point
// 	static int eat_filter_pre = 9;		// exclude center point
// 	static int eat_filter_post = 4;		// exclude center point
// 	static double mate_filter[] = { 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0,
// 									0.8, 0.6, 0.4, 0.2 };
// 	static double  eat_filter[] = { 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0,
// 									0.8, 0.6, 0.4, 0.2 };

// m19.9e9.4m1.0e1.0
// 	static int mate_filter_pre = 19;	// exclude center point
// 	static int mate_filter_post = 4;	// exclude center point
// 	static int eat_filter_pre = 9;		// exclude center point
// 	static int eat_filter_post = 4;		// exclude center point
// 	static double mate_filter[] = { 0.05, 0.1, 0.15, 0.2, 0.25, 0.3, 0.35, 0.4, 0.45, 0.5,
// 									0.55, 0.6, 0.65, 0.7, 0.75, 0.8, 0.85, 0.9, 0.95, 1.0,
// 									0.9, 0.8, 0.7, 0.6, 0.5, 0.4, 0.3, 0.2, 0.1 };
// 	static double  eat_filter[] = { 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0,
// 									0.8, 0.6, 0.4, 0.2 };

	long duration = activity->size1;	// may be less than lifespan due to MaxNumTimeStepsToComputeComplexityOver
	long agent_death = agent_birth + lifespan;	// seems like this should be -1, but this agrees with BirthsDeaths.log
	long agent_start = agent_death - duration + 1;
	bool mate_filtering = strchr( filter_events, 'm' );
	bool eat_filtering = strchr( filter_events, 'e' );


// 	filprint( "%s: num=%ld, birth=%ld, lifespan=%ld, death=%ld, start=%ld, duration=%ld, mate=%c, eat=%c\n", __func__,
// 			agent_number, agent_birth, lifespan, agent_death, agent_start, duration, mate_filtering ? 'T':'F', eat_filtering ? 'T':'F' );
	
	double* filter = (double*) calloc( duration, sizeof( *filter ) );	// 0.0 => mean, 1.0 => original signal
	
	for( long step = agent_start; step <= agent_death; step++ )
	{
	#if DebugFiltering
		printf( "  Step %ld\n", step );
		printf( "   Using GetAgentEventsMap and iterator:\n" );
		
		AgentEventsMapType agentEventsMap = events->GetAgentEventsMap( step );
		
		itfor( AgentEventsMapType, agentEventsMap, it )
		{
			printf( "    %ld: ", (*it).first );
			AgentEvent agentEvent = (*it).second;
			if( agentEvent.eat )
				printf( "%c ", 'e' );
			if( agentEvent.mate )
				printf( "%c", 'm' );
			printf( "\n" );
		}
	#endif
	
		AgentEvent agentEvent = events->GetAgentEvent( step, agent_number );
		int activity_step = step - agent_start;
		filprint( "   step=%ld, act_step=%d, .mate=%c, .eat=%c\n", step, activity_step, agentEvent.mate ? 'T':'F', agentEvent.eat ? 'T':'F' );
		if( mate_filtering && agentEvent.mate )
		{
			int left = activity_step - mate_filter_pre;
			int right = activity_step + mate_filter_post;
			int lo = left < 0 ? 0 : left;
			int hi = right >= duration ? duration - 1 : right;
			int offset = lo - left;
			filprint( "m %ld: left=%d, right=%d, lo=%d, hi=%d, offset=%d\n", step, left, right, lo, hi, offset );
			for( int i = lo; i <= hi; i++ )
			#if FILTER_SUM
 				filter[i] = fmin( 1.0, filter[i] + mate_filter[i-lo+offset] );
			#elif FILTER_MAX
				filter[i] = fmax( filter[i], mate_filter[i-lo+offset] );
			#elif FILTER_BOX
				filter[i] = 1.0;
			#elif FILTER_BOX_ZERO
				filter[i] = 1.0;
			#endif
		}
		if( eat_filtering && agentEvent.eat )
		{
			int left = activity_step - eat_filter_pre;
			int right = activity_step + eat_filter_post;
			int lo = left < 0 ? 0 : left;
			int hi = right >= duration ? duration - 1 : right;
			int offset = lo - left;
			filprint( "e %ld: left=%d, right=%d, lo=%d, hi=%d, offset=%d\n", step, left, right, lo, hi, offset );
			for( int i = lo; i <= hi; i++ )
			#if FILTER_SUM
 				filter[i] = fmin( 1.0, filter[i] + eat_filter[i-lo+offset] );
			#elif FILTER_MAX
				filter[i] = fmax( filter[i], eat_filter[i-lo+offset] );
			#elif FILTER_BOX
				filter[i] = 1.0;
			#elif FILTER_BOX_ZERO
				filter[i] = 1.0;
			#endif
		}
	}
	
	for( size_t j = 0; j < activity->size2; j++ )
	{
		for( size_t i = 0; i < activity->size1; i++ )
		{
		#if DebugFiltering
			if( j == numinputneurons )
				printf( "%lu,%lu: %g , %g => ", i, j, gsl_matrix_get( activity, i, j ), filter[i] );
		#endif
		#if FILTER_BOX_ZERO
			if( filter[i] == 0.0 )
				gsl_matrix_set( activity, i, j, 0.0 );
		#else
			gsl_matrix_set( activity, i, j, gsl_matrix_get( activity, i, j ) * filter[i]  +  0.5 * (1.0 - filter[i]) );
		#endif
		#if DebugFiltering
			if( j == numinputneurons )
				printf( "%g\n", gsl_matrix_get( activity, i, j ) );
		#endif
		}
	}
	
	free( filter );
}

// eof
