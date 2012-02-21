// Self
#include "PovMonitorView.h"

// Local
#include "AgentPovRenderer.h"
#include "Monitor.h"

//===========================================================================
// PovMonitorView
//===========================================================================

//---------------------------------------------------------------------------
// PovMonitorView::PovMonitorView
//---------------------------------------------------------------------------
PovMonitorView::PovMonitorView( PovMonitor *monitor )
	: MonitorView( monitor,
				   monitor->getRenderer()->getBufferWidth(),
				   monitor->getRenderer()->getBufferHeight(),
				   false )
	, renderer( monitor->getRenderer() )
{
	connect( renderer, SIGNAL(renderComplete()),
			 this, SLOT(draw()) );
}


//---------------------------------------------------------------------------
// PovMonitorView::~PovMonitorView
//---------------------------------------------------------------------------
PovMonitorView::~PovMonitorView()
{
}

//---------------------------------------------------------------------------
// PovMonitorView::paintGL
//---------------------------------------------------------------------------
void PovMonitorView::paintGL()
{
	draw();
}

//---------------------------------------------------------------------------
// PovMonitorView::draw
//---------------------------------------------------------------------------
void PovMonitorView::draw()
{
	if( isVisible() )
	{
		renderer->copyTo( this );
	}
}
