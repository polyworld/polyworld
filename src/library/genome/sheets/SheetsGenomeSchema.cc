#include "SheetsGenomeSchema.h"

#include <stdio.h>
#include <stdlib.h>

#include <iostream>

#include "agent.h"
#include "SheetsBrain.h"
#include "SheetsGenome.h"

using namespace std;
using namespace genome;
using namespace sheets;

static const float InputOutputSheetHeight = 0.1;
static const float InputOutputSheetDefaultWidth = 0.1;


SheetsGenomeSchema::Configuration SheetsGenomeSchema::config;

void SheetsGenomeSchema::processWorldfile( proplib::Document &doc )
{
	proplib::Property &sheets = doc.get( "Sheets" );

	{
		proplib::Property &cross = sheets.get( "CrossoverProbability" );

		SheetsGenomeSchema::config.crossoverProbability.sheet = cross.get( "Sheet" );
		SheetsGenomeSchema::config.crossoverProbability.receptiveField = cross.get( "ReceptiveField" );
		SheetsGenomeSchema::config.crossoverProbability.neuronAttr = cross.get( "NeuronAttr" );
		SheetsGenomeSchema::config.crossoverProbability.gene = cross.get( "Gene" );
	}

	{
		string val = sheets.get( "NeuronAttrEncoding" );
		if( val == "Sheet" )
			SheetsGenomeSchema::config.neuronAttrEncoding = SheetsGenomeSchema::Configuration::SheetNeuronAttr;
		else if( val == "Neuron" )
			SheetsGenomeSchema::config.neuronAttrEncoding = SheetsGenomeSchema::Configuration::PerNeuronAttr;
		else
			assert( false );
	}

	{
		string val = sheets.get( "SynapseAttrEncoding" );
		if( val == "Field" )
			SheetsGenomeSchema::config.synapseAttrEncoding = SheetsGenomeSchema::Configuration::FieldSynapseAttr;
		else
			assert( false );
	}

	{
		string val = sheets.get( "ReceptiveFieldEncoding" );
		if( val == "Permutations" )
			SheetsGenomeSchema::config.receptiveFieldEncoding = SheetsGenomeSchema::Configuration::Permutations;
		else if( val == "ExplicitVector" )
		{
			SheetsGenomeSchema::config.receptiveFieldEncoding = SheetsGenomeSchema::Configuration::ExplicitVector;
			SheetsGenomeSchema::config.minExplicitVectorSize = sheets.get( "MinExplicitVectorSize" );
			SheetsGenomeSchema::config.maxExplicitVectorSize = sheets.get( "MaxExplicitVectorSize" );
		}
		else
			assert( false );
	}

	SheetsGenomeSchema::config.enableReceptiveFieldCurrentRegion = sheets.get( "EnableReceptiveFieldCurrentRegion" );
	SheetsGenomeSchema::config.enableReceptiveFieldOtherRegion = sheets.get( "EnableReceptiveFieldOtherRegion" );
}


SheetsGenomeSchema::SheetsGenomeSchema()
{
}

SheetsGenomeSchema::~SheetsGenomeSchema()
{
}

Genome *SheetsGenomeSchema::createGenome( GenomeLayout *layout )
{
	return new SheetsGenome( this, layout );
}

SheetsCrossover *SheetsGenomeSchema::getCrossover()
{
	return &_crossover;
}

// Creates either mutable or immutable scalar gene, dependent on MIN == MAX
#define __SCALAR( CONT, NAME, MIN, MAX, ROUND )						\
	if( (MIN) != (MAX) )												\
		CONT->add( new MutableScalarGene(NAME, MIN, MAX, __InterpolatedGene::ROUND) ); \
	else																\
		CONT->add( new ImmutableScalarGene(NAME, MIN) )

#define SCALAR(CONT, NAME, MIN, MAX) __SCALAR(CONT, NAME, MIN, MAX, ROUND_INT_FLOOR)
#define SCALARV(CONT, NAME, MINMAX ) SCALAR(CONT, NAME, (MINMAX).a, (MINMAX).b)
#define INDEX(CONT, NAME, MIN, MAX) __SCALAR(CONT, NAME, MIN, MAX, ROUND_INT_BIN)
#define INDEXV(CONT, NAME, MINMAX) INDEX(CONT, NAME, (MINMAX).a, (MINMAX).b )
#define CONST(CONT, NAME, VAL) CONT->add( new ImmutableScalarGene(NAME, VAL) )


template<typename TSheetDef>
static void layoutInputOutputSheets( vector<TSheetDef> &defs )
{
	float centerB = 0.5;
	list<TSheetDef *> rowMembers;

	function<void ()> nextRow =
		[&centerB, &rowMembers]()
		{
			rowMembers.clear();
			if( centerB >= 0.5 )
				// Increase offset, opposite side of center.
				centerB = 1.0 - centerB - InputOutputSheetHeight;
			else
				// Same offset, opposite side of center.
				centerB = 1.0 - centerB;
		};

	for( size_t i = 0; i < defs.size(); i++ )
	{
		TSheetDef *def = &(defs[i]);

		if( !rowMembers.empty() )
		{
			if( (rowMembers.size() % 2) == 0 )
			{
				TSheetDef *end = rowMembers.back();
				if( (end->center.a + (end->maxWidth/2)) + def->maxWidth > 1.0 )
					nextRow();
				else
				{
					def->center.a = (end->center.a + (end->maxWidth/2)) + (def->maxWidth/2);
					def->center.b = centerB;
					rowMembers.push_back( def );
				}
			}
			else
			{
				TSheetDef *end = rowMembers.front();
				if( (end->center.a - (end->maxWidth/2)) - def->maxWidth < 0.0 )
					nextRow();
				else
				{
					def->center.a = (end->center.a - (end->maxWidth/2)) - (def->maxWidth/2);
					def->center.b = centerB;
					rowMembers.push_front( def );
				}
			}
		}

		if( rowMembers.empty() )
		{
			rowMembers.push_back( def );
			def->center.a = 0.5;
			def->center.b = centerB;
		}

		//cout << def->name << " @ " << def->center << endl;
	}
}

void SheetsGenomeSchema::define()
{
	// ---
	// --- Common Genes (e.g. MutationRate, Strength)
	// ---
	GenomeSchema::define();

	_crossover.PhysicalLevel.defineRange( getAll().front(), getAll().back() );

	int sheetId = 0;

	// ---
	// --- Model Properties
	// ---
	SCALAR( this, "SizeX", SheetsBrain::config.minBrainSize.x, SheetsBrain::config.maxBrainSize.x );
	SCALAR( this, "SizeY", SheetsBrain::config.minBrainSize.y, SheetsBrain::config.maxBrainSize.y );
	SCALAR( this, "SizeZ", SheetsBrain::config.minBrainSize.z, SheetsBrain::config.maxBrainSize.z );

	SCALAR( this, "InternalSheetsCount", SheetsBrain::config.minInternalSheetsCount, SheetsBrain::config.maxInternalSheetsCount );

	SCALAR( this, "SynapseProbabilityX", SheetsBrain::config.minSynapseProbabilityX, SheetsBrain::config.maxSynapseProbabilityX );

	SCALAR( this, "LearningRate", SheetsBrain::config.minLearningRate, SheetsBrain::config.maxLearningRate );
	

	// ---
	// --- Input Sheets
	// ---
	ContainerGene *inputSheets = new ContainerGene( "InputSheets" );
	{
		struct InputSheetDef
		{
			const char *name;
			float maxWidth;
			IntMinMax neuronCount;
			bool enabled;

			Vector2f center;
		};
		const IntMinMax VisionNeuronCount( SheetsBrain::config.minVisionNeuronsPerSheet,
										   SheetsBrain::config.maxVisionNeuronsPerSheet );

		// If you are adding a new input, please append to this list in order to maintain stable layout.
		vector<InputSheetDef> defs =
			{
				// Name					MaxWidth						NeuronCount        Enabled
				{ "Red",				1.0,							VisionNeuronCount, true },
				{ "Green",				1.0,							VisionNeuronCount, true },
				{ "Blue",				1.0,							VisionNeuronCount, true },
				{ "Random",				InputOutputSheetDefaultWidth,	1,			       true },
				{ "Energy",				InputOutputSheetDefaultWidth,	1,			       true },
				{ "MateWaitFeedback",	InputOutputSheetDefaultWidth,	1,			       agent::config.enableMateWaitFeedback },
				{ "SpeedFeedback",		InputOutputSheetDefaultWidth,	1,			       agent::config.enableSpeedFeedback },
				{ "Carrying",			InputOutputSheetDefaultWidth,	1,			       agent::config.enableCarry },
				{ "BeingCarried",		InputOutputSheetDefaultWidth,	1,			       agent::config.enableCarry }
			};

		layoutInputOutputSheets( defs );

		for( InputSheetDef def : defs )
		{
			if( def.enabled )
			{
				inputSheets->add(
								 defineSheet( /* name */ def.name,
											  /* id */ sheetId++,
											  /* type*/ Sheet::Input,
											  /* orientation */ (int)PlaneZY,
											  /* slot */ 0.0f,
											  /* centerA */ def.center.a,
											  /* centerB */ def.center.b,
											  /* sizeA */ def.maxWidth,
											  /* sizeB */ InputOutputSheetHeight,
											  /* neuronCountA */ def.neuronCount,
											  /* neuronCountB */ 1,
											  /* scaleToNeuronCountA */ def.neuronCount.a != def.neuronCount.b ) );
			}
		}

		add( inputSheets );
	}

	// ---
	// --- Output Sheets
	// ---
	ContainerGene *outputSheets = new ContainerGene( "OutputSheets" );
	{
		struct OutputSheetDef
		{
			const char *name;
			bool enabled;
			float maxWidth;
			Vector2f center;
		};

		// If you are adding a new output, please append to this list in order to maintain stable layout.
		vector<OutputSheetDef> defs =
			{
				// Name				Enabled
				{ "Eat",			true },
				{ "Mate",			true },
				{ "Fight",			true },
				{ "Speed",			true },
				{ "Yaw",			true },
				{ "YawOppose",		agent::config.yawEncoding == agent::YE_OPPOSE },
				{ "Light",			true },
				{ "Focus",			true },
				{ "VisionPitch",	agent::config.enableVisionPitch },
				{ "VisionYaw",		agent::config.enableVisionYaw },
				{ "Give",			agent::config.enableGive },
				{ "Pickup",			agent::config.enableCarry },
				{ "Drop",			agent::config.enableCarry }
			};

		for( OutputSheetDef &def : defs )
			def.maxWidth = InputOutputSheetDefaultWidth;

		layoutInputOutputSheets( defs );

		for( OutputSheetDef def : defs )
		{
			if( def.enabled )
			{
				outputSheets->add(
								  defineSheet( /* name */ def.name,
											   /* id */ sheetId++,
											   /* type */ Sheet::Output,
											   /* orientation */ (int)PlaneZY,
											   /* slot */ 1.0f,
											   /* centerA */ def.center.a,
											   /* centerB */ def.center.b,
											   /* sizeA */ def.maxWidth,
											   /* sizeB */ InputOutputSheetHeight,
											   /* neuronCountA */ 1,
											   /* neuronCountB */ 1 ) );
			}
		}

		add( outputSheets );
	}

	// ---
	// --- Internal Sheets
	// ---
	ContainerGene *internalSheets = new ContainerGene( "InternalSheets" );
	{
		for( int i = 0; i < SheetsBrain::config.maxInternalSheetsCount; i++ )
		{
			ContainerGene *sheet =
				defineSheet( /* name */ itoa(i),
							 /* id */ sheetId++,
							 /* type */ Sheet::Internal,
							 /* orientation */ IntMinMax( 0, 2 ),
							 /* slot */ FloatMinMax( 0.0f, 1.0f ),
							 /* centerA */ FloatMinMax( 0.0, 1.0 ),
							 /* centerB */ FloatMinMax( 0.0, 1.0 ),
							 /* sizeA */ FloatMinMax( SheetsBrain::config.minInternalSheetSize,
													  SheetsBrain::config.maxInternalSheetSize ),
							 /* sizeB */ FloatMinMax( SheetsBrain::config.minInternalSheetSize,
													  SheetsBrain::config.maxInternalSheetSize ),
							 /* neuronCountA */ IntMinMax( SheetsBrain::config.minInternalSheetNeuronCount,
														   SheetsBrain::config.maxInternalSheetNeuronCount ),
							 /* neuronCountB */ IntMinMax( SheetsBrain::config.minInternalSheetNeuronCount,
														   SheetsBrain::config.maxInternalSheetNeuronCount ) );

			internalSheets->add( sheet );
		}

		add( internalSheets );
	}

	// ---
	// --- Receptive Fields
	// ---
	switch( SheetsGenomeSchema::config.receptiveFieldEncoding )
	{
	case SheetsGenomeSchema::Configuration::Permutations:
		{
			defineReceptiveFields( inputSheets, internalSheets );
			defineReceptiveFields( inputSheets, outputSheets );

			defineReceptiveFields( internalSheets, internalSheets );
			defineReceptiveFields( internalSheets, outputSheets );

			defineReceptiveFields( outputSheets, internalSheets );
			defineReceptiveFields( outputSheets, outputSheets );
		}
		break;
	case SheetsGenomeSchema::Configuration::ExplicitVector:
		{
			int ninputSheets = inputSheets->getAll().size();
			int nsheets = ninputSheets + outputSheets->getAll().size() + internalSheets->getAll().size();

			defineReceptiveFieldVector( inputSheets, Sheet::Target, ninputSheets, nsheets );

			defineReceptiveFieldVector( outputSheets, Sheet::Source, ninputSheets, nsheets );
			defineReceptiveFieldVector( outputSheets, Sheet::Target, ninputSheets, nsheets );

			defineReceptiveFieldVector( internalSheets, Sheet::Source, ninputSheets, nsheets );
			defineReceptiveFieldVector( internalSheets, Sheet::Target, ninputSheets, nsheets );
		}
		break;
	default:
		assert( false );
	}
}

void SheetsGenomeSchema::complete( int offset )
{
	GenomeSchema::complete();

	_crossover.GeneLevel.defineRange( NULL, NULL );

	_crossover.SheetLevel.probability = SheetsGenomeSchema::config.crossoverProbability.sheet;
	_crossover.ReceptiveFieldLevel.probability = SheetsGenomeSchema::config.crossoverProbability.receptiveField;
	_crossover.NeuronAttrLevel.probability = SheetsGenomeSchema::config.crossoverProbability.neuronAttr;
	_crossover.GeneLevel.probability = SheetsGenomeSchema::config.crossoverProbability.gene;

	_crossover.complete( this );

#if false
	{
		SheetsCrossover::Segment segment;

		cout << "--> crossover" << endl;
		while( _crossover.nextSegment(segment) )
		{
			cout << "[" << segment.start << "," << segment.end << "]";
			if( segment.level ) cout << " " << segment.level->name;
			cout << endl;
		}
		exit(0);
	}
#endif
}

Neuron::Attributes::Type SheetsGenomeSchema::getNeuronType( SynapseType synapseType,
															Sheet::ReceptiveFieldNeuronRole role )
{
	switch( role )
	{
	case Sheet::From:
		switch( synapseType )
		{
		case EE:
		case EI:
			return Neuron::Attributes::E;
		case IE:
		case II:
			return Neuron::Attributes::I;
		default:
			assert( false );
			break;
		}
		break;
	case Sheet::To:
		switch( synapseType )
		{
		case EE:
		case IE:
			return Neuron::Attributes::E;
		case EI:
		case II:
			return Neuron::Attributes::I;
		default:
			assert( false );
			break;
		}
		break;
	default:
		assert( false );
	}
}

ContainerGene *SheetsGenomeSchema::defineSheet( const char *name,
												int id,
												Sheet::Type sheetType,
												IntMinMax orientation,
												FloatMinMax slot,
												FloatMinMax centerA,
												FloatMinMax centerB,
												FloatMinMax sizeA,
												FloatMinMax sizeB,
												IntMinMax neuronCountA,
												IntMinMax neuronCountB,
												bool scaleToNeuronCountA )
{
	ContainerGene *sheet = new ContainerGene( name );

	CONST( sheet, "Id", id );
	CONST( sheet, "Type", sheetType );
	INDEXV( sheet, "Orientation", orientation );
	SCALARV( sheet, "Slot", slot );
	SCALARV( sheet, "CenterA", centerA );
	SCALARV( sheet, "CenterB", centerB );
	SCALARV( sheet, "SizeA", sizeA );
	SCALARV( sheet, "SizeB", sizeB );
	SCALARV( sheet, "NeuronCountA", neuronCountA );
	SCALARV( sheet, "NeuronCountB", neuronCountB );
	CONST( sheet, "ScaleToNeuronCountA", (int)scaleToNeuronCountA );

	if( sheetType != Sheet::Input )
	{
		ContainerGene *attrs = new ContainerGene( "NeuronAttrs" );

		if( SheetsGenomeSchema::config.neuronAttrEncoding == Configuration::SheetNeuronAttr )
		{
			defineNeuronAttrs( attrs );
		}
		else if( SheetsGenomeSchema::config.neuronAttrEncoding == Configuration::PerNeuronAttr )
		{
			for( int a = 0; a < neuronCountA.b; a++ )
			{
				char aname[8];
				sprintf( aname, "a%d", a );
				ContainerGene *agene = new ContainerGene( aname );

				for( int b = 0; b < neuronCountB.b; b++ )
				{
					char bname[8];
					sprintf( bname, "b%d", b );
					ContainerGene *bgene = new ContainerGene( bname );

					defineNeuronAttrs( bgene );

					agene->add( bgene );
				}

				attrs->add( agene );
			}
		}

		sheet->add( attrs );

		_crossover.NeuronAttrLevel.defineBoundaryPoints( attrs );
	}

	_crossover.SheetLevel.defineBoundaryPoints( sheet );

	return sheet;
}

void SheetsGenomeSchema::defineNeuronAttrs( ContainerGene *attrs )
{
	SCALAR( attrs, "Bias", -Brain::config.maxbias, Brain::config.maxbias );

	switch( Brain::config.neuronModel )
	{
	case Brain::Configuration::FIRING_RATE:
		// no-op
		break;
	case Brain::Configuration::TAU:
		SCALAR( attrs, "Tau", Brain::config.Tau.minVal, Brain::config.Tau.maxVal );
		break;
	case Brain::Configuration::SPIKING:
		if( Brain::config.Spiking.enableGenes )
		{
			SCALAR( attrs, "SpikingParameterA", Brain::config.Spiking.aMinVal, Brain::config.Spiking.aMaxVal );
			SCALAR( attrs, "SpikingParameterB", Brain::config.Spiking.bMinVal, Brain::config.Spiking.bMaxVal );
			SCALAR( attrs, "SpikingParameterC", Brain::config.Spiking.cMinVal, Brain::config.Spiking.cMaxVal );
			SCALAR( attrs, "SpikingParameterD", Brain::config.Spiking.dMinVal, Brain::config.Spiking.dMaxVal );
		}
		break;
	default:
		assert( false );
	}
}

void SheetsGenomeSchema::defineReceptiveFields( ContainerGene *fromSheets,
												ContainerGene *toSheets )
{
	citfor( GeneVector, fromSheets->getAll(), it_from )
	{
		ContainerGene *from = GeneType::to_Container( *it_from );

		citfor( GeneVector, toSheets->getAll(), it_to )
		{
			ContainerGene *to = GeneType::to_Container( *it_to );
			{
				IntMinMax toId = (int)to->getConst("Id");
				defineReceptiveField( from, toId, Sheet::Target, EE );
				defineReceptiveField( from, toId, Sheet::Target, EI );
				defineReceptiveField( from, toId, Sheet::Target, IE );
				defineReceptiveField( from, toId, Sheet::Target, II );
			}

			{
				IntMinMax fromId = (int)from->getConst("Id");
				defineReceptiveField( to, fromId, Sheet::Source, EE );
				defineReceptiveField( to, fromId, Sheet::Source, EI );
				defineReceptiveField( to, fromId, Sheet::Source, IE );
				defineReceptiveField( to, fromId, Sheet::Source, II );
			}
		}
	}
}

void SheetsGenomeSchema::defineReceptiveFieldVector( ContainerGene *sheets,
													 Sheet::ReceptiveFieldRole role,
													 int ninputSheets,
													 int nsheets )
{
	IntMinMax otherSheetId;
	const char *vectorSizeName;
	switch( role )
	{
	case Sheet::Source:
		otherSheetId.set( 0, nsheets - 1 );
		vectorSizeName = "SourceReceptiveFieldsCount";
		break;
	case Sheet::Target:
		otherSheetId.set( ninputSheets, nsheets - 1 );
		vectorSizeName = "TargetReceptiveFieldsCount";
		break;
	default:
		assert( false );
	}

	IntMinMax synapseType( 0, __NumSynapseTypes - 1 );

	for( Gene *sheetGene_ : sheets->getAll() )
	{
		ContainerGene *sheetGene = GeneType::to_Container( sheetGene_ );
		SCALAR( sheetGene,
				vectorSizeName,
				SheetsGenomeSchema::config.minExplicitVectorSize,
				SheetsGenomeSchema::config.maxExplicitVectorSize );

		for( int i = 0; i < SheetsGenomeSchema::config.maxExplicitVectorSize; i++ )
		{
			defineReceptiveField( sheetGene,
								  otherSheetId,
								  role,
								  synapseType );
		}
		
	}
}

void SheetsGenomeSchema::defineReceptiveField( ContainerGene *currentSheet,
											   IntMinMax otherSheetId,
											   Sheet::ReceptiveFieldRole role,
											   IntMinMax synapseType )
{
	const char *fieldArrayName;
	switch( role )
	{
	case Sheet::Target:
		fieldArrayName = "TargetReceptiveFields";
		break;
	case Sheet::Source:
		fieldArrayName = "SourceReceptiveFields";
		break;
	}
	ContainerGene *fieldArray = GeneType::to_Container( currentSheet->gene(fieldArrayName) );
	if( !fieldArray )
	{
		fieldArray = new ContainerGene( fieldArrayName );
		currentSheet->add( fieldArray );
	}
	
	ContainerGene *field = new ContainerGene( itoa(fieldArray->getAll().size()) );

	SCALARV( field, "OtherSheetId", otherSheetId );
	CONST( field, "Role", (int)role );
	SCALARV( field, "SynapseType", synapseType );
	if( SheetsGenomeSchema::config.enableReceptiveFieldCurrentRegion )
	{
		SCALAR( field, "CurrentCenterA", 0.0, 1.0 );
		SCALAR( field, "CurrentCenterB", 0.0, 1.0 );
		SCALAR( field, "CurrentSizeA", 0.0, 1.0 );
		SCALAR( field, "CurrentSizeB", 0.0, 1.0 );
	}
	else
	{
		SCALAR( field, "CurrentCenterA", 0.5, 0.5 );
		SCALAR( field, "CurrentCenterB", 0.5, 0.5 );
		SCALAR( field, "CurrentSizeA", 1.0, 1.0 );
		SCALAR( field, "CurrentSizeB", 1.0, 1.0 );
	}
	if( SheetsGenomeSchema::config.enableReceptiveFieldOtherRegion )
	{
		SCALAR( field, "OtherCenterA", 0.0, 1.0 );
		SCALAR( field, "OtherCenterB", 0.0, 1.0 );
		SCALAR( field, "OtherSizeA", 0.0, 1.0 );
		SCALAR( field, "OtherSizeB", 0.0, 1.0 );
	}
	else
	{
		SCALAR( field, "OtherCenterA", 0.5, 0.5 );
		SCALAR( field, "OtherCenterB", 0.5, 0.5 );
		SCALAR( field, "OtherSizeA", 1.0, 1.0 );
		SCALAR( field, "OtherSizeB", 1.0, 1.0 );
	}
	SCALAR( field, "OffsetA", -1.0f, 1.0f );
	SCALAR( field, "OffsetB", -1.0f, 1.0f );
	SCALAR( field, "SizeA", 0.0f, 1.0f );
	SCALAR( field, "SizeB", 0.0f, 1.0f );

	if( SheetsGenomeSchema::config.synapseAttrEncoding == SheetsGenomeSchema::Configuration::FieldSynapseAttr )
		defineSynapseAttrs( field );

	fieldArray->add( field );

	_crossover.ReceptiveFieldLevel.defineBoundaryPoints( field );
}

void SheetsGenomeSchema::defineSynapseAttrs( ContainerGene *container )
{
	SCALAR( container, "Weight", 0.0f, Brain::config.initMaxWeight );
	SCALAR( container, "LearningRate", 0.0f, 1.0f );
}

SheetsModel *SheetsGenomeSchema::createSheetsModel( SheetsGenome *genome )
{
	Vector3f size( genome->get("SizeX"),
				   genome->get("SizeY"),
				   genome->get("SizeZ") );

	float synapseProbabilityX = genome->get( "SynapseProbabilityX" );

	SheetsModel *model = new SheetsModel( size, synapseProbabilityX );
	
	createSheets( model, genome, genome->gene("InputSheets") );
	createSheets( model, genome, genome->gene("OutputSheets") );
	createSheets( model, genome, genome->gene("InternalSheets"), genome->get("InternalSheetsCount") );

	createReceptiveFields( model, genome, genome->gene("InputSheets") );
	createReceptiveFields( model, genome, genome->gene("OutputSheets") );
	createReceptiveFields( model, genome, genome->gene("InternalSheets") );

	model->cull();

	return model;
}

void SheetsGenomeSchema::createSheets( SheetsModel *model, SheetsGenome *g, Gene *sheetsGene, int numSheets )
{
	ContainerGene *container = GeneType::to_Container( sheetsGene );
	if( numSheets == -1 )
		numSheets = (int)container->getAll().size();

	for( int i = 0; i < numSheets; i++ )
	{
		ContainerGene *sheetGene = GeneType::to_Container( container->getAll()[i] );

		createSheet( model, g, sheetGene );
	}
}

void SheetsGenomeSchema::createSheet( SheetsModel *model, SheetsGenome *g, ContainerGene *sheetGene )
{
	// ---
	// --- Fetch sheet attributes
	// ---
	string name = sheetGene->name;
	int id = g->get( sheetGene->gene("Id") );
	Sheet::Type sheetType = (Sheet::Type)(int)g->get( sheetGene->gene("Type") );
	int orientation = g->get( sheetGene->gene("Orientation") );
	float slot = g->get( sheetGene->gene("Slot") );
	Vector2f center( g->get( sheetGene->gene("CenterA") ),
					 g->get( sheetGene->gene("CenterB") ) );
	Vector2f size( g->get( sheetGene->gene("SizeA") ),
					 g->get( sheetGene->gene("SizeB") ) );
	Vector2i neuronCount( g->get( sheetGene->gene("NeuronCountA") ),
						  g->get( sheetGene->gene("NeuronCountB") ) );

	bool scaleToNeuronCountA = (int)sheetGene->getConst( "ScaleToNeuronCountA" ) != 0;
	if( scaleToNeuronCountA )
	{
		int maxNeurons = GeneType::to_MutableScalar( sheetGene->gene("NeuronCountA") )->getMax();
		float fraction = float(neuronCount.a) / maxNeurons;
		size.a *= fraction;
	}

	// ---
	// --- Create neuron attribute configuration function
	// ---
	function<void (Neuron *)> neuronCreated;

	function<void (Neuron *)> setIE =
		[sheetType] ( Neuron *neuron )
		{
			switch( sheetType )
			{
			case Sheet::Input:
			case Sheet::Output:
				neuron->attrs.type = Neuron::Attributes::EI;
				break;
			case Sheet::Internal:
				neuron->attrs.type = (neuron->sheetIndex.a % 2) == (neuron->sheetIndex.b % 2)
					? Neuron::Attributes::E
					: Neuron::Attributes::I;
				break;
			default:
				assert( false );
			}
		};

	if( sheetType == Sheet::Input )
	{
		neuronCreated = [setIE]( Neuron *neuron )
			{
				memset( &neuron->attrs, 0, sizeof(neuron->attrs) );
				setIE( neuron );
			};
	}
	else if( SheetsGenomeSchema::config.neuronAttrEncoding == Configuration::SheetNeuronAttr )
	{
		ContainerGene *attrsGene = GeneType::to_Container( sheetGene->gene( "NeuronAttrs" ) );
		Neuron::Attributes attrs = decodeNeuronAttrs( g, attrsGene );

		neuronCreated = [setIE, attrs]( Neuron *neuron )
		{
			neuron->attrs = attrs;
			setIE( neuron );
		};
	}
	else if( SheetsGenomeSchema::config.neuronAttrEncoding == Configuration::PerNeuronAttr )
	{
		ContainerGene *attrsGene = GeneType::to_Container( sheetGene->gene( "NeuronAttrs" ) );

		neuronCreated = [this, g, setIE, attrsGene]( Neuron *neuron )
		{
			ContainerGene *agene = GeneType::to_Container( attrsGene->getAll()[neuron->sheetIndex.a] );
			ContainerGene *bgene = GeneType::to_Container( agene->getAll()[neuron->sheetIndex.b] );
			neuron->attrs = decodeNeuronAttrs( g, bgene );

			setIE( neuron );
		};
	}
	else
	{
		assert( false );
	}

	// ---
	// --- Create Sheet
	// ---
	model->createSheet( name,
						id,
						(Sheet::Type)sheetType,
						(Orientation)orientation,
						slot,
						center,
						size,
						neuronCount,
						neuronCreated );
}

Neuron::Attributes SheetsGenomeSchema::decodeNeuronAttrs( SheetsGenome *g,
														  ContainerGene *attrsGene )
{
	Neuron::Attributes attrs;

	switch( Brain::config.neuronModel )
	{
	case Brain::Configuration::FIRING_RATE:
		attrs.neuronModel.firingRate.bias = g->get( attrsGene->gene("Bias") );
		break;
	case Brain::Configuration::TAU:
		attrs.neuronModel.tau.bias = g->get( attrsGene->gene("Bias") );
		attrs.neuronModel.tau.tau = g->get( attrsGene->gene("Tau") );
		break;
	case Brain::Configuration::SPIKING:
		if( Brain::config.Spiking.enableGenes )
		{
			attrs.neuronModel.spiking.bias = g->get( attrsGene->gene("Bias") );
			attrs.neuronModel.spiking.SpikingParameter_a = g->get( attrsGene->gene("SpikingParameterA") );
			attrs.neuronModel.spiking.SpikingParameter_b = g->get( attrsGene->gene("SpikingParameterB") );
			attrs.neuronModel.spiking.SpikingParameter_c = g->get( attrsGene->gene("SpikingParameterC") );
			attrs.neuronModel.spiking.SpikingParameter_d = g->get( attrsGene->gene("SpikingParameterD") );
		}
		break;
	default:
		assert( false );
	}			

	return attrs;
}

void SheetsGenomeSchema::createReceptiveFields( SheetsModel *model, SheetsGenome *g, Gene *sheetsGene )
{
	ContainerGene *container = GeneType::to_Container( sheetsGene );

	citfor( GeneVector, container->getAll(), it )
	{
		ContainerGene *sheetGene = GeneType::to_Container( *it );

		int id = g->get( sheetGene->gene("Id") );
		Sheet *sheet = model->getSheet( id );
		if( sheet == NULL )
			continue;

		vector<const char*> vectorNames = { "SourceReceptiveFields", "TargetReceptiveFields" };

		for( const char *vectorName : vectorNames )
		{
			ContainerGene *fieldsGene = GeneType::to_Container(sheetGene->gene(vectorName));
			if( fieldsGene )
			{
				int nfields;
				switch( SheetsGenomeSchema::config.receptiveFieldEncoding )
				{
				case SheetsGenomeSchema::Configuration::Permutations:
					nfields = fieldsGene->getAll().size();
					break;
				case SheetsGenomeSchema::Configuration::ExplicitVector:
					nfields = g->get( sheetGene->gene(string(vectorName) + "Count") );
					break;
				default:
					assert( false );
				}

				for( int i = 0; i < nfields; i++ )
				{
					ContainerGene *fieldGene = GeneType::to_Container( fieldsGene->getAll()[i] );
		
					createReceptiveField( model, sheet, g, fieldGene );
				}
			}
		}
	}
}

void SheetsGenomeSchema::createReceptiveField( SheetsModel *model,
											   Sheet *sheet,
											   SheetsGenome *g,
											   ContainerGene *fieldGene )
{
	// ---
	// --- Fetch field attributes
	// ---
	int otherSheetId = g->get( fieldGene->gene("OtherSheetId") );
	Sheet *otherSheet = model->getSheet( otherSheetId );
	if( otherSheet == NULL )
		return;

	Sheet::ReceptiveFieldRole role = (Sheet::ReceptiveFieldRole)(int)g->get( fieldGene->gene("Role") );
	SynapseType synapseType = (SynapseType)(int)g->get( fieldGene->gene("SynapseType") );
	Vector2f currentCenter( g->get( fieldGene->gene("CurrentCenterA") ),
							g->get( fieldGene->gene("CurrentCenterB") ) );
	Vector2f currentSize( g->get( fieldGene->gene("CurrentSizeA") ),
						  g->get( fieldGene->gene("CurrentSizeB") ) );
	Vector2f otherCenter( g->get( fieldGene->gene("OtherCenterA") ),
							g->get( fieldGene->gene("OtherCenterB") ) );
	Vector2f otherSize( g->get( fieldGene->gene("OtherSizeA") ),
						  g->get( fieldGene->gene("OtherSizeB") ) );
	Vector2f offset( g->get( fieldGene->gene("OffsetA") ),
					 g->get( fieldGene->gene("OffsetB") ) );
	Vector2f size( g->get( fieldGene->gene("SizeA") ),
				   g->get( fieldGene->gene("SizeB") ) );

	// ---
	// --- Create neuron predicate function
	// ---
	function<bool (Neuron *, Sheet::ReceptiveFieldNeuronRole)> neuronPredicate =
		[this, synapseType]( Neuron *neuron, Sheet::ReceptiveFieldNeuronRole role )
		{
			if( neuron->attrs.type == Neuron::Attributes::EI )
				return true;

			return getNeuronType( synapseType, role ) == neuron->attrs.type;
		};

	// ---
	// --- Create synapse attribute configuration function
	// ---
	function<void (Synapse *)> synapseCreated;
	Synapse::Attributes attrs;
	if( SheetsGenomeSchema::config.synapseAttrEncoding == SheetsGenomeSchema::Configuration::FieldSynapseAttr )
	{
		WARN_ONCE( "IMPLEMENT INDEX-BASED WEIGHT" ); 
		attrs = decodeSynapseAttrs( g, fieldGene );

		if( getNeuronType(synapseType, Sheet::From) == Neuron::Attributes::I )
		{
			attrs.weight *= -1;
			attrs.lrate *= -1;
		}

		synapseCreated =
			[attrs] ( Synapse *synapse )
			{
				synapse->attrs = attrs;
			};
	}
	else
	{
		assert( false );
	}

	sheet->addReceptiveField( role,
							  currentCenter,
							  currentSize,
							  otherCenter,
							  otherSize,
							  offset,
							  size,
							  otherSheet,
							  neuronPredicate,
							  synapseCreated );
}

Synapse::Attributes SheetsGenomeSchema::decodeSynapseAttrs( SheetsGenome *g,
															ContainerGene *container )
{
	Synapse::Attributes attrs;

	attrs.weight = g->get( container->gene("Weight") );
	attrs.lrate = g->get( "LearningRate" );
	attrs.lrate *= (float)g->get( container->gene("LearningRate") );

	return attrs;
}
