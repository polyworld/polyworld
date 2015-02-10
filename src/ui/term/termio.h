#pragma once

namespace termio
{
	bool isKeyPressed();
	void setBlockingInput( bool enabled );
	void discardInput();
	void setEchoEnabled( bool enabled );
}
