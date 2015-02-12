#pragma once

#include "PwMovieUtils.h"

//===========================================================================
// PwMovieQGLPixelBufferRecorder
//===========================================================================
class PwMovieQGLPixelBufferRecorder
{
 public:
	PwMovieQGLPixelBufferRecorder( class QGLPixelBuffer *pixelBuffer, PwMovieWriter *writer );
	~PwMovieQGLPixelBufferRecorder();
	
	void recordFrame( uint32_t timestep );

 private:
	class QGLPixelBuffer *pixelBuffer;
	PwMovieWriter *writer;
	uint32_t width;
	uint32_t height;
	uint32_t *rgbBufOld;
	uint32_t *rgbBufNew;
};
