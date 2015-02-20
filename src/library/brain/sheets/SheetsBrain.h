#pragma once

#include "Brain.h"
#include "SheetsModel.h"

namespace genome { class SheetsGenome; }

class SheetsBrain : public Brain
{
 public:
	static struct Configuration
	{
		sheets::Vector3f minBrainSize;
		sheets::Vector3f maxBrainSize;
		float minSynapseProbabilityX;
		float maxSynapseProbabilityX;
		float minLearningRate;
		float maxLearningRate;
		int minVisionNeuronsPerSheet;
		int maxVisionNeuronsPerSheet;
		int minInternalSheetsCount;
		int maxInternalSheetsCount;
		float minInternalSheetSize;
		float maxInternalSheetSize;
		int minInternalSheetNeuronCount;
		int maxInternalSheetNeuronCount;
	} config;

	static void processWorldfile( proplib::Document &doc );
	static void init();

	SheetsBrain( NervousSystem *cns, genome::SheetsGenome *genome, sheets::SheetsModel *model );
	virtual ~SheetsBrain();

	int getNumInternalSheets();
	int getNumInternalNeurons();
	int getNumSynapses( sheets::Sheet::Type from, sheets::Sheet::Type to );

 private:
    void grow( genome::SheetsGenome *genome, sheets::SheetsModel *model );

	int _numInternalSheets;
	int _numInternalNeurons;
	int _numSynapses[sheets::Sheet::__NTYPES][sheets::Sheet::__NTYPES];
};

inline int SheetsBrain::getNumInternalSheets() { return _numInternalSheets; }
inline int SheetsBrain::getNumInternalNeurons() { return _numInternalNeurons; }
inline int SheetsBrain::getNumSynapses( sheets::Sheet::Type from, sheets::Sheet::Type to ) { return _numSynapses[from][to]; }
