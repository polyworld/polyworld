#ifndef GLWidget_h
#define GLWidget_h

#include <QGLWidget>

#include "PwMovieTools.h"

class GLWidget : public QGLWidget
{
	Q_OBJECT

public:
	GLWidget( QWidget *parent = 0, unsigned long widthParam = 400, unsigned long heightParam = 400, unsigned long movieVersionParam = kCurrentMovieVersion,
				FILE* movieFileParam = NULL, char** legendParam = NULL, unsigned long endFrameParam = 0, double frameRateParam = 0.0 );
	~GLWidget();
	
	void Draw();

	QSize minimumSizeHint() const;
	QSize sizeHint() const;

protected:
	void initializeGL();
	void paintGL();
	void resizeGL( int width, int height );

private:
	unsigned long	width;
	unsigned long	height;
	unsigned long	movieVersion;
	FILE*			movieFile;
	char**			legend;
	unsigned long	endFrame;
	double			frameRate;

	ulong		rgbBufSize;
	ulong		rleBufSize;
	ulong*		rgbBuf1;	// actually allocated
//	ulong*		rgbBuf2;	// actually allocated
	ulong*		rleBuf;
};

#endif