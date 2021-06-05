#include "Genome.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "GenomeLayout.h"
#include "utils/AbstractFile.h"


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
#define SEEDVAL(VAL) (unsigned char)((VAL) == 1 ? 255 : (VAL) * 256)

void Genome::seed( Gene *gene,
				   float rawval_ratio )
{
	SEEDCHECK(rawval_ratio);

	gene->seed( this,
				SEEDVAL(rawval_ratio) );
}

void Genome::seedRandom( Gene *gene,
						 float rawval_ratio_min,
						 float rawval_ratio_max )
{
	SEEDCHECK(rawval_ratio_min);
	SEEDCHECK(rawval_ratio_max);

	gene->randomize( this,
					 SEEDVAL(rawval_ratio_min),
					 SEEDVAL(rawval_ratio_max) );
}

void Genome::seedAll( float rawval_ratio )
{
	SEEDCHECK(rawval_ratio);

	unsigned char val = SEEDVAL(rawval_ratio);
	for (long byte = 0; byte < nbytes; byte++)
	{
		mutable_data[byte] = val;
	}
}

void Genome::randomizeBits( float bitonprob )
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

void Genome::randomizeBits()
{
	randomizeBits( GeneType::to_ImmutableInterpolated(gene("BitProbability"))->interpolate(randpw()) );
}

void Genome::randomizeBytes()
{
	set_raw_random( 0, nbytes, 0, 255 );
}

void Genome::randomize()
{
	if (GenomeSchema::config.resolution == GenomeSchema::RESOLUTION_BIT)
		randomizeBits();
	else if (GenomeSchema::config.resolution == GenomeSchema::RESOLUTION_BYTE)
		randomizeBytes();
	else
		assert( false );
}

void Genome::mutateBits( float rate )
{
    for (long byte = 0; byte < nbytes; byte++)
    {
        for (long bit = 0; bit < 8; bit++)
        {
            if (randpw() < rate)
                mutable_data[byte] ^= char(1 << (7-bit));
        }
    }
}

void Genome::mutateBits()
{
    mutateBits( get( "MutationRate" ) );
}

void Genome::mutateOneByte( long byte, float stdev )
{
    int val = round( nrand( mutable_data[byte], stdev ) );
    mutable_data[byte] = clamp( val, 0, 255 );
}

void Genome::mutateBytes( float rate )
{
    float stdev = pow( 2.0, get( "MutationStdevPower" ) );
    for (long byte = 0; byte < nbytes; byte++)
    {
        if (randpw() < rate)
            mutateOneByte( byte, stdev );
    }
}

void Genome::mutateBytes()
{
    mutateBytes( get( "MutationRate" ) );
}

void Genome::mutate( float rate )
{
	if (!GenomeSchema::config.enableEvolution)
		return;
	if (GenomeSchema::config.resolution == GenomeSchema::RESOLUTION_BIT)
		mutateBits( rate );
	else if (GenomeSchema::config.resolution == GenomeSchema::RESOLUTION_BYTE)
		mutateBytes( rate );
	else
		assert( false );
}

void Genome::mutate()
{
	mutate( get( "MutationRate" ) );
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

	if (!GenomeSchema::config.enableEvolution)
		numCrossPoints = 0;

	if( numCrossPoints == 0 )
	{
		Genome *gTemplate = (randpw() < 0.5) ? g1 : g2;

		copyFrom( gTemplate );
		if( mutate )
			this->mutate();
		return;
	}


	// allocate crossover buffer on stack -- fast & automatically free'd
	long *crossoverPoints = (long *)alloca( numCrossPoints * sizeof(long) );

	// figure out crossover points -- derived class logic.
	getCrossoverPoints( crossoverPoints, numCrossPoints );

    long i, j;

#ifdef DUMPBITS
    if (GenomeSchema::config.resolution == GenomeSchema::RESOLUTION_BIT)
    {
        cout << "**The crossover bits(bytes) are:" nl << "  ";
        for (i = 0; i < numCrossPoints; i++)
        {
            long byte = crossoverPoints[i] >> 3;
            cout << crossoverPoints[i] << "(" << byte << ") ";
        }
        cout nlf;
    }
    else if (GenomeSchema::config.resolution == GenomeSchema::RESOLUTION_BYTE)
    {
        cout << "**The crossover bytes are:" nl << "  ";
        for (i = 0; i < numCrossPoints; i++)
        {
            cout << crossoverPoints[i] << " ";
        }
        cout nlf;
    }
    else
    {
        assert( false );
    }
#endif

    long begbyte = 0;
    long endbyte = -1;
    long bit;
    bool first = (randpw() < 0.5);
    const Genome* ga;
    const Genome* gb;

	// now do crossover using the ordered pts
    for (i = 0; i <= numCrossPoints; i++)
    {
		// for copying the end of the genome
        if (i == numCrossPoints)
        {
            endbyte = nbytes;
        }
        else
        {
            if (GenomeSchema::config.resolution == GenomeSchema::RESOLUTION_BIT)
                endbyte = crossoverPoints[i] >> 3;
            else if (GenomeSchema::config.resolution == GenomeSchema::RESOLUTION_BYTE)
                endbyte = crossoverPoints[i];
            else
                assert( false );
		}
        ga = first ? g1 : g2;
        gb = first ? g2 : g1;

#ifdef DUMPBITS
        cout << "**copying bytes " << begbyte << " to " << endbyte
             << " from the ";
        if (first)
            cout << "first genome" nl;
        else
            cout << "second genome" nl;
        cout.flush();
#endif

        for (j = begbyte; j < endbyte; j++)
            mutable_data[j] = ga->mutable_data[j];    // copy from the appropriate genome

        if (i < numCrossPoints)  // except on the last stretch...
        {
            if (GenomeSchema::config.resolution == GenomeSchema::RESOLUTION_BIT)
            {
                bit = crossoverPoints[i] - (endbyte << 3);
                // this goes left to right, corresponding more directly to little-endian machines, but leave it alone (at least for now)
                mutable_data[endbyte] = char((ga->mutable_data[endbyte] & (255 << (8 - bit)))
                                        | (gb->mutable_data[endbyte] & (255 >> bit)));
            }
            else if (GenomeSchema::config.resolution == GenomeSchema::RESOLUTION_BYTE)
            {
                mutable_data[endbyte] = gb->mutable_data[endbyte];
            }
            else
            {
                assert( false );
            }
        }

        first = !first;
        begbyte = endbyte + 1;
    }

    if (mutate)
        this->mutate();
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
