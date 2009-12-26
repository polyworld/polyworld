#pragma once

#include "Genome.h"

namespace genome
{
	class GenomeUtil
	{
	public:
		static GenomeSchema *createSchema();
		static Genome *createGenome( bool randomized = false );
		static void randomize( Genome *g );
		static void seed( Genome *g );

		static void test();

	public:
		static GenomeSchema *schema;
	};

}
