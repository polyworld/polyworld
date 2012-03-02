#pragma once

#include <map>

#include <QObject>

#include "AgentAttachedData.h"

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
	void copyTo( class QGLWidget *dst );

	int getBufferWidth();
	int getBufferHeight();

 signals:
	void renderComplete();

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
	// This gives us a reference to a per-agent opaque pointer.
	AgentAttachedData::SlotHandle slotHandle;
	Viewport *fViewports;
	std::map<int, Viewport *> fFreeViewports;
};
