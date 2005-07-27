#define GLW_DEBUG 0

// System
#include <math.h>

// Qt
#include <QtGui>
#include <QtOpenGL>

// Self
#include "GLWidget.h"

#if GLW_DEBUG
	#define glwPrint( x... ) { printf( "%s: ", __FUNCTION__ ); printf( x ); }
#else
	#define glwPrint( x... )
#endif

GLWidget::GLWidget( QWidget *parent, unsigned long widthParam, unsigned long heightParam, unsigned long movieVersionParam, FILE* movieFileParam )
	: QGLWidget( parent )
{
	width = widthParam;
	height = heightParam;
	movieVersion = movieVersionParam;
	movieFile = movieFileParam;

	glwPrint( "width = %lu, height = %lu\n", width, height );

	QRect rect( 0, 0, width, height );
	setGeometry( rect );

	// Allocate the rle and rgb buffers for movie decoding and playback
	rgbBufSize = width * height * sizeof( *rgbBuf1 );
	rleBufSize = rgbBufSize + 1;	// it better compress!
	rgbBuf1 = (unsigned long*) malloc( rgbBufSize );
//	rgbBuf2 = (unsigned long*) malloc( rgbBufSize );
	rleBuf  = (unsigned long*) malloc( rleBufSize );
}

GLWidget::~GLWidget()
{
}

QSize GLWidget::minimumSizeHint() const
{
	return QSize( 100, 100 );
}

QSize GLWidget::sizeHint() const
{
	return QSize( width, height );
}

void GLWidget::initializeGL()
{
	glwPrint( "called\n" );

	qglClearColor( Qt::black );
//	glPolygonMode( GL_FRONT_AND_BACK, GL_FILL );
	glViewport( 0, 0, width, height );
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	glOrtho( 0, width, 0, height, -1.0, 1.0 ); 
//	glMatrixMode(GL_MODELVIEW);
    glColor4ub( 0, 255, 255, 255 );
	setAutoBufferSwap( false );
}

void GLWidget::Draw()
{
	static bool firstFrame = true;
	static unsigned long frame;
	
	frame++;
//	printf( "%s: frame # %lu\n", __FUNCTION__, frame );
	
	if( frame < 1 )
		return;
	
	makeCurrent();
	
	if( readrle( movieFile, rleBuf, firstFrame ? 0 : movieVersion ) )	// 0 = success, other = failure
		return;
	
	glClear( GL_COLOR_BUFFER_BIT );

	if( (movieVersion <= 1) || firstFrame )
	{
		unrle( rleBuf, rgbBuf1, width, height, movieVersion );
		firstFrame = false;
	}
	else
	{
		if( movieVersion < 4 )
		{
			if( movieVersion == 2 )
				unrlediff2( rleBuf, rgbBuf1, width, height, movieVersion );
			else if( movieVersion == 3 )
				unrlediff3( rleBuf, rgbBuf1, width, height, movieVersion );
		}
		else
		{
			unrlediff4( rleBuf, rgbBuf1, width, height, movieVersion );
		}
	}
	glRasterPos2i( 0, 0 );
#if 0
	for( int line = 0; line < height; line += 5 )
	{
		printf( "[%3d]  ", line );
		for( int i = 0; i < 50; i += 5 )
			printf( "%08lx  ", rgbBuf1[line*width + i] );
		printf( "\n" );
	}
#endif
	glDrawPixels( width, height, GL_RGBA, GL_UNSIGNED_BYTE, rgbBuf1 );

//	glRecti( frame*10, frame*10, (frame+5)*10, (frame+5)*10 );

	swapBuffers();
	
//	sleep( 3 );
}

void GLWidget::paintGL()
{
}

void GLWidget::resizeGL( int widthParam, int heightParam )
{
	width = widthParam;
	height = heightParam;
	
	glwPrint( "width = %lu, height = %lu\n", width, height );
}
