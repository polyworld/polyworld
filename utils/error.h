/********************************************************************/
/* PolyWorld:  An Artificial Life Ecological Simulator              */
/* by Larry Yaeger                                                  */
/* Copyright Apple Computer 1990,1991,1992                          */
/********************************************************************/
// error.h: definition of standard error reporting mechanism

#ifndef ERROR_H
#define ERROR_H

#include "debug.h"

void error(     int level, int delay, const char* msg);
void error(     int level, const char* msg);
void errorwait( int level, const char* msg);
void errorflash(int level, const char* msg);
void error(     int level, const char* msg1, const char* msg2);
void error(     int level, const char* msg1, const char* msg2, const char* msg3);
void error(     int level, const char* msg1, const char* msg2, const char* msg3,
                                   const char* msg4);
void error(     int level, const char* msg1, const char* msg2, const char* msg3,
                                   const char* msg4, const char* msg5);
void error(     int level, const char* msg1, const char* msg2, const char* msg3,
                                   const char* msg4, const char* msg5, const char* msg6);
void error(     int level, const char* msg1, const char* msg2, const char* msg3,
                                   const char* msg4, const char* msg5, const char* msg6,
                                   const char* msg7);
void error(     int level, const char* msg1, long  num2, const char* msg3);
void error(     int level, const char* msg1, long  num2, const char* msg3,
                                   const char* msg4, const char* msg5);
void error(     int level, const char* msg1, long  num2, const char* msg3,
                                   long  num4, const char* msg5);
void error(     int level, const char* msg1, const char* msg2, long  num3,
                                   const char* msg4, long  num5, const char* msg6,
                                   long  num7);

#endif

