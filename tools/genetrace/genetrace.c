// genetrace.c
//
// Reads a genestats.txt file output by Polyworld and outputs a .dat file suitable for MatLab
//
// Usage:  genetrace [-d numDigitsToProcess (all)] [-m maxSeparationOfDigits (1)] [digits_file (stdin)]

#define DebugGeneTrace 1

#include <stdio.h>
#include <stdarg.h>
#include <math.h>
#include <unistd.h>

#include "ArgParse.h"
#include "Stream.h"

#define chkptr( p, s... ) if( !p ) { fprintf( stderr, s ); exit( 1 ); }

#if DebugGeneTrace
	#define dbgPrintf( x... ) printf( x )
#else
	#define dbgPrintf( x... )
#endif

void usage( char* arg0 );
void args( int argc, char** argv, int* something );

#define NumGenes 2494
#define FirstLearningRateGene 862
#define LastLearningRateGene 1677
// input neuron groups:  r, g, b, energy, random
#define NumInputNeuronGroups 5
// output neuron groups:  eat, mate, fight, move, turn, light, focus
#define NumOutputNeuronGroups 7
#define MinInternalNeuronGroups 0
#define MaxInternalNeuronGroups 5

#define MinLearningRate 0.0
#define MaxLearningRate 0.1

#define ReportFrequency 1000

int MaxNeuronGroups = NumInputNeuronGroups + MaxInternalNeuronGroups + NumOutputNeuronGroups;
double OneOver255 = 1.0 / 255.0;

int main( int argc, char **argv )
{
	int		something;
	Stream	stream;
	int		numGenes;
	int		numVals;
	char*	s;
	char**	val;
	int		numInternalNeuronGroups;
	int		numNeuronGroups;
	int		EELRGeneOffset;
	int		EILRGeneOffset;
	int		IILRGeneOffset;
	int		IELRGeneOffset;
	int		nextGeneOffset;
	int		lastGeneOffset;
	int		i;
	int		j;
	int		line = 0;
	double	meanRate;
	
	// Read arguments
	args( argc, argv, &something );
	
	stream = StreamOpenFileRead( "genestats.txt" );
	
	s = StreamReadUntil( stream, "\n" );
	chkptr( s, "failed initial stream read\n" );
	
	numGenes = atoi( s );
	dbgPrintf( "numGenes = %d\n", numGenes );
	if( numGenes != NumGenes )
	{
		fprintf( stderr, "gene count in file (%d) != compiled gene count (%d)\n", numGenes, NumGenes );
		exit( 1 );
	}
	
	free( s );
	
	s = StreamReadWhile( stream, "\n" );	// read the newline out of the way
	chkptr( s, "failed reading the rest of the first line out of the way\n" );
	free( s );
	
	line++;
	
	printf( "This trace only valid under the following conditions:\n" );
	printf( "  NumGenes = %d (confirmed)\n", NumGenes );
	printf( "  NumInputNeuronGroups = %d\n", NumInputNeuronGroups );
	printf( "  NumOutputNeuronGroups = %d\n", NumOutputNeuronGroups );
	printf( "  MinInternalNeuronGroups = %d\n", MinInternalNeuronGroups );
	printf( "  MaxInternalNeuronGroups = %d\n", MaxInternalNeuronGroups );
	printf( "  MinLearningRate = %03.1f\n", MinLearningRate );
	printf( "  MaxLearningRate = %03.1f\n", MaxLearningRate );
	printf( "  FirstLearningRateGene = %d\n", FirstLearningRateGene );
	printf( "  LastLearningRateGene = %d\n", LastLearningRateGene );
	
	EELRGeneOffset = FirstLearningRateGene * 2;	// *2 to skip std. dev. values
	EILRGeneOffset = EELRGeneOffset  +  MaxNeuronGroups * (MaxNeuronGroups - NumInputNeuronGroups) * 2;	// *2 to skip std. dev. values
	IILRGeneOffset = EILRGeneOffset  +  MaxNeuronGroups * (MaxNeuronGroups - NumInputNeuronGroups) * 2;	// *2 to skip std. dev. values
	IELRGeneOffset = IILRGeneOffset  +  MaxNeuronGroups * (MaxNeuronGroups - NumInputNeuronGroups) * 2;	// *2 to skip std. dev. values
	nextGeneOffset = IELRGeneOffset  +  MaxNeuronGroups * (MaxNeuronGroups - NumInputNeuronGroups) * 2;	// *2 to skip std. dev. values
	lastGeneOffset = nextGeneOffset - 2;
	if( (lastGeneOffset/2) != LastLearningRateGene )
	{
		fprintf( stderr, "failed sanity check, lastGeneOffset/2 (%d) != LastLearningRateGene (%d)\n", lastGeneOffset/2, LastLearningRateGene );
		exit( 1 );
	}

	numVals = numGenes * 2;	// mean and std. dev.
	val = (char**) malloc( sizeof( *val ) * numVals );
	chkptr( val, "unable to allocate val array\n" );
	
	while( (s = StreamReadUntil( stream, "\n" )) )
	{
		char*	t;
		int		step;
		
		line++;
//		dbgPrintf( "line = %d\n", line );
//		dbgPrintf( "%s\n", s );
		
		// Snag just the timestep
		t = s;
		(void) strsep( &t, " \t" );
		step = atoi( s );	// s now has a 0 inserted after the timestep

//		dbgPrintf( "step = %d\n", step );
		
		if( (step == 1) || (step % ReportFrequency == 0) )
		{
			char**	v;
			int		numRates;
			double	geneValue;
			
			// Break up the input string into space-, tab-, or comma-separated values
			for( v = val; (*v = strsep( &t, " ,\t" )) != NULL; )
				if( **v != '\0' )
					if( ++v >= &val[numVals] )
						break;
			
			geneValue = atof( val[11*2] );		// *2 to skip over std. dev. values
			numInternalNeuronGroups = lrint( interp( geneValue * OneOver255, MinInternalNeuronGroups, MaxInternalNeuronGroups ) );

			numNeuronGroups = NumInputNeuronGroups + numInternalNeuronGroups + NumOutputNeuronGroups;
			dbgPrintf( "at step %d, numNeuronGroups: input = %d, internal = %d, output = %d, total = %d, max = %d\n",
						step, NumInputNeuronGroups, numInternalNeuronGroups, NumOutputNeuronGroups, numNeuronGroups, MaxNeuronGroups );
			
			meanRate = 0.0;
			numRates = 0;
			for( i = NumInputNeuronGroups; i < numNeuronGroups; i++ )	// target group
			{
				for( j = 0; j < numNeuronGroups; j++ )	// source group
				{
					double	rate;
					int		groupPairIndex = ((i - NumInputNeuronGroups) * MaxNeuronGroups + j) * 2;	//*2 to skip std. dev. values
					// e-e
					geneValue = atof( val[EELRGeneOffset + groupPairIndex] );
					rate = interp( geneValue * OneOver255, MinLearningRate, MaxLearningRate );
					meanRate += rate;
					numRates++;
					// e-i
					geneValue = atof( val[EILRGeneOffset + groupPairIndex] );
					rate = interp( geneValue * OneOver255, MinLearningRate, MaxLearningRate );
					meanRate += rate;
					numRates++;
					// i-i
					geneValue = atof( val[IILRGeneOffset + groupPairIndex] );
					rate = interp( geneValue * OneOver255, MinLearningRate, MaxLearningRate );
					meanRate += rate;
					numRates++;
					// i-e
					geneValue = atof( val[IELRGeneOffset + groupPairIndex] );
					rate = interp( geneValue * OneOver255, MinLearningRate, MaxLearningRate );
					meanRate += rate;
					numRates++;
				}
			}
			meanRate /= numRates;

			printf( "%g\n", meanRate );
		}
		
		free( s );
		
		s = StreamReadWhile( stream, "\n" );	// read the newline out of the way
		chkptr( s, "failed reading line %d out of the way\n", line );
		free( s );
	}
	
	StreamClose( stream );

	exit( 0 );
}

void args( int argc, char** argv, int* something )
{	
	ArgParse( argv, NULL /* HelpFunc */,
//				"-n %i(numDigitsToProcess,0)", &something,	// 0 = do them all
//				"-s %i(numStateDigits,4)", &something,		// 10 digits back
//				"-o%i<1,0>", something,						// do computation the old way
//       		"%f(digitsFile,required)", &fileName,		// name of file to process
				NULL );

	*something = 1;	
}

void usage( char* arg0 )
{

}
