/********************************************************************/
/* PolyWorld:  An Artificial Life Ecological Simulator              */
/* by Larry Yaeger                                                  */
/* Copyright Apple Computer 1990,1991,1992                          */
/********************************************************************/

// error.C: implementation of standard error reporting mechanism

// System
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Local
#include "graphics.h"

// Self
#include "error.h"

using namespace std;

#define LOOPSPERSECOND 5000

char errorstring[256];


bool exitnow;

#if 0
void mywait(int errwait)
{
   long dev;
   short data;

   qdevice(LEFTMOUSE);
   qdevice(KEYBD);

   if (errwait < 0)
   {
      while (((dev = qread(&data)) != LEFTMOUSE) && (dev != KEYBD))
         { qenter(short(dev),data); }
      if ((dev == KEYBD) && (data == 27)) // ESCape
          exitnow = true;
   }
   else
   {
      long waitlength = ((errwait == 0) ? 5 : errwait) * LOOPSPERSECOND;
      long waited = 0;
      bool done = false;
      while ((!done) && (waited++ < waitlength))
      {
         if (qtest())
         {
            dev = qread(&data);
            if ((dev == LEFTMOUSE) || (dev == KEYBD))
            {
               done = true;
               if ((dev == KEYBD) && (data == 27)) // ESCape
                   exitnow = true;
            }
            else
                qenter(short(dev),data);
         }
      }
   }
}
#endif

void error(int errlevel, int /*errwait*/, const char* errmsg)
{
// errlevel = 0 ==> warning only
//            1 ==> error, but no abort
//            2 ==> error, abort
// errwait >= 0 ==> sleep(errwait) [0 defaults to 5]
//          < 0 ==> wait for LEFTMOUSE or KEYBD
   exitnow = (errlevel > 1);
   char errmsg2[256];
   strcpy(errmsg2, "PolyWorld");
   if (errlevel)
      strcat(errmsg2," ERROR: ");
   else
      strcat(errmsg2," WARNING: ");
   strcat(errmsg2,errmsg);
   strcat(errmsg2,"\n");
   cerr << errmsg2;
   
#if 0
   if (globals::graphics != 0)
   {
		[TODO]
       gwindow errwin;
       errwin.setprefposition(319,959,511,767);
       errwin.setborder(true);
       errwin.setframewidth(0);
       errwin.setdoublebuffer(false);
       errwin.setvisibility(true);
       errwin.settitle("Error message");
       errwin.open();
       errwin.dumbtext(errmsg2);
       mywait(errwait);
       errwin.clearc();
       errwin.close();
   }
#endif

   cerr.flush();

	if (exitnow)
		exit(errlevel);
}


void error(     int level, const char* msg) { error(level,-1,msg); }


void errorwait( int level, const char* msg) { error(level,-1,msg); }


void errorflash(int level, const char* msg) { error(level, 5,msg); }


void error(     int level, const char* msg1, const char* msg2)
{
    sprintf(errorstring,"%s %s", msg1, msg2);
    error(level,-1,errorstring);
}


void error(     int level, const char* msg1, const char* msg2, const char* msg3)
{
    sprintf(errorstring,"%s%s%s",msg1,msg2,msg3);
    error(level,-1,errorstring);
}


void error(     int level, const char* msg1, const char* msg2, const char* msg3, const char* msg4)
{
    sprintf(errorstring,"%s%s%s%s",msg1,msg2,msg3,msg4);
    error(level,-1,errorstring);
}


void error(     int level, const char* msg1, const char* msg2, const char* msg3, const char* msg4,
                            const char* msg5)
{
    sprintf(errorstring,"%s%s%s%s%s",msg1,msg2,msg3,msg4,msg5);
    error(level,-1,errorstring);
}


void error(     int level, const char* msg1, const char* msg2, const char* msg3, const char* msg4,
                            const char* msg5, const char* msg6)
{
    sprintf(errorstring,"%s%s%s%s%s%s",msg1,msg2,msg3,msg4,msg5,msg6);
    error(level,-1,errorstring);
}


void error(int level, const char* msg1, const char* msg2, const char* msg3, const char* msg4,
                            const char* msg5, const char* msg6, const char* msg7)
{
    sprintf(errorstring,"%s%s%s%s%s%s%s",msg1,msg2,msg3,msg4,msg5,msg6,msg7);
    error(level,-1,errorstring);
}


void error(int level, const char* msg1, long num2, const char* msg3)
{
    sprintf(errorstring,"%s %ld %s",msg1,num2,msg3);
    error(level,-1,errorstring);
}


void error(     int level, const char* msg1, long num2, const char* msg3, const char* msg4,
                            const char* msg5)
{
    sprintf(errorstring,"%s %ld %s %s %s",msg1,num2,msg3,msg4,msg5);
    error(level,-1,errorstring);
}


void error(     int level, const char* msg1, long num2, const char* msg3, long  num4,
                            const char* msg5)
{
    sprintf(errorstring,"%s %ld %s %ld %s",msg1,num2,msg3,num4,msg5);
    error(level,-1,errorstring);
}


void error(     int level, const char* msg1, const char* msg2, long  num3, const char* msg4,
                            long  num5, const char* msg6, long  num7)
{
    sprintf(errorstring,"%s %s %ld %s %ld %s %ld",msg1,msg2,num3,msg4,num5,msg6,num7);
    error(level,-1,errorstring);
}

