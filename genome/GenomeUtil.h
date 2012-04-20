#pragma once

#include "Genome.h"
#include "GroupsGenome.h"

// forward decls
class Metabolism;

namespace genome
{
	// forward decls
	class GenomeLayout;

	class GenomeUtil
	{
	public:
		static GenomeSchema *createSchema();
		static Genome *createGenome( bool randomized = false );
		static void randomize( Genome *g );
		static void seed( Genome *g );
		static const Metabolism *getMetabolism( Genome *g );

		static Gene *getGene( const std::string &name, const std::string &err = "" );

	public:
		static GenomeSchema *schema;
		static GenomeLayout *layout;
	};

}
