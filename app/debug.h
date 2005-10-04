#ifndef DEBUG_H
#define DEBUG_H

/********************************************************************/
/* PolyWorld:  An Artificial Life Ecological Simulator              */
/* by Larry Yaeger                                                  */
/* Copyright Apple Computer 1990,1991,1992                          */
/********************************************************************/
//#define DEBUGCHECK

#define DebugSetRadius 0
#define TestWorld 0

#define dbprintf( x... ) { fprintf( stderr, x ); fflush( stderr ); }

// for error logging:
#define eprintf( x... ) { fprintf( stderr, "%s/%d: ", __FILE__, __LINE__ ); fprintf( stderr, x ); fflush( stderr ); }

#if DebugSetRadius
	#define srPrint( x... ) dbprintf( x )
#else
	#define srPrint( x... )
#endif

extern void  debugcheck(const char* s);

#define BoolString( boolVar ) (boolVar) ? "true" : "false"

#endif DEBUG_H



