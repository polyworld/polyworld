#if false
#pragma once

#include <string>

#include <QDoubleSpinBox>
#include <QWidget>

#include "Nerve.h"

// forward decl
class QCheckBox;

class agent;
class DebuggerWindow;
namespace genome
{
	class GenomeSchema;
}


//===========================================================================
// BehaviorPanel
//===========================================================================
class BehaviorPanel : public QWidget
{
	Q_OBJECT

 public:
	BehaviorPanel( DebuggerWindow *debuggerWindow,
				   genome::GenomeSchema *genomeSchema,
				   QWidget *parent );

 signals:
	void manualBehaviorChanged(bool);

 private slots:
	void fChkManual_stateChanged( int state );
	void DebuggerWindow_agentChanged( agent *a );

 private:
	void updateActivationsEnabled();

 private:
	agent *fAgent;
	QCheckBox *fChkManual;
	QWidget *fPnlActivations;
	bool fManualBehavior;

};

//===========================================================================
// BehaviorNeuronSpinBox
//===========================================================================
class BehaviorNeuronSpinBox : public QDoubleSpinBox
{
	Q_OBJECT

 public:
	BehaviorNeuronSpinBox( std::string name,
						   BehaviorPanel *panel,
						   QWidget *parent );


 public slots:
	void timeStep();
	void agentChanged( agent *a );
	void manualBehaviorChanged( bool enabled );

 private slots:
	void valueChanged( double val );

 private:
	void updateNerveState();

 private:
	std::string fName;
	agent *fAgent;
	Nerve *fNerve;
	char fNerveSimState[sizeof(Nerve)];
	float fActivation;
	float *fActivationPointer;
	bool fManualBehavior;
};
#endif
