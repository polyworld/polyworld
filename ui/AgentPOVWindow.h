//---------------------------------------------------------------------------
//	File:		AgentPOVWindow.h
//
//	Contains:
//
//	Copyright:
//---------------------------------------------------------------------------

#pragma once

// qt
#include <qgl.h>

//===========================================================================
// TAgentPOVWindow
//===========================================================================
class TAgentPOVWindow : public QGLWidget
{
public:
    TAgentPOVWindow();
    virtual ~TAgentPOVWindow();
	
	void BeginStep();
	void EndStep();
	void DrawAgentPOV( class agent *c );
	void RestoreFromPrefs(long x, long y);

	void SaveVisibility();

protected:
	void SaveWindowState();
    void SaveDimensions();

private:
	class QGLPixelBuffer *fPixelBuffer;
	QString windowSettingsName;

};
