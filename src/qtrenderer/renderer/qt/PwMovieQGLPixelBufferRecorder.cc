#include "PwMovieQGLPixelBufferRecorder.h"

#include <gl.h>
#include <QGLPixelBuffer>

//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---
//--- PwMovieQGLPixelBufferRecorder
//---
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
PwMovieQGLPixelBufferRecorder::PwMovieQGLPixelBufferRecorder( QGLPixelBuffer *pixelBuffer,
															  PwMovieWriter *writer )
{
	this->pixelBuffer = pixelBuffer;
	this->writer = writer;

	width = pixelBuffer->size().width();
	height = pixelBuffer->size().height();
	
	rgbBufOld = NULL;
	rgbBufNew = NULL;

	uint32_t rgbBufSize = width * height * sizeof(*rgbBufNew);
	rgbBufOld = (uint32_t *)malloc( rgbBufSize );
	rgbBufNew = (uint32_t *)malloc( rgbBufSize );
}

PwMovieQGLPixelBufferRecorder::~PwMovieQGLPixelBufferRecorder()
{
	if( rgbBufOld ) free( rgbBufOld );
	if( rgbBufNew ) free( rgbBufNew );
}

void PwMovieQGLPixelBufferRecorder::recordFrame( uint32_t timestep )
{
	pixelBuffer->makeCurrent();

	glReadPixels( 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, rgbBufNew );

	writer->writeFrame( timestep, width, height, rgbBufOld, rgbBufNew );

	uint32_t *rgbBufSwap = rgbBufNew;
	rgbBufNew = rgbBufOld;
	rgbBufOld = rgbBufSwap;

	pixelBuffer->doneCurrent();
}
