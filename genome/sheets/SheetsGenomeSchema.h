#pragma once

#include "GenomeSchema.h"
#include "proplib.h"
#include "SheetsCrossover.h"
#include "SheetsModel.h"

namespace genome
{
	class SheetsGenomeSchema : public GenomeSchema
	{
	public:
		static struct Configuration
		{
			struct CrossoverProbability
			{
				float sheet;
				float receptiveField;
				float neuronAttr;
				float gene;
			} crossoverProbability;
			enum NeuronAttrEncoding
			{
				SheetNeuronAttr,
				PerNeuronAttr
			} neuronAttrEncoding;
			enum SynapseAttrEncoding
			{
				FieldSynapseAttr
			} synapseAttrEncoding;
			enum ReceptiveFieldEncoding
			{
				Permutations,
				ExplicitVector
			} receptiveFieldEncoding;
			bool enableReceptiveFieldCurrentRegion;
			bool enableReceptiveFieldOtherRegion;
			int minExplicitVectorSize;
			int maxExplicitVectorSize;
		} config;

		static void processWorldfile( proplib::Document &doc );

		SheetsGenomeSchema();
		virtual ~SheetsGenomeSchema();

		virtual Genome *createGenome( GenomeLayout *layout );

		SheetsCrossover *getCrossover();

		// ------------------
		// --- Definition ---
		// ------------------
	public:
		virtual void define();
		virtual void complete( int offset = 0 );

	private:
		typedef sheets::Vector2i IntMinMax;
		typedef sheets::Vector2f FloatMinMax;

		enum SynapseType
		{
			EE = 0, EI, IE, II, __NumSynapseTypes
		};
		static sheets::Neuron::Attributes::Type getNeuronType( SynapseType synapseType,
															   sheets::Sheet::ReceptiveFieldNeuronRole role );

		ContainerGene *defineSheet( const char *name,
									int id,
									sheets::Sheet::Type sheetType,
									IntMinMax orientation,
									FloatMinMax slot,
									FloatMinMax centerA,
									FloatMinMax centerB,
									FloatMinMax sizeA,
									FloatMinMax sizeB,
									IntMinMax neuronCountA,
									IntMinMax neuronCountB,
									bool scaleToNeuronCount = false );

		void defineNeuronAttrs( ContainerGene *container );

		void defineReceptiveFields( ContainerGene *fromSheets,
									ContainerGene *toSheets );
		void defineReceptiveFieldVector( ContainerGene *sheets,
										 sheets::Sheet::ReceptiveFieldRole role,
										 int ninputSheets,
										 int nsheets );
		void defineReceptiveField( ContainerGene *currentSheet,
								   IntMinMax otherSheetId,
								   sheets::Sheet::ReceptiveFieldRole role,
								   IntMinMax synapseType );
		void defineSynapseAttrs( ContainerGene *container );

		SheetsCrossover _crossover;

		// --------------------
		// --- Create Model ---
		// --------------------
	public:
		sheets::SheetsModel *createSheetsModel( class SheetsGenome *genome ); 

	private:
		void createSheets( sheets::SheetsModel *model, SheetsGenome *g, Gene *sheetsGene, int numSheets = -1 );
		void createSheet( sheets::SheetsModel *model, SheetsGenome *g, ContainerGene *sheetGene );
		sheets::Neuron::Attributes decodeNeuronAttrs( SheetsGenome *g,
													  ContainerGene *attrsGene );

		void createReceptiveFields( sheets::SheetsModel *model, SheetsGenome *g, Gene *sheetsGene );
		void createReceptiveField( sheets::SheetsModel *model,
								   sheets::Sheet *sheet,
								   SheetsGenome *g,
								   ContainerGene *fieldGene );
		sheets::Synapse::Attributes decodeSynapseAttrs( SheetsGenome *g,
														ContainerGene *container );
	};
}
