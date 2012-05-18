#pragma once

#include "Genome.h"

namespace genome
{
	class SheetsGenome : public Genome
	{
	public:
		SheetsGenome( class SheetsGenomeSchema *schema,
					  GenomeLayout *layout );
		virtual ~SheetsGenome();

		Brain *createBrain( NervousSystem *cns );

		virtual void crossover( Genome *g1,
								Genome *g2,
								bool mutate );

	private:
		SheetsGenomeSchema *_schema;
	};
}
