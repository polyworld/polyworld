#pragma once

#include <stdio.h>

#include <string>
#include <vector>

#include "Gene.h"
#include "GenomeLayout.h"
#include "proplib.h"

namespace genome
{
	// ================================================================================
	// ===
	// === CLASS GenomeSchema
	// ===
	// ================================================================================
	class GenomeSchema
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

		virtual void defineImmutable();
		virtual void defineMutable();
		virtual void seed( Genome *genome );

		virtual Gene *add( Gene *gene );

		Gene *get( const std::string &name );
		Gene *get( const char *name );
		Gene *get( const GeneType *type );
		const GeneVector &getAll( const GeneType *type );

		int getGeneCount( const GeneType *type );

		int getPhysicalCount();
		int getMutableSize();

		virtual void complete() = 0;

		void getIndexes( std::vector<std::string> &geneNames, std::vector<int> &result );

		void printIndexes( FILE *f, GenomeLayout *layout = NULL );
		void printTitles( FILE *f );
		void printRanges( FILE *f );

	protected:
		void beginComplete();
		void endComplete();

		enum State
		{
			STATE_CONSTRUCTING,
			STATE_CACHING,
			STATE_COMPLETE
		} state;

		GeneMap name2gene;
		GeneTypeMap type2genes;
		GeneList genes;

		struct Cache
		{
			int physicalCount;
			int mutableSize;
		} cache;
	};

} // namespace genome
