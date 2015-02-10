#pragma once

#include <assert.h>

namespace genome
{
	// forward decls
	class GenomeSchema;
	class GroupsGenomeSchema;

	// ================================================================================
	// ===
	// === CLASS GenomeLayout
	// ===
	// ================================================================================
	class GenomeLayout
	{
	public:
		enum LayoutType
		{
			None,
			NeurGroup
		};

		static GenomeLayout *create( GenomeSchema *schema,
									 LayoutType type );
		
	public:
		int getMutableDataOffset( int geneOffset );
		int getMutableDataOffset_nocheck( int geneOffset );

	private:
		static void createNone( GenomeLayout *layout );
		static void createNeurGroup( GenomeLayout *layout,
									 GroupsGenomeSchema *schema );

	private:
		GenomeLayout( GenomeSchema *schema );
		~GenomeLayout();

		void validate();

		int numOffsets;
		int *geneOffset2mutableDataOffset;
	};

	//===========================================================================
	// inlines
	//===========================================================================
	inline int GenomeLayout::getMutableDataOffset_nocheck( int geneOffset )
	{
		return geneOffset2mutableDataOffset[ geneOffset ];
	}

	inline int GenomeLayout::getMutableDataOffset( int geneOffset )
	{
		assert( (geneOffset < numOffsets) && (geneOffset >= 0) );

		return getMutableDataOffset_nocheck( geneOffset );
	}

}
