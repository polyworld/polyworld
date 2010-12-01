#pragma once

#include <assert.h>

namespace genome
{
	// forward decls
	class GenomeSchema;

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
			LEGACY,
			NEURGROUP
		};

		static GenomeLayout *create( GenomeSchema *schema,
									 LayoutType type );
		
	public:
		int getMutableDataOffset( int geneOffset );

	private:
		static void createLegacy( GenomeLayout *layout );
		static void createNeurGroup( GenomeLayout *layout,
									 GenomeSchema *schema );

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
	inline int GenomeLayout::getMutableDataOffset( int geneOffset )
	{
		assert( (geneOffset < numOffsets) && (geneOffset >= 0) );

		return geneOffset2mutableDataOffset[ geneOffset ];
	}

}
