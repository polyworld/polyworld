#include "complexity_brain.h"

#include <string.h>

#include <iostream>

#include "complexity_algorithm.h"

using namespace std;

//===========================================================================
// compile-time options
//===========================================================================

// Turn on no more than one of the *Options flags
#define VirgilOptions 0
#define OlafOptions 1

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
	#define FLAG_subtractBias 1
	#define IgnoreAgentsThatLivedLessThan_N_Timesteps 0
	#define MaxNumTimeStepsToComputeComplexityOver 500		// set this to a positive value to only compute Complexity over the final N timestesps of an agent's life.
#endif
/*
   If both CalcComplexity's ignore_timesteps_after AND MaxNumTimeStepsToComputeComplexityOver are defined, it will:
	First, reduce the matrix to only the first ignore_timesteps_after timesteps.
	Second, read only the first MaxNumTimeStepsToComputeComplexityOver timesteps
*/

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
	CalcComplexity_brainfunction_result *result = new CalcComplexity_brainfunction_result(parms,
											      nparms);
	if(callback)
	{
		callback->begin(parms, nparms);
	}

#pragma omp parallel for
	for(int iparm = 0; iparm < nparms; iparm++)
	{
		CalcComplexity_brainfunction_parms *parm = &parms[iparm];

		double Complexity = CalcComplexity_brainfunction(parm->path,
								 parm->parts,
								 parm->ignore_timesteps_after,
								 result->agent_number + iparm,
								 result->lifespan + iparm,
								 result->num_neurons + iparm);
		result->complexity[iparm] = Complexity;

		if(callback)
		{
		  callback->parms_result(result,
					 iparm);
		}
	}

	if(callback)
	{
		callback->end(result);
	}

	return result;
}

//---------------------------------------------------------------------------
// CalcComplexity_brainfunction
//---------------------------------------------------------------------------
double CalcComplexity_brainfunction(const char *fnameAct,
				    const char *part,
				    int ignore_timesteps_after,
				    long *agent_number,
				    long *lifespan,
				    long *num_neurons)
{
		
	long numinputneurons = 0;		// this value will be defined by readin_brainfunction()
	long numoutputneurons = 0;
	
	gsl_matrix * activity = readin_brainfunction__optimized(fnameAct,
															ignore_timesteps_after,
															agent_number,
															lifespan,
															num_neurons,
															&numinputneurons,
															&numoutputneurons);

	return CalcComplexityWithMatrix_brainfunction(activity,
												  part,
												  numinputneurons,
												  numoutputneurons);
}

//---------------------------------------------------------------------------
// CalcComplexityWithMatrix_brainfunction
//---------------------------------------------------------------------------
double CalcComplexityWithMatrix_brainfunction(gsl_matrix *activity,
											  const char *part,
											  long numinputneurons,
											  long numoutputneurons)
{
    // if had an invalid brain file, return 0.
    if( activity == NULL ) { return 0.0; }

    // If agent lived less timesteps than it has neurons, return Complexity = 0.0.
    if( activity->size2 > activity->size1 || activity->size1 < IgnoreAgentsThatLivedLessThan_N_Timesteps ) { return 0.0; }
	
    gsl_matrix * o;			// we don't need this guy yet but we will in a bit.  We need to define him here so the useGSAMP can assign to it.

/* Now to inject a little bit of noise into the activity matrix */

    gsl_rng *randNumGen = create_rng(DEFAULT_SEED);
	
    for( unsigned int i=0; i<activity->size1; i++)
	for( unsigned int j=0; j<activity->size2; j++)
		gsl_matrix_set(activity, i, j, gsl_matrix_get(activity, i,j) + 0.00001*gsl_ran_ugaussian(randNumGen));	// we can do smaller values

    dispose_rng(randNumGen);


    if( FLAG_useGSAMP )		// If we're GSAMP'ing, do that now.
    {
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


	o = gsamp( subset_of_activity );
	gsl_matrix_free( activity );
//	cout << "size(o) = " << o->size1 << " x " << o->size2 << endl;
//debug	cout << "Array Size = " << array_size << endl;
//	cout << "Beginning Timestep = " << beginning_timestep << endl;


      }


	// We just calculate the covariance matrix and compute the Complexity of that instead of using the correlation matrix.  It uses less cycles and the results are identical.
	gsl_matrix * COVwithbias = mCOV( o );
	gsl_matrix_free( o );			// don't need this anymore

	gsl_matrix * COV;
	if( FLAG_subtractBias )
	{
		// Now time to through away the bias neuron from the COVwithbias matrix.
		int numNeurons = COVwithbias->size1 - 1;
		int Neurons[numNeurons];
		for( int i=0;i<numNeurons;i++ ) { Neurons[i] = i; }
		COV = matrix_crosssection( COVwithbias, Neurons, numNeurons );	// no more bias now!
		gsl_matrix_free( COVwithbias );
	}
	else {
		COV = COVwithbias;
	}

	int flag_All = 0;
	int flag_Pro = 0;
	int flag_Inp = 0;
	int flag_Beh = 0;
	int flag_Hea = 0;
	
	int PstartIndex = numinputneurons;
	int PnumIndex = COV->size1 - numinputneurons;
	int Pro_id[PnumIndex];
	
	int IstartIndex = 0;
	int InumIndex = numinputneurons;
	int Inp_id[InumIndex];
	
	int BstartIndex = numinputneurons;
	int BnumIndex = numoutputneurons;
	int Beh_id[BnumIndex];
	
	int Hea_id = 1;
	
	// store the index of neurons related to the string complexity type
	int *Com_id = new int[COV->size1];
	
	if( numinputneurons == 0 ) {return -2; }
	
	for( unsigned int j = 0; j < strlen( part ); j++ )
	{
		char complexityType = part[j];
		
		
		switch(toupper(complexityType))
		{
			case'A':
				flag_All = 1;
				break;
			case'P':
				flag_Pro = 1;
		
				for(int i=0; i<PnumIndex; i++ ) { Pro_id[i] = PstartIndex+i; }
				break;
			case'I':
				flag_Inp = 1;
			
				for(int i=0; i<InumIndex; i++ ) { Inp_id[i] = IstartIndex+i; }
				break;
			case'B':
				flag_Beh = 1;
				for(int i=0; i<BnumIndex; i++ ) { Beh_id[i] = BstartIndex+i; }
				break;
			case'H':
				flag_Hea = 1;
				break;
			default:
				fprintf( stderr, "%s: Invalid complexity type (%c)\n",
						__FUNCTION__, complexityType );
				exit( 1 );
		}
	}
		
	if (flag_All == 1)
	{
		double Complexity = calcC_det3__optimized( COV );
		gsl_matrix_free( COV );
		return Complexity;
	}
	
	int curIndex = 0;
	if (flag_Inp == 1)
	{
		int j = 0;
		for(int i=curIndex;i<(InumIndex+curIndex);i++) {Com_id[i] = Inp_id[j++];}
		curIndex = InumIndex + curIndex;
	}
	
	
	if (flag_Hea == 1 && flag_Inp == 0)
	{
	    Com_id[0] = Hea_id;
		curIndex = 1;
	}
	
	if (flag_Pro == 1)
	{
		
		int j = 0;
		for(int i=curIndex;i<(PnumIndex+curIndex);i++){Com_id[i] = Pro_id[j++];}
		curIndex = PnumIndex+curIndex;
		
	}
			
	if (flag_Beh == 1 && flag_Pro == 0)
    {
		int j = 0;
		for(int i=curIndex;i<(BnumIndex+curIndex);i++) {Com_id[i] = Beh_id[j++];}
		curIndex = BnumIndex + curIndex;
	}
	
	int numCom = curIndex;

	gsl_matrix * Xed_COR = matrix_crosssection( COV, Com_id, numCom );
	gsl_matrix_free( COV );
	
	float Cplx_Com = calcC_det3__optimized( Xed_COR );
	gsl_matrix_free( Xed_COR );
	
	delete[] Com_id;
	Com_id = NULL;
	return Cplx_Com;				
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


//---------------------------------------------------------------------------
// readin_brainfunction__optimized
//---------------------------------------------------------------------------
gsl_matrix * readin_brainfunction__optimized(const char* fname,
											 int ignore_timesteps_after,
											 long *agent_number,
											 long *lifespan,
											 long *num_neurons,
											 long *num_ineurons,
											 long *num_oneurons)
{
/* This function largely replicates the function of the MATLAB file
   readin_brainfunction.m (Calculates, but does not return filnum,
   fitness).  This function has also been extended to take a parameter
   that calculates complexity only over the first 'ignore_timesteps_after'
   of an agent's lifetime.
*/
	assert( ignore_timesteps_after >= 0 );		// just to be safe.

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

	int version;
	if( 0 != strncmp(tline, "version ", 8) )
	{
		version = 0;
	}
	else
	{
		version = atoi(tline + 8);

		// read in line after version
		char *nl = strchr(tline, '\n');
		fseek( FunctionFile, (nl - tline) + 1, SEEK_SET );
		fgets( tline, 100, FunctionFile );
	}

	string params = tline;
	params = params.substr(14, params.length());

	string agentNum = params.substr( 0, params.find(" ",0));
	params.erase( 0, params.find(" ",0)+1 );						//Remove the agentNum
	if(agent_number) *agent_number = atoi(agentNum.c_str());

	string numneu = params.substr( 0 , params.find(" ",0) );
	params.erase( 0, params.find(" ",0)+1 );						//Remove the numneu
	if(num_neurons) *num_neurons = atoi(numneu.c_str());

	string numineu = params.substr(0,params.find(" ",0));
	params.erase( 0, params.find(" ",0)+1 );						//Remove the numineu
	if(num_ineurons) *num_ineurons = atoi(numineu.c_str());

	if(num_oneurons)
	{
		if( version == 0 )
		{
			*num_oneurons = 7;
		}
		else
		{
			string numoneu = params.substr(0,params.find(" ",0));
			params.erase( 0, params.find(" ",0)+1 );			//Remove the numoneu
			*num_oneurons = atoi(numoneu.c_str());
		}
	}
	
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

	int numcols = atoi(numneu.c_str());

	if( FileContents.size() % numcols )		// This will be true anytime we are dealing with a complete brainfunction file.  It will FAIL to be true if we processing an incomplete brainfunction file.
	{
		FileContents.pop_back();                        // get rid of the last line (in complete files, it's useless).
	}
	

	int numrows = int(round( FileContents.size() / numcols ));
	if(lifespan) *lifespan = numrows;

	if( numcols == *num_ineurons ) { *num_ineurons = 0; }	// make sure there was a num_ineurons

	if( (float(numrows) - (FileContents.size() / numcols)) != 0.0 )
	{
		cerr << "Warning: #lines (" << FileContents.size() << ") in brainFunction file '" << fname << "' is not an even multiple of #neurons (" << numcols << ").  brainFunction file may be corrupt." << endl;
	}

	if( ignore_timesteps_after > 0 ) 	// if we are only looking at the first N timestep's of an agent's life...
	{
		numrows = min( numrows, ignore_timesteps_after ); //
		ignore_timesteps_after = min( numrows, ignore_timesteps_after ); //If ignore_timesteps is too big, make it small.
	}

	gsl_matrix * activity = NULL;

	// Make sure the matrix isn't invalid.  If it is, return NULL.
	if( numcols <= 0 || numrows <= 0)
	{
//		cerr << "brainFunction file '" << fname << "' is corrupt.  Number of columns is zero." << endl;
		activity = NULL;
		return activity;
	}

	assert( numcols > 0 && numrows > 0 );		// double checking that we're not going to allocate an impossible matrix.
	activity = gsl_matrix_alloc(numrows, numcols);

	int tcnt=0;

	list<string>::iterator FileContents_it;

	// For efficiency, we have a DIFFERENT FOR LOOP depending on whether ignore_timesteps_after is set.
	if( ignore_timesteps_after == 0 )	// read in the entire agent's lifetime
	{
		for( FileContents_it = FileContents.begin(); FileContents_it != FileContents.end(); FileContents_it++)
		{
			int thespace = (*FileContents_it).find(" ",0);
			int    tstep1 = atoi( ((*FileContents_it).substr( 0, thespace)).c_str() );
			double  tstep2 = atof( ((*FileContents_it).substr( thespace, (*FileContents_it).length())).c_str() );
	
			gsl_matrix_set( activity, int(tcnt/numcols), tstep1, tstep2);
//			cout << "set (" << int(tcnt/numcols) << "," << tstep1 << ") to " << tstep2 << " Matrix Size: " << numrows << "x" << numcols << "||| thespace = " << thespace << endl;
			tcnt++;
		}
	}
	else	// read in the agent's lifetime only up until ignore_timesteps_after
	{
		for( FileContents_it = FileContents.begin(); int(tcnt/numcols) < ignore_timesteps_after; FileContents_it++)
		{
			int thespace = (*FileContents_it).find(" ",0);
			int    tstep1 = atoi( ((*FileContents_it).substr( 0, thespace)).c_str() );
			double  tstep2 = atof( ((*FileContents_it).substr( thespace, (*FileContents_it).length())).c_str() );
	
			gsl_matrix_set( activity, int(tcnt/numcols), tstep1, tstep2);
//			cout << "set (" << int(tcnt/numcols) << "," << tstep1 << ") to " << tstep2 << " Matrix Size: " << numrows << "x" << numcols << "||| thespace = " << thespace << endl;
			tcnt++;
		}



	}
/* MATLAB CODE:
        fitness = str2num(nextl(15:end));
        eval(['save M',fname,'.mat']);
        fclose(fid);
        Mname = ['M',fname,'.mat'];
*/


	return activity;
}

// eof
