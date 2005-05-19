
/********************************************************************/
/* PolyWorld:  An Artificial Life Ecological Simulator              */
/* by Larry Yaeger                                                  */
/* Copyright Apple Computer 1990,1991,1992                          */
/********************************************************************/

// Self
#include "debug.h"

// Local
#include "barrier.h"
#include "critter.h"
#include "error.h"
#include "graphics.h"

void debugcheck(const char* s)
{
	static const char* slast = NULL;
#if defined(DEBUGCHECK)
	bool caughtError = false;
	gpolyobj* critterobj = critter::GetCritterObj();
	
	if( critterobj )
	{
		if( critterobj->numPolygons() != 10 )
		{
			caughtError = true;
			printf( "Bad critterobj->numPolygons() (%ld): %s\n", critterobj->numPolygons(), s );
		}
	}
	else
	{
		caughtError = true;
		printf( "NULL critterobj: %s\n", s );
	}

  #if 0
	TODO
    TCastList* c = globals::worldstage.cast();
    if (c != NULL)
    {
		[TODO]
		if (c->kount)
        {
            c->checklast(s);
            if (!(c->last))
                printf("Bad l: %s\n",s);
            else if (!(c->last->next))
                printf("Bad l-n: %s\n",s);
            else if (!(c->last->next->next))
                printf("Bad l-n-n: %s\n",s);
            else if (!(c->last->next->next->next))
                printf("Bad l-n-n-n: %s\n",s);
            else if (!(c->last->next->next->next->next))
                printf("Bad l-n-n-n-n: %s\n",s);
        }
    }
    else
    {
        printf("cast NULLed out: %s\n",s);
	}

    if (globals::maxneurons > 217)
        printf("Bad maxneurons(%d): %s\n", globals::maxneurons, s);
        
    if (globals::maxsynapses > 45584)
        printf("Bad maxsynapses(%ld): %s\n", globals::maxsynapses, s);
  #endif

	if( caughtError )
	{
		if( slast )
			printf("debugcheck last called from: %s\n",slast);
		else
			printf("debugcheck: sorry, slast is NULL\n");
		
		exit(0);
	}
#endif        

    slast = s;
}


