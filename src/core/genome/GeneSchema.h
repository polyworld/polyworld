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
	// === CLASS GeneSchema
	// ===
	// ================================================================================
	class GeneSchema
	{
	public:
		GeneSchema();
		virtual ~GeneSchema();

		virtual Gene *add( Gene *gene );

		Gene *get( const std::string &name );
		Gene *get( const char *name );
		Gene *get( const GeneType *type );
		const GeneVector &getAll( const GeneType *type );
		const GeneVector &getAll();

		int getGeneCount( const GeneType *type );

		int getMutableSize();

		virtual void complete( int offset = 0 );

		void getIndexes( std::vector<std::string> &geneNames, std::vector<int> &result );

		void printIndexes( FILE *f, GenomeLayout *layout = NULL, const std::string &prefix = "" );
		void printTitles( FILE *f, const std::string &prefix = "" );
		void printRanges( FILE *f, const std::string &prefix = ""  );

	protected:
		void beginComplete( int offset );
		void endComplete();

		enum State
		{
			STATE_CONSTRUCTING,
			STATE_CACHING,
			STATE_COMPLETE
		} _state;
		int _offset;

		GeneMap _name2gene;
		GeneTypeMap _type2genes;
		GeneVector _genes;

		struct Cache
		{
			int mutableSize;
		} _cache;
	};

} // namespace genome
