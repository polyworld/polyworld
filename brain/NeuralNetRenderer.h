#pragma once

class NeuralNetRenderer
{
 public:
	virtual ~NeuralNetRenderer() {}

	virtual void getSize( short patchWidth, short patchHeight,
						  short *ret_width, short *ret_height ) = 0;
	virtual void render( short patchWidth, short patchHeight ) = 0;
};
