#ifndef DEBUG_H
#define DEBUG_H

/********************************************************************/
/* PolyWorld:  An Artificial Life Ecological Simulator              */
/* by Larry Yaeger                                                  */
/* Copyright Apple Computer 1990,1991,1992                          */
/********************************************************************/
#define DEBUGCHECK 0

#define DebugSetRadius 0
#define TestWorld 0

#define dbprintf( x... ) ( fprintf( stderr, x ), fflush( stderr ) )

// for error logging:
#define eprintf( x... ) ( fprintf( stderr, "%s/%d: ", __FUNCTION__, __LINE__ ), fprintf( stderr, x ), fflush( stderr ) )

#if DebugSetRadius
	#define srPrint( x... ) dbprintf( x )
#else
	#define srPrint( x... )
#endif

extern void DebugCheck( const char* func, const char* fmt, ... );

#if DEBUGCHECK
	#define debugcheck( x... ) DebugCheck( __FUNCTION__, x )
#else
	#define debugcheck( x... )
#endif

#define BoolString( boolVar ) (boolVar) ? "true" : "false"

#endif //  DEBUG_H



