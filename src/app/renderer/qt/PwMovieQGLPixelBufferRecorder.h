#pragma once

#include "MovieRecorder.h"
#include "PwMovieUtils.h"

//===========================================================================
// PwMovieQGLPixelBufferRecorder
//===========================================================================
class PwMovieQGLPixelBufferRecorder : public MovieRecorder
{
 public:
	PwMovieQGLPixelBufferRecorder( class QGLPixelBuffer *pixelBuffer, PwMovieWriter *writer );
	virtual ~PwMovieQGLPixelBufferRecorder();
	
	virtual void recordFrame( uint32_t timestep ) override;

 private:
	class QGLPixelBuffer *pixelBuffer;
	PwMovieWriter *writer;
	uint32_t width;
	uint32_t height;
	uint32_t *rgbBufOld;
	uint32_t *rgbBufNew;
};
