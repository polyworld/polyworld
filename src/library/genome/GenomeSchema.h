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
		enum Resolution
		{
			RESOLUTION_BIT,
			RESOLUTION_BYTE
		};

		enum SeedType
		{
			SEED_LEGACY,
			SEED_SIMPLE,
			SEED_RANDOM
		};

		static struct Configuration
		{
			GenomeLayout::LayoutType layoutType;
			Resolution resolution;
			typedef std::map<std::string,float> GeneInterpolationPowers;
			GeneInterpolationPowers geneInterpolationPower;

			bool dieAtMaxAge;
			bool enableEvolution;

			SeedType seedType;
			float seedMutationRate;
			float simpleSeedYawBiasDelta;
			float seedFightBias;
			float seedFightExcitation;
			float seedGiveBias;
			float seedPickupBias;
			float seedDropBias;
			float seedPickupExcitation;
			float seedDropExcitation;

			float minMutationRate;
			float maxMutationRate;
			float mutationStdev;
			long minNumCpts;
			long maxNumCpts;
			long minLifeSpan;
			long maxLifeSpan;
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
