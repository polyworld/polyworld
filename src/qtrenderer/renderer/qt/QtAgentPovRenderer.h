#pragma once

#include <map>

#include "agent/AgentAttachedData.h"
#include "agent/AgentPovRenderer.h"


class QtAgentPovRenderer : public AgentPovRenderer
{
 public:
	QtAgentPovRenderer( int maxAgents,
                        int retinaWidth,
                        int retinaHeight );
	virtual ~QtAgentPovRenderer();

	virtual void add( class agent *a ) override;
	virtual void remove( class agent *a ) override;

	virtual void beginStep() override;
	virtual void render( class agent *a ) override;
	virtual void endStep() override;

	void copyTo( class QGLWidget *dst );
	int getBufferWidth();
	int getBufferHeight();

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
