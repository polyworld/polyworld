#pragma once

#include <map>

#include <QObject>

class AgentPovRenderer : public QObject
{
	Q_OBJECT

 public:
	AgentPovRenderer( int maxAgents,
					  int retinaWidth,
					  int retinaHeight );
	virtual ~AgentPovRenderer();

	void add( class agent *a );
	void remove( class agent *a );

	void beginStep();
	void render( class agent *a );
	void endStep();
	void copyBufferTo( class QGLWidget *dst );

	int getBufferWidth();
	int getBufferHeight();

 signals:
	void stepRenderComplete();

 private:
	struct Viewport
	{
		int index;
		short x;
		short y;
		short width;
		short height;
	};
	class QGLPixelBuffer *fPixelBuffer;
	int fBufferWidth;
	int fBufferHeight;
	Viewport *fViewports;
	std::map<int, Viewport *> fFreeViewports;
};
