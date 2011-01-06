#ifndef GLWidget_h
#define GLWidget_h

#include <stdint.h>

#include <QGLWidget>

#include "PwMovieUtils.h"

class GLWidget : public QGLWidget
{
	Q_OBJECT

public:
	GLWidget( QWidget *parent = 0, uint32_t widthParam = 400, uint32_t heightParam = 400, uint32_t movieVersionParam = kCurrentMovieVersion,
				FILE* movieFileParam = NULL, char** legendParam = NULL, uint32_t endFrameParam = 0, double frameRateParam = 0.0 );
	~GLWidget();
	
	void Draw();

	QSize minimumSizeHint() const;
	QSize sizeHint() const;

protected:
	void initializeGL();
	void paintGL();
	void resizeGL( int width, int height );

private:
	uint32_t	width;
	uint32_t	height;
	uint32_t	movieVersion;
	FILE*			movieFile;
	char**			legend;
	uint32_t	endFrame;
	double			frameRate;

	uint32_t		rgbBufSize;
	uint32_t		rleBufSize;
	uint32_t*		rgbBuf1;	// actually allocated
//	uint32_t*		rgbBuf2;	// actually allocated
	uint32_t*		rleBuf;
};

#endif
