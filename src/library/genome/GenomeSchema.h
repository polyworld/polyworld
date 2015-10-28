#pragma once

#include "GeneSchema.h"
#include "proplib/proplib.h"

namespace genome
{
	// ================================================================================
	// ===
	// === CLASS GenomeSchema
	// ===
	// ================================================================================
	class GenomeSchema : public GeneSchema
	{
	public:
		static struct Configuration
		{
			GenomeLayout::LayoutType layoutType;

			typedef std::map<std::string,float> GeneInterpolationPowers;
			GeneInterpolationPowers geneInterpolationPower;

			bool simpleSeed;
			bool enableEvolution;

			float seedMutationRate;
			float seedFightBias;
			float seedFightExcitation;
			float seedGiveBias;
			float seedPickupBias;
			float seedDropBias;
			float seedPickupExcitation;
			float seedDropExcitation;

			float minMutationRate;
			float maxMutationRate;
			float minMutationStdev;
			float maxMutationStdev;
			long minNumCpts;
			long maxNumCpts;
			float miscBias;
			float miscInvisSlope;
			float minBitProb;
			float maxBitProb;
			float byteMean;
			float byteStdev;
			bool grayCoding;
		} config;

		static void processWorldfile( proplib::Document &doc );

		GenomeSchema();
		virtual ~GenomeSchema();

		virtual void define();
		virtual void seed( Genome *genome );

		virtual Genome *createGenome( GenomeLayout *layout ) = 0;
	};

} // namespace genome
