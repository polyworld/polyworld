#define GLW_DEBUG 0

// System
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <sstream>

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

GLWidget::GLWidget( QWidget *parent,
					char** legendParam )
	: QGLWidget( parent )
{
//	printf( "%s: width = %lu\n", __func__, width );

	legend = legendParam;
	frame = NULL;

	glwPrint( "width = %lu, height = %lu\n", width, height );
}

GLWidget::~GLWidget()
{
}

void GLWidget::SetFrame( Frame *frame )
{
	this->frame = frame;

	Draw();
}

void GLWidget::initializeGL()
{
	glwPrint( "called\n" );

	qglClearColor( Qt::black );
    glColor4ub( 255, 255, 255, 255 );
	setAutoBufferSwap( false );
}

void GLWidget::Draw()
{
	makeCurrent();

	if( frame == NULL )
	{
		glClear(GL_COLOR_BUFFER_BIT);
	}
	else
	{
		glRasterPos2i( 0, 0 );
		glPixelZoom( width()/float(frame->width), height()/float(frame->height) );
		glDrawPixels( frame->width, frame->height, GL_RGBA, GL_UNSIGNED_BYTE, frame->rgbBuf );

		// Superimpose the timestep number
		QFont font( "Monospace", 8 );
		font.setStyleHint( QFont::TypeWriter );
		char timestepString[16];
		sprintf( timestepString, "%8u", frame->timestep );
		renderText( width() - 60, 15, timestepString, font );

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
	}

	// Done drawing, so show it
	swapBuffers();
}

void GLWidget::Write( FILE *file )
{
	fwrite( frame->rgbBuf, sizeof(uint32_t), frame->width * frame->height, file );
}

void GLWidget::Save()
{
	std::stringstream fileName;
	fileName << frame->index << ".png";
	QImage image( (uchar*)frame->rgbBuf, frame->width, frame->height, QImage::Format_ARGB32 );
	image.rgbSwapped().mirrored().save( fileName.str().c_str() );
}

void GLWidget::paintGL()
{
	Draw();
}

void GLWidget::resizeGL( int width, int height )
{
	glViewport( 0, 0, width, height );
	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	glOrtho( 0, width, 0, height, -1.0, 1.0 );

	glwPrint( "width = %lu, height = %lu\n", width, height );
}
