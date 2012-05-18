#include "Genome.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "AbstractFile.h"
#include "GenomeLayout.h"
#include "misc.h"


#ifdef __ALTIVEC__
	// Turn pw_UseAltivec on to use ALTIVEC if it's available
	#define pw_UseAltivec false
#endif

#if pw_UseAltivec
	// veclib
	#include <vecLib/vBLAS.h>
#endif

using namespace genome;
using namespace std;

// ================================================================================
// ===
// === CLASS Genome
// ===
// ================================================================================

Genome::Genome( GenomeSchema *schema,
				GenomeLayout *layout )
{
	this->schema = schema;
	this->layout = layout;

	MISC_BIAS = gene("MiscBias");
	MISC_INVIS_SLOPE = gene("MiscInvisSlope");
	gray = GenomeSchema::config.grayCoding;

	nbytes = schema->getMutableSize();

	alloc();
}

Genome::~Genome()
{
	delete [] mutable_data ;
}

Gene *Genome::gene( const char *name )
{
	return schema->get( name );
}

Scalar Genome::get( const char *name )
{
	return get( gene(name) );
}

Scalar Genome::get( Gene *gene )
{
	return GeneType::to_NonVector( gene )->get( this );
}

unsigned int Genome::get_raw_uint( long byte )
{
	return (unsigned int)get_raw( byte );
}

void Genome::updateSum( unsigned long *sum, unsigned long *sum2 )
{
	// This function is more verbose than necessary because we're optimizing
	// for speed. This loop is run *a lot*. So, we move the if(gray) outside
	// the loop and implement the get_raw() logic in here.
	if( gray )
	{
		for( int i = 0; i < nbytes; i++ )
		{
			int layoutOffset = layout->getMutableDataOffset_nocheck( i );
			unsigned long raw = (unsigned long)mutable_data[layoutOffset];

			raw = binofgray[raw];

			sum[i] += raw;
			sum2[i] += raw * raw;
		}
	}
	else
	{
		for( int i = 0; i < nbytes; i++ )
		{
			int layoutOffset = layout->getMutableDataOffset_nocheck( i );
			unsigned long raw = (unsigned long)mutable_data[layoutOffset];

			sum[i] += raw;
			sum2[i] += raw * raw;
		}
	}
}

#define SEEDCHECK(VAL) assert(((VAL) >= 0) && ((VAL) <= 1))
#define SEEDVAL(VAL) (unsigned char)((VAL) * 255)

void Genome::seed( Gene *gene,
				   float rawval_ratio )
{
	SEEDCHECK(rawval_ratio);

	gene->seed( this,
				SEEDVAL(rawval_ratio) );
}

void Genome::randomize( float bitonprob )
{
	// do a random initialization of the bitstring
    for (long byte = 0; byte < nbytes; byte++)
    {
        for (long bit = 0; bit < 8; bit++)
        {
            if (randpw() < bitonprob)
                mutable_data[byte] |= char(1 << (7-bit));
            else
                mutable_data[byte] &= char(255 ^ (1 << (7-bit)));
		}
	}		              	
}

void Genome::randomize()
{
	randomize( GeneType::to_ImmutableInterpolated(gene("BitProbability"))->interpolate(randpw()) );
}

void Genome::mutate()
{
	float rate = get( "MutationRate" );

    for (long byte = 0; byte < nbytes; byte++)
    {
        for (long bit = 0; bit < 8; bit++)
        {
            if (randpw() < rate)
                mutable_data[byte] ^= char(1 << (7-bit));
        }
    }
}

void Genome::crossover( Genome *g1, Genome *g2, bool mutate )
{
	assert(g1 != NULL && g2 != NULL);
	assert(g1 != g2);
	assert(mutable_data != NULL);

    // Randomly select number of crossover points from chosen genome
    long numCrossPoints;
    if (randpw() < 0.5)
        numCrossPoints = g1->get( "CrossoverPointCount" );
    else
		numCrossPoints = g2->get( "CrossoverPointCount" );

	if( numCrossPoints == 0 )
	{
		Genome *gTemplate = (randpw() < 0.5) ? g1 : g2;

		copyFrom( gTemplate );
		if( mutate )
			this->mutate();
		return;
	}

	// Sanity checking
	assert( numCrossPoints <= GenomeSchema::config.maxNumCpts );
    
	// allocate crossover buffer on stack -- fast & automatically free'd
	long *crossoverPoints = (long *)alloca( numCrossPoints * sizeof(long) );

	// figure out crossover points -- derived class logic.
	getCrossoverPoints( crossoverPoints, numCrossPoints );
	
    long i, j;
    
#ifdef DUMPBITS    
    cout << "**The crossover bits(bytes) are:" nl << "  ";
    for (i = 0; i <= numCrossPoints; i++)
    {
        long byte = crossoverPoints[i] >> 3;
        cout << crossoverPoints[i] << "(" << byte << ") ";
    }
    cout nlf;
#endif
    
	float mrate = 0.0;
    if (mutate)
    {
        if (randpw() < 0.5)
            mrate = g1->get( "MutationRate" );
        else
            mrate = g2->get( "MutationRate" );
    }
    
    long begbyte = 0;
    long endbyte = -1;
    long bit;
    bool first = (randpw() < 0.5);
    const Genome* g;
    
	// now do crossover using the ordered pts
    for (i = 0; i <= numCrossPoints + 1; i++)
    {
		// for copying the end of the genome
        if (i == numCrossPoints + 1)
        {
            if (endbyte == nbytes - 1)
            {
            	// already copied last byte (done) so just get out of the loop
                break;
			}             
                
            endbyte = nbytes - 1;
        }
        else
        {
            endbyte = crossoverPoints[i] >> 3;
		}
        g = first ? g1 : g2;
        
#ifdef DUMPBITS    
        cout << "**copying bytes " << begbyte << " to " << endbyte
             << " from the ";
        if (first)
            cout << "first genome" nl;
        else
            cout << "second genome" nl;
        cout.flush();
#endif

        if (mutate)
        {
            for (j = begbyte; j < endbyte; j++)
            {
                mutable_data[j] = g->mutable_data[j];    // copy from the appropriate genome
                for (bit = 0; bit < 8; bit++)
                {
                    if (randpw() < mrate)
                        mutable_data[j] ^= char(1 << (7-bit));	// this goes left to right, corresponding more directly to little-endian machines, but leave it alone (at least for now)
                }
            }
        }
        else
        {
            for (j = begbyte; j < endbyte; j++)
                mutable_data[j] = g->mutable_data[j];    // copy from the appropriate genome
        }
        
        if (i != (numCrossPoints + 1))  // except on the last stretch...
        {
            first = !first;
            bit = crossoverPoints[i] - (endbyte << 3);
            
            if (first)
            {	// this goes left to right, corresponding more directly to little-endian machines, but leave it alone (at least for now)
                mutable_data[endbyte] = char((g2->mutable_data[endbyte] & (255 << (8 - bit)))
                   					 | (g1->mutable_data[endbyte] & (255 >> bit)));
			}                  
            else
            {	// this goes left to right, corresponding more directly to little-endian machines, but leave it alone (at least for now)
                mutable_data[endbyte] = char((g1->mutable_data[endbyte] & (255 << (8 - bit)))
                   					 | (g2->mutable_data[endbyte] & (255 >> bit)));
			}
						              
            begbyte = endbyte + 1;
        }
        else
        {
            mutable_data[endbyte] = g->mutable_data[endbyte];
		}
		            
        if (mutate)
        {
            for (bit = 0; bit < 8; bit++)
            {
                if (randpw() < mrate)
                    mutable_data[endbyte] ^= char(1 << (7 - bit));	// this goes left to right, corresponding more directly to little-endian machines, but leave it alone (at least for now)
            }
        }
    }
}

void Genome::copyFrom( Genome *g )
{
	assert( schema == g->schema );

	memcpy( mutable_data, g->mutable_data, nbytes );
}

float Genome::separation( Genome *g )
{
	assert( schema == g->schema );

#if pw_UseAltivec
	#warning compiling with (broken) ALTIVEC code
#else
	int sep = 0;
#endif
	float fsep = 0.f;
    unsigned char* gi = mutable_data;
    unsigned char* gj = g->mutable_data;
    
    if( gray )
    {
#if pw_UseAltivec
		float val_gi[ nbytes ];
		float val_gj[ nbytes ];
		short size = 4;

                long max = nbytes / size;
                long left = nbytes - (max * size);

		float result[ max * size ];

		for (long i = 0; i < nbytes; i++)
		{
			val_gi[i] = binofgray[gi[i]];
			val_gj[i] = binofgray[gj[i]];
		}
		

		for (long i = 0; i < max; i++)
		{
			vector float vGi = vec_ld(size, val_gi + size * i);
			vector float vGj = vec_ld(size, val_gj + size * i);
			
			vector float diff = vec_sub(vGi, vGj);
			
			vec_st(diff, size, result + i * size);
			
		}
		
		fsep = cblas_sasum(size * max, result, 1);
		vector float vGi = vec_ld(left, val_gi + size * max);
		vector float vGj = vec_ld(left, val_gj + size * max);
		
		vector float diff = vec_sub(vGi, vGj);
		vector float absdiff = vec_abs(diff);
		
		vec_st(absdiff, left, result);
		
		for (long j = 0; j < left ;  j++)
		{
			fsep += result[j];
		}
#else
    for (long i = 0; i < nbytes; i++)
        {
            short vi, vj;
            vi = binofgray[ * (gi++)];
            vj = binofgray[ * (gj++)];
            sep += abs(vi - vj);
        }
#endif
    }
    else
    {
#if pw_UseAltivec
		float val_gi[ nbytes ];
		float val_gj[ nbytes ];
		short size = 4;
		
		long max = nbytes / size;
		long left = nbytes - (max * size);
		
		float result[ max * size ];
		
		for (long i = 0; i < nbytes; i++)
		{
			val_gi[i] = gi[i];
			val_gj[i] = gj[i];
		}
		
		
		for (long i = 0; i < max; i++)
		{
			vector float vGi = vec_ld(size, val_gi + size * i);
			vector float vGj = vec_ld(size, val_gj + size * i);
			
			vector float diff = vec_sub(vGi, vGj);
			
			vec_st(diff, size, result + i * size);
			
		}
		
		fsep = cblas_sasum(size * max, result, 1);
		vector float vGi = vec_ld(left, val_gi + size * max);
		vector float vGj = vec_ld(left, val_gj + size * max);
		
		vector float diff = vec_sub(vGi, vGj);
		vector float absdiff = vec_abs(diff);
		
		vec_st(absdiff, left, result);
		
		for (long j = 0; j < left ;  j++)
		{
			fsep += result[j];
		}
#else
        for (long i = 0; i < nbytes; i++)
        {
            short vi, vj;
            vi = * (gi++);
            vj = * (gj++);
            sep += abs(vi - vj);
        }
#endif
    }
#if pw_UseAltivec
    fsep /= (255.0f * float(nbytes));
    return fsep;
#else
	fsep = float(sep) / (255 * nbytes);
    return fsep;
#endif
	
}

float Genome::mateProbability( Genome *g )
{
	double miscbias = get( MISC_BIAS );

    // returns probability that two agents will successfully mate
    // based on their degree of genetic similarity/difference
    if( miscbias == 0.0 )
        return 1.0;
        
    float a = separation( g );
    float cosa = cos( pow(a, miscbias) * PI );
    float s = cosa > 0.0 ? 0.5 : -0.5;
    float p = 0.5  +  s * pow(fabs(cosa), get(MISC_INVIS_SLOPE));
    
    return p;
}

void Genome::dump( AbstractFile *out )
{
	for( int i = 0; i < nbytes; i++ )
	{
		out->printf( "%d\n", get_raw(i) );
	}
}

void Genome::load( AbstractFile *in )
{
    int num = 0;
    for (long i = 0; i < nbytes; i++)
    {
		int rc = in->scanf( "%d\n", &num );
		if( rc != 1 )
		{
			// not enough genes
			cerr << "Failure in reading seed file " << in->getAbstractPath() << endl;
			cerr << "Probably due to genome schema mismatch." << endl;
			exit( 1 );
		}
		set_raw( i, 1, num );
    }

	int rc = in->scanf( "%d\n", &num );
	if( rc > 0 )
	{
		// too many genes
		cerr << "Unexpected data in seed file after genome bytes: " << num << endl;
		cerr << "Probably due to genome schema mismatch." << endl;
		exit( 1 );
	}
}

void Genome::print()
{
	long lobit = 0;
	long hibit = nbytes * 8;
	print( lobit, hibit );
}

void Genome::print( long lobit, long hibit )
{
    cout << "genome bits " << lobit << " through " << hibit << " =" nl;
    for (long i = lobit; i <= hibit; i++)
    {
        long byte = i >> 3; // 0-based
        long bit = i % 8; // 0-based,from left
        cout << ((get_raw(byte) >> (7-bit)) & 1);
    }
    cout nlf;
}

void Genome::getCrossoverPoints( long *crossoverPoints, long numCrossPoints )
{
	// Derived class must either override crossover() or implement this function.
	assert( false );
}

void Genome::alloc()
{
	mutable_data = new unsigned char[nbytes];
}
