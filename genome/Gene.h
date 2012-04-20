#pragma once

#include <list>
#include <map>
#include <string>
#include <vector>

#include "cppprops.h"
#include "NeurGroupType.h"
#include "Scalar.h"

namespace genome
{

	// forward decls
	class Genome;
	class GenomeLayout;
	class GenomeSchema;

	// ================================================================================
	// ===
	// === CLASS GeneType
	// ===
	// ================================================================================
	class GeneType
	{
	public:
		static const GeneType *SCALAR;

#define CAST_TO(TYPE)											\
		static class TYPE##Gene *to_##TYPE(class Gene *gene);
		CAST_TO(NonVector);
		CAST_TO(ImmutableScalar);
		CAST_TO(MutableScalar);
		CAST_TO(ImmutableInterpolated);
		CAST_TO(__Interpolated);
#undef CAST_TO
	};

	// ================================================================================
	// ===
	// === CLASS Gene
	// ===
	// ================================================================================
	class Gene
	{
	public:
		virtual ~Gene() {}

		void seed( Genome *genome,
				   unsigned char rawval );

		int getMutableSize();

		virtual void printIndexes( FILE *file, GenomeLayout *layout );
		virtual void printTitles( FILE *file );
		virtual void printRanges( FILE *file );

		const GeneType *type;
		std::string name;
		bool ismutable;

	protected:
		Gene() {}
		void init( const GeneType *type,
				   bool ismutable,
				   const char *name );

		virtual int getMutableSizeImpl();

		GenomeSchema *schema;

	protected:
		friend class GenomeSchema;
		friend class GenomeLayout;

		int offset;
	};


	// ================================================================================
	// ===
	// === CLASS __ConstantGene
	// ===
	// === Common base class for immutable gene types
	// ===
	// ================================================================================
	class __ConstantGene : virtual Gene
	{
	public:
		__ConstantGene( const GeneType *type,
						const char *name,
						const Scalar &value );
		virtual ~__ConstantGene() {}

		const Scalar &get();

	private:
		const Scalar value;
	};


	// ================================================================================
	// ===
	// === CLASS __InterpolatedGene
	// ===
	// === Common base class for genes whose values are derived from a range.
	// === 
	// ================================================================================
	class __InterpolatedGene : virtual Gene
	{
		PROPLIB_CPP_PROPERTIES

	public:
		enum Rounding
		{
			ROUND_NONE,
			ROUND_INT_FLOOR,
			ROUND_INT_NEAREST,
			ROUND_INT_BIN
		};

		__InterpolatedGene( const GeneType *type,
							bool ismutable,
							const char *name,
							Gene *gmin,
							Gene *gmax,
							Rounding rounding );
		virtual ~__InterpolatedGene() {}

		const Scalar &getMin();
		const Scalar &getMax();

		void setInterpolationPower( double interpolationPower );

		Scalar interpolate( unsigned char raw );
		Scalar interpolate( double ratio );

		void printRanges( FILE *file );

	private:
		Scalar smin;
		Scalar smax;
		Rounding rounding;
		double interpolationPower;
	};


	// ================================================================================
	// ===
	// === CLASS NonVectorGene
	// ===
	// === Genome interface; common for gene types that do not contain
	// === multiple elements.
	// ===
	// ================================================================================
	class NonVectorGene : virtual public Gene
	{
	public:
		virtual ~NonVectorGene() {}

		virtual Scalar get( Genome *genome ) = 0;

	protected:
		virtual int getMutableSizeImpl();
	};


	// ================================================================================
	// ===
	// === CLASS ImmutableScalarGene
	// ===
	// === For public use; holds constant values.
	// ===
	// ================================================================================
	class ImmutableScalarGene : public NonVectorGene, private __ConstantGene
	{
	public:
		ImmutableScalarGene( const char *name,
							 const Scalar &value );
		virtual ~ImmutableScalarGene() {}

		virtual Scalar get( Genome *genome );
	};


	// ================================================================================
	// ===
	// === CLASS MutableScalarGene
	// ===
	// === For public use; holds single mutable value interpolated over a range.
	// ===
	// ================================================================================
	class MutableScalarGene : public NonVectorGene, public __InterpolatedGene
	{
	public:
		MutableScalarGene( const char *name,
						   Gene *gmin,
						   Gene *gmax,
						   __InterpolatedGene::Rounding rounding );
		virtual ~MutableScalarGene() {}

		virtual Scalar get( Genome *genome );

		const Scalar &getMin();
		const Scalar &getMax();

		virtual void printRanges( FILE *file );
	};


	// ================================================================================
	// ===
	// === CLASS ImmutableInterpolatedGene
	// ===
	// === For public use; the "raw" value used for interpolation between min and
	// === max is provided externally. That is, the "raw" value does not come from
	// === the mutable data of the Genome.
	// ===
	// ================================================================================
	class ImmutableInterpolatedGene : virtual public Gene, public __InterpolatedGene
	{
	public:
		ImmutableInterpolatedGene( const char *name,
								   Gene *gmin,
								   Gene *gmax,
								   __InterpolatedGene::Rounding rounding );
		virtual ~ImmutableInterpolatedGene() {}

		Scalar interpolate( double ratio );
	};


	// ================================================================================
	// ===
	// === Collections
	// ===
	// ================================================================================
	typedef std::vector<Gene *> GeneVector;
	typedef std::map<std::string, Gene *> GeneMap;
	typedef std::list<Gene *> GeneList;
	typedef std::map<const GeneType *, GeneVector> GeneTypeMap;

} // namespace genome
