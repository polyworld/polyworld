#pragma once

#include "GeneSchema.h"
#include "proplib.h"

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
			long minNumCpts;
			long maxNumCpts;
			float miscBias;
			float miscInvisSlope;
			float minBitProb;
			float maxBitProb;
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
