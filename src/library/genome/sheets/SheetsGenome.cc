#include "SheetsGenome.h"

#include <iostream>

#include "SheetsBrain.h"
#include "SheetsGenomeSchema.h"

using namespace std;
using namespace genome;


SheetsGenome::SheetsGenome( SheetsGenomeSchema *schema,
							GenomeLayout *layout )
: Genome( schema, layout )
, _schema( schema )
{
}

SheetsGenome::~SheetsGenome()
{
}

Brain *SheetsGenome::createBrain( NervousSystem *cns )
{
	return new SheetsBrain( cns, this, _schema->createSheetsModel(this) );
}

void SheetsGenome::crossover( Genome *g1,
							  Genome *g2,
							  bool mutate )
{
	SheetsGenome *genomes[] = { dynamic_cast<SheetsGenome *>(g1),
								dynamic_cast<SheetsGenome *>(g2) };
	int genomeIndex = 0;

	SheetsCrossover *crossover = _schema->getCrossover();
	SheetsCrossover::Segment segment;

	while( crossover->nextSegment(segment) )
	{
		SheetsGenome *g = genomes[ genomeIndex++ % 2 ];

		/*
		cout << g << " [" << segment.start << "," << segment.end << "]";
		if( segment.level ) cout << " " << segment.level->name;
		cout << endl;
		*/

		memcpy( mutable_data + segment.start,
				g->mutable_data + segment.start,
				segment.end - segment.start + 1 );
	}

	if( mutate )
		this->mutate();
}
