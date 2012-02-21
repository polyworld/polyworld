#include "termio.h"

#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>


namespace termio
{

	bool isKeyPressed()
	{
		struct timeval tv;
		fd_set fds;
		tv.tv_sec = 0;
		tv.tv_usec = 0;
		FD_ZERO(&fds);
		FD_SET(STDIN_FILENO, &fds); //STDIN_FILENO is 0
		select(STDIN_FILENO+1, &fds, NULL, NULL, &tv);
		return FD_ISSET(STDIN_FILENO, &fds);
	}

	void setBlockingInput( bool enabled )
	{
		struct termios ttystate;
 
		//get the terminal state
		tcgetattr(STDIN_FILENO, &ttystate);
 
		if( !enabled )
		{
			//turn off canonical mode
			ttystate.c_lflag &= ~ICANON;
			//minimum of number input read.
			ttystate.c_cc[VMIN] = 1;
		}
		else
		{
			//turn on canonical mode
			ttystate.c_lflag |= ICANON;
		}
		//set the terminal attributes.
		tcsetattr(STDIN_FILENO, TCSANOW, &ttystate);
	}

	void discardInput()
	{
		tcflush( STDIN_FILENO, TCIFLUSH );
	}

	void setEchoEnabled( bool enabled )
	{
		if( enabled )
			system( "stty echo" );
		else
			system( "stty -echo" );
	}

}
/*
int main( int argc, char **argv )
{
	nonblock( NB_ENABLE );
	for( int i = 0; i < 10; i++ )
	{
		cout << "kbhit: " << kbhit() << endl;
		if( kbhit() )
		{
			tcflush( 0, TCIFLUSH );

			nonblock( NB_DISABLE );

			cout << "$ ";

			char buf[128];
			char *result = fgets( buf, sizeof(buf), stdin );
			cout << "input='" << result << "'" << endl;

			nonblock( NB_ENABLE );
		}
		sleep( 1 );
	}

	nonblock( NB_DISABLE );
	
	return 0;
}
*/
