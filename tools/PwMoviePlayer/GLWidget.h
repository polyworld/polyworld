#ifndef GLWidget_h
#define GLWidget_h

#include <stdint.h>

#include <QGLWidget>

#include "PwMovieUtils.h"

class GLWidget : public QGLWidget
{
	Q_OBJECT

public:
	struct Frame
	{
		uint32_t    index;
		uint32_t    timestep;
		uint32_t	width;
		uint32_t	height;
		uint32_t*	rgbBuf;
	};

	GLWidget( QWidget *parent = 0,
			  char** legendParam = NULL );
	~GLWidget();

	void SetFrame( Frame *frame );
	
	void Draw();

protected:
	void initializeGL();
	void paintGL();
	void resizeGL( int width, int height );

private:
	char **legend;
	Frame *frame;
};

#endif
