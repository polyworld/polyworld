
/********************************************************************/
/* PolyWorld:  An Artificial Life Ecological Simulator              */
/* by Larry Yaeger                                                  */
/* Copyright Apple Computer 1990,1991,1992                          */
/********************************************************************/

// System
#include <stdarg.h>

// Self
#include "debug.h"

// Local
#include "agent.h"
#include "barrier.h"
#include "error.h"
#include "gpolygon.h"
#include "graphics.h"
#include "misc.h"
#include "Simulation.h"

#define DEBUGCHECK_SPECIFICS 1

void DebugCheck( const char* func, const char* frmt, ... )
{
	va_list ap;
	static char slast[1024] = "\n";
	char s[1024];
	long step;
	gobject* a = NULL;
	gobject* abad = NULL;
	gobject* b = NULL;
	gobject* bbad = NULL;
	
	gdlink<gobject*> *saveCurr = objectxsortedlist::gXSortedObjects.getcurr();	// save the state of the x-sorted list
	
	objectxsortedlist::gXSortedObjects.reset();
	objectxsortedlist::gXSortedObjects.nextObj( AGENTTYPE, (gobject**) &a );
	if( a )
		step = ((agent*)a)->fSimulation->fStep;
	else
		step = 0;
	
	sprintf( s, "%4ld: %15s: ", step, func );
	
	va_start( ap, frmt );
	vsprintf( s+strlen(s), frmt, ap );
	va_end( ap );
	
#if DEBUGCHECK_SPECIFICS
	bool caughtError = false;
	//gpolyobj* agentobj = agent::GetAgentObj();

	// look for any object that thinks it is being carried by an agent that isn't carrying it

	
	objectxsortedlist::gXSortedObjects.reset();
	while( objectxsortedlist::gXSortedObjects.nextObj( ANYTYPE, (gobject**) &a ) )
	{
		b = a->CarriedBy();
		
		if( b != NULL )	// a thinks it is being carried by b1
		{
			//printf( "%ld: %s # %lu thinks it is being carried by %s # %lu, which is carrying",
			//		((agent*)b)->fSimulation->fStep, OBJECTTYPE( a ), a->getTypeNumber(), OBJECTTYPE( b ), b->getTypeNumber() );
		
			bool found = false;

			if( b->fCarries.size() > 0 )
			{
				itfor( std::list<gobject*>, b->fCarries, it )
				{
					gobject* c = *it;
					//printf( " %s # %lu", OBJECTTYPE( c ), c->getTypeNumber() );
					
					if( c == a )
					{
						found = true;
						break;	// comment this line out if printing all the intermediate values
					}
				}
				//printf( "\n" );
			}
			else
			{
				//printf( " nothing\n" );
			}

			if( !found )
			{
				printf( "object thinks it is being carried by an object that is not carrying it\n" );
				caughtError = true;
				abad = a;
				bbad = b;
				break;
			}
		}
	}
	
	objectxsortedlist::gXSortedObjects.setcurr( saveCurr );	// restore the state of the x-sorted list

	//if( step > 410 )
	//	printf( "%s\n", s );
	if( caughtError )
	{
		printf( "ERROR: %s\n", s );
		printf( "%ld: %s # %lu thinks it is being carried by %s # %lu, which is carrying",
				step, OBJECTTYPE( abad ), abad->getTypeNumber(), OBJECTTYPE( bbad ), bbad->getTypeNumber() );
		if( bbad->fCarries.size() > 0 )
		{
			itfor( std::list<gobject*>, bbad->fCarries, it )
			{
				gobject* c = *it;
				printf( " %s # %lu", OBJECTTYPE( c ), c->getTypeNumber() );
			}
			printf( "\n" );
		}
		else
		{
			printf( " nothing\n" );
		}
		if( strlen( slast ) > 0 )
			printf( "    PREV: %s\n", slast );
		else
			printf( "    PREV: (string empty)\n" );
		
		exit( 0 );
	}
#else
	printf( "%s\n", s );
#endif        

    strcpy( slast, s );
}


