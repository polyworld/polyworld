#include "Prompt.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>

#include "termio.h"

using namespace std;

static void exit_cleanup()
{
	termio::setEchoEnabled( true );
}

//===========================================================================
// Prompt
//===========================================================================

//---------------------------------------------------------------------------
// Prompt::Prompt
//---------------------------------------------------------------------------
Prompt::Prompt()
: inputThread( NULL )
, pendingDismiss( false )
{
	atexit( exit_cleanup );

	termio::setBlockingInput( false );
	termio::setEchoEnabled( false );
}

//---------------------------------------------------------------------------
// Prompt::~Prompt
//---------------------------------------------------------------------------
Prompt::~Prompt()
{
}

//---------------------------------------------------------------------------
// Prompt::isActive
//---------------------------------------------------------------------------
bool Prompt::isActive()
{
	return inputThread != NULL;
}

//---------------------------------------------------------------------------
// Prompt::getUserInput
//---------------------------------------------------------------------------
char *Prompt::getUserInput()
{
	char *result = NULL;

	MUTEX( lock,
		   {
			   if( pendingDismiss )
			   {
				   inputThread->wait();
				   delete inputThread;
				   inputThread = NULL;
				   pendingDismiss = false;

				   termio::setBlockingInput( false );
				   termio::setEchoEnabled( false );
			   }

			   if( (inputThread == NULL) && termio::isKeyPressed() )
			   {
				   termio::discardInput();
				   termio::setBlockingInput( true );
				   termio::setEchoEnabled( true );
				   
				   inputThread = new InputThread( this );
				   inputThread->start();
			   }

			   if( !userInputQueue.empty() )
			   {
				   result = userInputQueue.front();
				   userInputQueue.pop();
			   }
		   } );

	return result;
}

//---------------------------------------------------------------------------
// Prompt::dismissed
//---------------------------------------------------------------------------
void Prompt::dismissed()
{
	pendingDismiss = true;
}

//---------------------------------------------------------------------------
// Prompt::addUserInput
//---------------------------------------------------------------------------
void Prompt::addUserInput( char *input )
{
	MUTEX( lock,
		   {
			   userInputQueue.push( input );
		   } );
}

//===========================================================================
// InputThread
//===========================================================================

Prompt::InputThread::InputThread( Prompt *_prompt )
	: prompt( _prompt )
{
}

Prompt::InputThread::~InputThread()
{
}

void Prompt::InputThread::run()
{
	printf( "$ " );
	char buf[4096];

	char *input = fgets( buf, sizeof(buf), stdin );
		
	// strip trailing newline
	input[ strlen(input) - 1 ] = 0;
		
	prompt->addUserInput( strdup(input) );

	prompt->dismissed();
}

