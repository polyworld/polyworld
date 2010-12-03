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

	gray = get( "GrayCoding" );

#define ST(NAME) NAME = schema->getSynapseType(#NAME)
	ST(EE);
	ST(EI);
	ST(IE);
	ST(II);
#undef ST

	CONNECTION_DENSITY = gene("ConnectionDensity");
	TOPOLOGICAL_DISTORTION = gene("TopologicalDistortion");
	LEARNING_RATE = gene("LearningRate");
	MISC_BIAS = gene("MiscBias");
	MISC_INVIS_SLOPE = gene("MiscInvisSlope");
	INHIBITORY_COUNT = gene("InhibitoryNeuronCount");
	EXCITATORY_COUNT = gene("ExcitatoryNeuronCount");
	BIAS = gene("Bias");
	BIAS_LEARNING_RATE = gene("BiasLearningRate");
	INTERNAL = schema->getGroupGene( schema->getFirstGroup(NGT_INTERNAL) );

	nbytes = schema->getMutableSize();

	alloc();
}

Genome::~Genome()
{
	delete mutable_data ;
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
	return gene->to_NonVector()->get( this );
}

Scalar Genome::get( Gene *gene,
					 int group )
{
	return gene->to_NeurGroupAttr()->get( this,
										  group );
}

Scalar Genome::get( Gene *gene,
					 SynapseType *synapseType,
					 int from,
					 int to )
{
	return gene->to_SynapseAttr()->get( this,
										synapseType,
										from,
										to );
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

int Genome::getGroupCount( NeurGroupType type )
{
	if( (type == NGT_INPUT) || (type == NGT_OUTPUT) )
	{
		return schema->getMaxGroupCount( type );
	}
	else
	{
		int noninternal;

		switch( type )
		{
		case NGT_ANY:
			noninternal = schema->getMaxGroupCount( NGT_INPUT )
				+ schema->getMaxGroupCount( NGT_OUTPUT );
			break;
		case NGT_INTERNAL:
			noninternal = 0;
			break;
		case NGT_NONINPUT:
			noninternal = schema->getMaxGroupCount( NGT_OUTPUT );
			break;
		default:
			assert(false);
		}

		return noninternal + get( INTERNAL );
	}
}

int Genome::getNeuronCount( NeuronType type,
							 int group )
{
	NeurGroupGene *g = schema->getGroupGene( group );

	switch( g->getGroupType() )
	{
	case NGT_INPUT:
	case NGT_OUTPUT:
		return get( g );
	case NGT_INTERNAL:
		switch(type)
		{
		case INHIBITORY:
			return get( INHIBITORY_COUNT,
						group );
		case EXCITATORY:
			return get( EXCITATORY_COUNT,
						group );
		default:
			assert(false);
		}
	default:
		assert(false);
	}
}

int Genome::getNeuronCount( int group )
{
	switch( schema->getNeurGroupType(group) )
	{
	case NGT_INPUT:
	case NGT_OUTPUT:
		return getNeuronCount( INHIBITORY,
							   group );
	case NGT_INTERNAL:
		return getNeuronCount( INHIBITORY,
							   group ) 
			+ getNeuronCount( EXCITATORY,
							  group );
	default:
		assert(false);
	}
}

const SynapseTypeList &Genome::getSynapseTypes()
{
	return schema->getSynapseTypes();
}

int Genome::getSynapseCount( SynapseType *synapseType,
							  int from,
							  int to )
{
	NeuronType nt_from = synapseType->nt_from;
	NeuronType nt_to = synapseType->nt_to;
	bool to_output = schema->getNeurGroupType(to) == NGT_OUTPUT;

	if( (nt_to == INHIBITORY) && to_output )
	{
		// As targets, the output neurons are treated exclusively as excitatory
		return 0;
	}

	float cd = get( CONNECTION_DENSITY,
					synapseType,
					from,
					to );

	int nfrom = getNeuronCount( nt_from, from );
	int nto = getNeuronCount( nt_to, to );

	if( from == to )
	{
		if( (synapseType == IE) && to_output )
		{
			// If the source and target groups are the same, and both are an output
			// group, then this will evaluate to zero.
			nto--;
		}
		else if( nt_from == nt_to )
		{
			nto--;
		}
	}

	return nint(cd * nfrom * nto);
}

int Genome::getSynapseCount( int from,
							 int to )
{
	int n = 0;

	for( SynapseTypeList::const_iterator
			 it = schema->getSynapseTypes().begin(),
			 end = schema->getSynapseTypes().end();
		 it != end;
		 it++ )
	{
		n += getSynapseCount( *it, from, to );
	}

	return n;
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

void Genome::seed( Gene *attr,
					Gene *group,
					float rawval_ratio )
{
	SEEDCHECK(rawval_ratio);

	attr->to_NeurGroupAttr()->seed( this,
									group->to_NeurGroup(),
									SEEDVAL(rawval_ratio) );
}

void Genome::seed( Gene *gene,
					SynapseType *synapseType,
					Gene *from,
					Gene *to,
					float rawval_ratio )
{
	SEEDCHECK(rawval_ratio);

	gene->to_SynapseAttr()->seed( this,
								  synapseType,
								  from->to_NeurGroup(),
								  to->to_NeurGroup(),
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
	randomize( gene("BitProbability")->to_ImmutableInterpolated()->interpolate(randpw()) );
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
    
	// allocate crossover buffer on stack -- fast & automatically free'd
	long *crossoverPoints = (long *)alloca( numCrossPoints * sizeof(long) );

	long numphysbytes = schema->getPhysicalCount();

    // guarantee crossover in "physiology" genes
    crossoverPoints[0] = long(randpw() * numphysbytes * 8 - 1);
    crossoverPoints[1] = numphysbytes * 8;

	// Sanity checking
	assert(numCrossPoints <= get( "CrossoverPointCount_max" ));
	
	// Generate & order the crossover points.
	// Start iteration at [2], as [0], [1] set above
    long i, j;
    
    for (i = 2; i <= numCrossPoints; i++) 
    {
        long newCrossPoint = long(randpw() * (nbytes - numphysbytes) * 8 - 1) + crossoverPoints[1];
        bool equal;
        do
        {
            equal = false;
            for (j = 1; j < i; j++)
            {
                if (newCrossPoint == crossoverPoints[j])
                    equal = true;
            }
            
            if (equal)
                newCrossPoint = long(randpw() * (nbytes - numphysbytes) * 8 - 1) + crossoverPoints[1];
                
        } while (equal);
        
        if (newCrossPoint > crossoverPoints[i-1])
        {
            crossoverPoints[i] = newCrossPoint;  // happened to come out ordered
		}           
        else
        {
            for (j = 2; j < i; j++)
            {
                if (newCrossPoint < crossoverPoints[j])
                {
                    for (long k = i; k > j; k--)
                        crossoverPoints[k] = crossoverPoints[k-1];
                    crossoverPoints[j] = newCrossPoint;
                    break;
                }
            }
        }
    }
    
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

void Genome::set_raw( int offset,
					  int n,
					  unsigned char val )
{
	assert( (offset >= 0) && (offset + n <= nbytes) );

	for( int i = 0; i < n; i++ )
	{
		int layoutOffset = layout->getMutableDataOffset( offset + i );

		mutable_data[layoutOffset] = val;
	}
}

void Genome::alloc()
{
	mutable_data = new unsigned char[nbytes];
}
