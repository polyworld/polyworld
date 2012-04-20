#if false
#include "BehaviorPanel.h"

#include <assert.h>

#include <iostream>

#include <QCheckBox>
#include <QGridLayout>
#include <QLabel>

#include "agent.h"
#include "DebuggerWindow.h"
#include "GenomeSchema.h"
#include "misc.h"
#include "NervousSystem.h"

using namespace std;

using namespace genome;

//===========================================================================
// BehaviorPanel
//===========================================================================

//---------------------------------------------------------------------------
// BehaviorPanel::BehaviorPanel
//---------------------------------------------------------------------------
BehaviorPanel::BehaviorPanel( DebuggerWindow *debuggerWindow,
							  GenomeSchema *genomeSchema,
							  QWidget *parent ) : QWidget( NULL )
{
	fAgent = NULL;
	fManualBehavior = false;

	connect( debuggerWindow, SIGNAL(agentChanged(agent*)), this, SLOT(DebuggerWindow_agentChanged(agent*)) );

	QVBoxLayout *layout = new QVBoxLayout(this);
	setLayout( layout );

	fChkManual = new QCheckBox( tr("Manual Behavior Activation"), this );
	layout->addWidget( fChkManual );
	// receive notification of user changing 'manual' checkbox
	connect( fChkManual, SIGNAL(stateChanged(int)), this, SLOT(fChkManual_stateChanged(int)) );

	fPnlActivations = new QWidget( this );
	layout->addWidget( fPnlActivations );
	QGridLayout *activationLayout = new QGridLayout(fPnlActivations);
	fPnlActivations->setLayout(activationLayout);

	int ngroups = 0;

	citfor( GeneVector, genomeSchema->getAll(GroupsGeneType::NEURGROUP), it )
	{
		NeurGroupGene *gene = (*it)->to_NeurGroup();

		if( gene->getGroupType() == NGT_OUTPUT )
		{
			QLabel *label = new QLabel(gene->name.c_str(), this);
			activationLayout->addWidget( label, ngroups, 0);

			BehaviorNeuronSpinBox *spinBox = new BehaviorNeuronSpinBox( gene->name, this, this );
			// get notification of new agent
			connect(debuggerWindow, SIGNAL(agentChanged(agent*)), spinBox, SLOT(agentChanged(agent*)) );
			// get notification of timestep
			connect(debuggerWindow, SIGNAL(timeStep()), spinBox, SLOT(timeStep()) );
			// get notification of manual behavior
			connect(this, SIGNAL(manualBehaviorChanged(bool)), spinBox, SLOT(manualBehaviorChanged(bool)) );

			activationLayout->addWidget( spinBox, ngroups, 1);

			ngroups++;
		}
	}

	updateActivationsEnabled();
}

//---------------------------------------------------------------------------
// BehaviorPanel::fChkManual_stateChanged
//---------------------------------------------------------------------------
void BehaviorPanel::fChkManual_stateChanged( int state )
{
	updateActivationsEnabled();
}

//---------------------------------------------------------------------------
// BehaviorPanel::setAgent
//---------------------------------------------------------------------------
void BehaviorPanel::DebuggerWindow_agentChanged( agent *a )
{
	fAgent = a;

	updateActivationsEnabled();
}

//---------------------------------------------------------------------------
// BehaviorPanel::updateActivationsEnabled
//---------------------------------------------------------------------------
void BehaviorPanel::updateActivationsEnabled()
{
	bool oldValue = fManualBehavior;

	if( fChkManual->isChecked() && (fAgent != NULL) )
	{
		fManualBehavior = true;
	}
	else
	{
		fManualBehavior = false;
	}

	fPnlActivations->setEnabled( fManualBehavior );
	
	if( oldValue != fManualBehavior )
	{
		manualBehaviorChanged( fManualBehavior );
	}
}

//===========================================================================
// BehaviorNeuronSpinBox
//===========================================================================

//---------------------------------------------------------------------------
// BehaviorNeuronSpinBox::BehaviorSpinBox
//---------------------------------------------------------------------------
BehaviorNeuronSpinBox::BehaviorNeuronSpinBox( std::string name,
											  BehaviorPanel *panel,
											  QWidget *parent ) : QDoubleSpinBox( parent )
{
	fName = name;
	fAgent = NULL;
	fNerve = NULL;
	fManualBehavior = false;

	setRange( 0, 1 );

	if( fName == "Yaw" )
	{
		setSingleStep( 0.05 );
	}
	else
	{
		setSingleStep( 0.1 );
	}
	fActivationPointer = &fActivation;

	// get notification of user changing value
	connect( this, SIGNAL(valueChanged(double)), this, SLOT(valueChanged(double)) );
}

//---------------------------------------------------------------------------
// BehaviorNeuronSpinBox::timeStep
//---------------------------------------------------------------------------
void BehaviorNeuronSpinBox::timeStep()
{
	setValue( fNerve->get(0) );
}

//---------------------------------------------------------------------------
// BehaviorNeuronSpinBox::agentChanged
//---------------------------------------------------------------------------
void BehaviorNeuronSpinBox::agentChanged( agent *a )
{
	// need to add code to restore old agent's neuron
	assert( fAgent == NULL );

	fAgent = a;

	fNerve = fAgent->GetNervousSystem()->get( fName );
	assert( fNerve->getNeuronCount() == 1 );

	// Save a copy of the original state.
	memcpy( fNerveSimState, fNerve, sizeof(Nerve) );

	updateNerveState();
}

//---------------------------------------------------------------------------
// BehaviorNeuronSpinBox::manualBehaviorChanged
//---------------------------------------------------------------------------
void BehaviorNeuronSpinBox::manualBehaviorChanged( bool enabled )
{
	fManualBehavior = enabled;
	
	updateNerveState();
}

//---------------------------------------------------------------------------
// BehaviorNeuronSpinBox::updateNerveState
//---------------------------------------------------------------------------
void BehaviorNeuronSpinBox::updateNerveState()
{
	if( fNerve )
	{
		if( fManualBehavior )
		{
			fNerve->config( 1, 0 );
			fNerve->config( &fActivationPointer, &fActivationPointer );		
		}
		else
		{
			memcpy( fNerve, fNerveSimState, sizeof(Nerve) );
		}
	}
}

//---------------------------------------------------------------------------
// BehaviorNeuronSpinBox::valueChanged
//---------------------------------------------------------------------------
void BehaviorNeuronSpinBox::valueChanged( double val )
{
	fActivation = val;
}
#endif
