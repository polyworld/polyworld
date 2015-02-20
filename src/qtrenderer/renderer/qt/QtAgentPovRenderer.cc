#include "QtAgentPovRenderer.h"

#include <assert.h>

#include <QGLPixelBuffer>
#include <QGLWidget>

#include "agent/agent.h"
#include "agent/Retina.h"

#define CELL_PAD 2

//---------------------------------------------------------------------------
// AgentPovRenderer::AgentPovRenderer
//---------------------------------------------------------------------------
AgentPovRenderer *AgentPovRenderer::create( int maxAgents,
                                            int retinaWidth,
                                            int retinaHeight )
{
    return new QtAgentPovRenderer( maxAgents, retinaWidth, retinaHeight );
}

//---------------------------------------------------------------------------
// QtAgentPovRenderer::QtAgentPovRenderer
//---------------------------------------------------------------------------
QtAgentPovRenderer::QtAgentPovRenderer( int maxAgents,
                                        int retinaWidth,
                                        int retinaHeight )
: fPixelBuffer( NULL )
{
	// If we decide we want the width W (in cells) to be a multiple of N (call it I)
	// and we want the aspect ratio of W to height H (in cells) to be at least A,
	// and we call maxAgents M, then (do the math or trust me):
	// I = floor( (sqrt(M*A) + (N-1)) / N )
	// W = I * N
	// H = floor( (M + W - 1) / W )
	int n = 10;
	int a = 3;
	int i = (int) (sqrt( (float) (maxAgents * a) ) + n - 1) / n;
	int ncols = i * n;   // width in cells
	int nrows = (maxAgents + ncols - 1) / ncols;
	fBufferWidth = ncols * (retinaWidth + CELL_PAD);
	fBufferHeight = nrows * (retinaHeight + CELL_PAD);

	slotHandle = AgentAttachedData::createSlot();

	fViewports = new Viewport[ maxAgents ];
	for( int i = 0; i < maxAgents; i++ )
	{
		Viewport *viewport = fViewports + i;

		viewport->index = i;
		
		short irow = short(i / ncols);            
		short icol = short(i) - (ncols * irow);

		viewport->width = retinaWidth;
		viewport->height = retinaHeight;

		viewport->x = icol * (viewport->width + CELL_PAD)  +  CELL_PAD;
		short ytop = fBufferHeight  -  (irow) * (viewport->height + CELL_PAD) - CELL_PAD - 1;
		viewport->y = ytop  -  viewport->height  +  1;

		fFreeViewports.insert( make_pair(viewport->index, viewport) );
	}
}

//---------------------------------------------------------------------------
// QtAgentPovRenderer::~QtAgentPovRenderer
//---------------------------------------------------------------------------
QtAgentPovRenderer::~QtAgentPovRenderer()
{
	delete fPixelBuffer;
	delete [] fViewports;
}

//---------------------------------------------------------------------------
// QtAgentPovRenderer::add
//---------------------------------------------------------------------------
void QtAgentPovRenderer::add( agent *a )
{
	assert( AgentAttachedData::get( a, slotHandle ) == NULL );

	Viewport *viewport = fFreeViewports.begin()->second;
	fFreeViewports.erase( fFreeViewports.begin() );

	AgentAttachedData::set( a, slotHandle, viewport );
}

//---------------------------------------------------------------------------
// QtAgentPovRenderer::remove
//---------------------------------------------------------------------------
void QtAgentPovRenderer::remove( agent *a )
{
	Viewport *viewport = (Viewport *)AgentAttachedData::get( a, slotHandle );

	if( viewport )
	{
		AgentAttachedData::set( a, slotHandle, NULL );
		fFreeViewports.insert( make_pair(viewport->index, viewport) );
	}
}

//---------------------------------------------------------------------------
// QtAgentPovRenderer::beginStep
//---------------------------------------------------------------------------
void QtAgentPovRenderer::beginStep()
{
	if( fPixelBuffer == NULL )
	{
		fPixelBuffer = new QGLPixelBuffer( fBufferWidth, fBufferHeight );
		fPixelBuffer->makeCurrent();

		glEnable( GL_DEPTH_TEST );
		glEnable( GL_NORMALIZE );

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
	}
	else
	{
		fPixelBuffer->makeCurrent();
	}

	glClearColor( 0, 0, 0, 1 );
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

//---------------------------------------------------------------------------
// QtAgentPovRenderer::render
//---------------------------------------------------------------------------
void QtAgentPovRenderer::render( agent *a )
{
	Viewport *viewport = (Viewport *)AgentAttachedData::get( a, slotHandle );

	// Initialize projection
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	// Limit our drawing to this agent's small window
	glViewport( viewport->x, viewport->y, viewport->width, viewport->height );

	// Do the actual drawing
	glPushMatrix();
		a->GetScene().Draw();
	glPopMatrix();

	// Copy pixel data into retina
	a->GetRetina()->updateBuffer( viewport->x, viewport->y, viewport->width, viewport->height );
}

//---------------------------------------------------------------------------
// QtAgentPovRenderer::endStep
//---------------------------------------------------------------------------
void QtAgentPovRenderer::endStep()
{
	fPixelBuffer->doneCurrent();

	renderComplete();
}

//---------------------------------------------------------------------------
// QtAgentPovRenderer::copyTo
//---------------------------------------------------------------------------
void QtAgentPovRenderer::copyTo( QGLWidget *dst)
{
	if( fPixelBuffer )
	{
		QImage image = fPixelBuffer->toImage();
		QPainter painter( dst );
		painter.drawImage( QRect(0,0,dst->width(),dst->height()), image );
	}
}

//---------------------------------------------------------------------------
// QtAgentPovRenderer::getBufferWidth
//---------------------------------------------------------------------------
int QtAgentPovRenderer::getBufferWidth()
{
	return fBufferWidth;
}

//---------------------------------------------------------------------------
// QtAgentPovRenderer::getBufferHeight
//---------------------------------------------------------------------------
int QtAgentPovRenderer::getBufferHeight()
{
	return fBufferHeight;
}
