#pragma once

#include "Gene.h"

namespace genome
{
	// forward decls
	class GroupsSynapseType;

	// ================================================================================
	// ===
	// === CLASS GroupsGeneType
	// ===
	// ================================================================================
	class GroupsGeneType
	{
	public:
		static const GeneType *NEURGROUP;
		static const GeneType *NEURGROUP_ATTR;
		static const GeneType *SYNAPSE_ATTR;

#define CAST_TO(TYPE)											\
		static class TYPE##Gene *to_##TYPE(class Gene *gene);
		CAST_TO(NeurGroup);
		CAST_TO(NeurGroupAttr);
		CAST_TO(SynapseAttr);
#undef CAST_TO
	};

	// ================================================================================
	// ===
	// === CLASS NeurGroupGene
	// ===
	// === Common base class for neural group genes.
	// ===
	// ================================================================================
	class NeurGroupGene : public NonVectorGene
	{
	public:
		NeurGroupGene( NeurGroupType group_type );				   
		virtual ~NeurGroupGene() {}

		virtual Scalar get( Genome *genome ) = 0;

		bool isMember( NeurGroupType group_type );
		NeurGroupType getGroupType();

		virtual int getMaxGroupCount() = 0;
		virtual int getMaxNeuronCount() = 0;

		virtual std::string getTitle( int group ) = 0;

		class GroupsGenomeSchema *schema;

	protected:
		friend class GroupsGenomeSchema;

		int first_group;

	private:
		NeurGroupType group_type;
	};


	// ================================================================================
	// ===
	// === CLASS MutableNeurGroupGene
	// ===
	// === For public use; represents Internal groups as well as Input groups with
	// === variable neuron counts
	// ===
	// ================================================================================
	class MutableNeurGroupGene : public NeurGroupGene, public __InterpolatedGene
	{
	public:
		MutableNeurGroupGene( const char *name,
							  NeurGroupType group_type,
							  Scalar min_,
							  Scalar max_ );
		virtual ~MutableNeurGroupGene() {}

		virtual Scalar get( Genome *genome );

		virtual int getMaxGroupCount();
		virtual int getMaxNeuronCount();

		virtual std::string getTitle( int group );
	};


	// ================================================================================
	// ===
	// === CLASS ImmutableNeurGroupGene
	// ===
	// === For public use; represents Input groups with fixed neuron counts as well as
	// === Output groups.
	// ===
	// ================================================================================
	class ImmutableNeurGroupGene : public NeurGroupGene, private __ConstantGene
	{
	public:
		ImmutableNeurGroupGene( const char *name,
								NeurGroupType group_type );
		virtual ~ImmutableNeurGroupGene() {}

		virtual Scalar get( Genome *genome );

		virtual int getMaxGroupCount();
		virtual int getMaxNeuronCount();

		virtual std::string getTitle( int group );
	};


	// ================================================================================
	// ===
	// === CLASS NeurGroupAttrGene
	// ===
	// === For public use; provides a per-group attribute vector
	// ===
	// ================================================================================
	class NeurGroupAttrGene : virtual public Gene, public __InterpolatedGene
	{
	public:
		NeurGroupAttrGene( const char *name,
						   NeurGroupType group_type,
						   Scalar min_,
						   Scalar max_ );
		virtual ~NeurGroupAttrGene() {}

		virtual Scalar get( Genome *genome,
							int group );

		const Scalar &getMin();
		const Scalar &getMax();

		void seed( Genome *genome,
				   NeurGroupGene *group,
				   unsigned char rawval );

		virtual void printIndexes( FILE *file, const std::string &prefix, GenomeLayout *layout );
		virtual void printTitles( FILE *file, const std::string &prefix );

		const NeurGroupType group_type;

		class GroupsGenomeSchema *schema;

	protected:
		virtual int getMutableSizeImpl();

		friend class GenomeLayout;

		int getOffset( int group );
	};

	// ================================================================================
	// ===
	// === CLASS SynapseAttrGene
	// ===
	// === For public use; provides a per-synapse attribute matrix
	// ===
	// ================================================================================
	class SynapseAttrGene : virtual public Gene, public __InterpolatedGene
	{
	public:
		SynapseAttrGene( const char *name,
						 bool negateInhibitory,
						 bool lessThanZero,
						 Scalar min_,
						 Scalar max_ );
		virtual ~SynapseAttrGene() {}

		virtual Scalar get( Genome *genome,
							GroupsSynapseType *synapseType,
							int group_from,
							int group_to );

		void seed( Genome *genome,
				   GroupsSynapseType *synapseType,
				   NeurGroupGene *from,
				   NeurGroupGene *to,
				   unsigned char rawval );

		virtual void printIndexes( FILE *file, const std::string &prefix, GenomeLayout *layout );
		virtual void printTitles( FILE *file, const std::string &prefix );

		class GroupsGenomeSchema *schema;

	protected:
		virtual int getMutableSizeImpl();

		friend class GenomeLayout;

		int getOffset( GroupsSynapseType *synapseType,
					   int from,
					   int to );

	private:
		bool negateInhibitory;
		bool lessThanZero;
	};
}
