//---------------------------------------------------------------------------
//	File:		AgentPOVWindow.h
//
//	Contains:
//
//	Copyright:
//---------------------------------------------------------------------------

#pragma once

// qt
#include <QGLWidget>

//===========================================================================
// TAgentPOVWindow
//===========================================================================
class TAgentPOVWindow : public QGLWidget
{
	Q_OBJECT

public:
    TAgentPOVWindow( class AgentPovRenderer *renderer );
    virtual ~TAgentPOVWindow();
	
	void RestoreFromPrefs(long x, long y);
	
private slots:
	void RenderPovBuffer();

private:
	void SaveWindowState();

	class AgentPovRenderer *fRenderer;
};
