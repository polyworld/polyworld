// Self
#include "StatusTextMonitorView.h"

#include <glu.h>

// Local
#include "monitor/Monitor.h"
#include "sim/simtypes.h"
#include "utils/misc.h"

using namespace sim;
using namespace std;

#define DEFAULT_WIDTH 300
#define DEFAULT_HEIGHT 500


//===========================================================================
// StatusTextMonitorView
//===========================================================================

//---------------------------------------------------------------------------
// StatusTextMonitorView::StatusTextMonitorView
//---------------------------------------------------------------------------
StatusTextMonitorView::StatusTextMonitorView( StatusTextMonitor *_monitor )
	: MonitorView( _monitor, DEFAULT_WIDTH, DEFAULT_HEIGHT, false )
	, monitor( _monitor )
	, font( "Monospace", 8, QFont::Normal )
	, lineHeight( QFontMetrics(font).lineSpacing() )
	, nlines( 0 )
	
{
    font.setStyleHint(QFont::TypeWriter);
    monitor->update += [=](){this->update();};
}


//---------------------------------------------------------------------------
// StatusTextMonitorView::~StatusTextMonitorView
//---------------------------------------------------------------------------
StatusTextMonitorView::~StatusTextMonitorView()
{
}


//---------------------------------------------------------------------------
// StatusTextMonitorView::paintGL
//---------------------------------------------------------------------------
void StatusTextMonitorView::paintGL()
{
	draw();
}


//---------------------------------------------------------------------------
// StatusTextMonitorView::resizeGL
//---------------------------------------------------------------------------
void StatusTextMonitorView::resizeGL(int width, int height)
{
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0.0, width, 0.0, height);	
	glMatrixMode(GL_MODELVIEW);
}


//---------------------------------------------------------------------------
// StatusTextMonitorView::update
//---------------------------------------------------------------------------
void StatusTextMonitorView::update()
{
	if( !isVisible() )
		return;	

	StatusText &statusText = monitor->getStatusText();

	if( statusText.size() > nlines )
	{
		resize( width(),
				lineHeight * statusText.size()  +  3 );
	}

	draw();
}


//---------------------------------------------------------------------------
// StatusTextMonitorView::draw
//---------------------------------------------------------------------------
void StatusTextMonitorView::draw()
{
	StatusText &statusText = monitor->getStatusText();

	makeCurrent();
		
	// Clear the window to black
	qglClearColor( Qt::black );
	glClear( GL_COLOR_BUFFER_BIT );
	
	// Draw text in white
    glColor4ub( 255, 255, 255, 255 );
	
	int y = lineHeight;

	itfor( StatusText, statusText, iter )
	{
		renderText( 7, y, *iter, font );
		y += lineHeight;
	}
	
	swapBuffers();
}
