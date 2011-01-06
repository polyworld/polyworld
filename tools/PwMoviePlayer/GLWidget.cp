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

GLWidget::GLWidget( QWidget *parent, uint32_t widthParam, uint32_t heightParam, uint32_t movieVersionParam,
					FILE* movieFileParam, char** legendParam, uint32_t endFrameParam, double frameRateParam )
	: QGLWidget( parent )
{
	width = widthParam;
//	printf( "%s: width = %lu\n", __func__, width );

	if( width == 0 )
		return;
	
	height = heightParam;
	movieVersion = movieVersionParam;
	movieFile = movieFileParam;
	legend = legendParam;
	endFrame = endFrameParam;
	frameRate = frameRateParam;

	glwPrint( "width = %lu, height = %lu\n", width, height );

	QRect rect( 0, 0, width, height );
	setGeometry( rect );

	// Allocate the rle and rgb buffers for movie decoding and playback
	rgbBufSize = width * height * sizeof( *rgbBuf1 );
	rleBufSize = rgbBufSize + 1;	// it better compress!
	rgbBuf1 = (uint32_t*) malloc( rgbBufSize );
//	rgbBuf2 = (uint32_t*) malloc( rgbBufSize );
	rleBuf  = (uint32_t*) malloc( rleBufSize );
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
	glViewport( 0, 0, width, height );
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	glOrtho( 0, width, 0, height, -1.0, 1.0 ); 
    glColor4ub( 255, 255, 255, 255 );
	setAutoBufferSwap( false );
}

void GLWidget::Draw()
{
#define RecentSteps 10
	static bool				firstFrame = true;
	static uint32_t	frame;
	static double			timePrevious;
	double					timeNow;	
	
	if( width == 0 )
		return;
	
	frame++;
//	printf( "%s: frame # %lu\n", __FUNCTION__, frame );
	
	if( endFrame && (frame > endFrame) )
		exit( 0 );	
	
	if( frameRate )
	{
		double	deltaTime;
		double	ifr = 1.0 / frameRate;
		
		timeNow = hirestime();
		deltaTime = timeNow - timePrevious;
		
		if( deltaTime < ifr )	// too soon, so delay
		{
//			printf( "%4lu: t=%g, dt=%g, ifr=%g, s=%g\n", frame, timeNow, deltaTime, ifr, (ifr-deltaTime)*1000000 );
			usleep( (ifr - deltaTime) * 1000000 );
		}
		
		timePrevious = hirestime();
	}
	
	makeCurrent();
	
	if( readrle( movieFile, rleBuf, movieVersion, firstFrame ) )	// 0 = success, other = failure
	{
		exit( 0 );
//		return;
	}
	
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

#if 0
// Halve the number of pixels
	int newWidth= width / 2;
	int newHeight = height / 2;
	for( int j = 0; j < newHeight; j++ )
	{
		for( int i = 0; i < newWidth; i++ )
		{
			rgbBuf1[i + j*newWidth] = round( 0.25 * (rgbBuf1[2*j*width + 2*i] + rgbBuf1[2*j*width + 2*i + 1] + rgbBuf1[(2*j+1)*width + 2*i] + rgbBuf1[(2*j+1)*width + 2*i + 1] ) );
		}
	}
	glDrawPixels( newWidth, newHeight, GL_RGBA, GL_UNSIGNED_BYTE, rgbBuf1 );
#else
	glDrawPixels( width, height, GL_RGBA, GL_UNSIGNED_BYTE, rgbBuf1 );
#endif

	// Superimpose the frame number
	QFont font( "Monaco", 10 );
	char frameString[16];
	sprintf( frameString, "%8lu", frame );
	renderText( width - 54, 15, frameString, font );
	
	// Draw the legend
	if( legend )
	{
		int i = 0;
		int y = 24;

		QFont font2( "Arial", 12 );
		QFont font3( "Arial", 20 );
		while( legend[i] )
		{
			if( i == 0 )
				renderText( 10, y, legend[i], font2 );
			else
				renderText( 10, y, legend[i], font3 );
			i++;
			y += 24;
		}
	}
	
	// Done drawing, so show it
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
