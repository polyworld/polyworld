/********************************************************************/
/* PolyWorld:  An Artificial Life Ecological Simulator              */
/* by Larry Yaeger                                                  */
/* Copyright Apple Computer 1990,1991,1992                          */
/********************************************************************/

// misc.h: miscellaneous useful defines & short procedures

#ifndef MISC_H
#define MISC_H

#include <errno.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <vector>

#define nl <<"\n"
#define pnl <<")\n"
#define qnl <<"\"\n"
#define nlf <<"\n"; cout.flush()
#define pnlf <<")\n"; cout.flush()
#define qnlf <<"\"\n"; cout.flush()
#define comma <<","<<
#define cms <<", "<<
#define sp <<" "<<
#define sp2 <<"  "<<
#define ses <<" = "<<
#define eql <<"="<<
#define sopar <<" ("<<
#define cpars <<") "<<

#define nint(a) ((long)((a)+(((a)<0.0)?-0.499999999:0.499999999)))

#define interp(x,ylo,yhi) ((ylo)+(x)*((yhi)-(ylo)))

#define randpw() drand48()
#define rrand(lo,hi) (interp(randpw(),(lo),(hi)))
double nrand();
double nrand(double mean, double stdev);

#define index2(i,j,nj) ((i)*(nj)+(j))
#define index3(i,j,k,nj,nk) ((i)*(nj)*(nk)+(j)*(nk)+(k))
#define index4(i,j,k,l,nj,nk,nl) ((i)*(nj)*(nk)*(nl)+(j)*(nk)*(nl)+(k)*(nl)+(l))

#define getbit(a,b) (((a)>>(b))&1)

#define lxor(p,q) ( !((p) && (q)) && ((p) || (q)) )

inline int   sign(int   x) { return (x<0)  ? -1  : 1 ; }
inline long  sign(long  x) { return (x<0)  ? -1  : 1 ; }
inline float sign(float x) { return (x<0.) ? -1. : 1.; }


char* concat(const char* s1, const char* s2);
char* concat(const char* s1, const char* s2, const char* s3);
char* concat(const char* s1, const char* s2, const char* s3, const char* s4);
char* concat(const char* s1, const char* s2, const char* s3, const char* s4, const char* s5);
char* concat(const char* s1, const char* s2, const char* s3, const char* s4, const char* s5, const char* s6);
char* concat(const char* s1, const char* s2, const char* s3, const char* s4, const char* s5, const char* s6,
             const char* s7);


std::string operator+(const char *, const std::string &);

char* itoa(long i);
char* ftoa(float f);

inline float fmax(float f1, float f2, float f3)
    { return (f2>f1) ? fmax(f2,f3) : fmax(f1,f3); }
inline float fmax(float f1, float f2, float f3, float f4)
    { return (f2>f1) ? fmax(f2,f3,f4) : fmax(f1,f3,f4); }
inline float fmax(float f1, float f2, float f3, float f4, float f5)
    { return (f2>f1) ? fmax(f2,f3,f4,f5) : fmax(f1,f3,f4,f5); }


inline float fmin(float f1, float f2, float f3)
    { return (f2<f1) ? fmin(f2,f3) : fmin(f1,f3); }
inline float fmin(float f1, float f2, float f3, float f4)
    { return (f2<f1) ? fmin(f2,f3,f4) : fmin(f1,f3,f4); }
inline float fmin(float f1, float f2, float f3, float f4, float f5)
    { return (f2 < f1) ? fmin(f2, f3, f4, f5) : fmin(f1, f3, f4, f5); }

#define clamp(VAL, MIN, MAX) ((VAL) < (MIN) ? (MIN) : ((VAL) > (MAX) ? (MAX) : (VAL)))


float logistic(float x, float slope);
float biasedLogistic(float x, float bias, float slope);
float gaussian( float x, float mean, float variance );

inline float dist( float x1, float y1, float x2, float y2 ) { return sqrt( (x1-x2)*(x1-x2) + (y1-y2)*(y1-y2) ); }

bool exists( const std::string &path );
std::string dirname( const std::string &path );
void makeDirs( const std::string &path );
void makeParentDir( const std::string &path );
int SetMaximumFiles( long filecount );
int GetMaximumFiles( long *filecount );

std::vector<std::string> split( const std::string& str, const std::string& delimiters = " " );

#ifndef PI
#define PI M_PI
#endif /*PI*/

#define TWOPI 6.28318530717059647602
#define HPI M_PI_2
#define RADTODEG 57.29577951
#define DEGTORAD 0.017453292

// iterator for loop
#define itfor(TYPE,CONT,IT)						\
	for(TYPE::iterator					\
			IT = (CONT).begin(),				\
			IT##_end = (CONT).end();			\
		IT != IT##_end;							\
		IT++)

// const iterator for loop
#define citfor(TYPE,CONT,IT)					\
	for(TYPE::const_iterator			\
			IT = (CONT).begin(),				\
			IT##_end = (CONT).end();			\
		IT != IT##_end;							\
		IT++)

#endif

#define SYS(STMT) {int rc = STMT; if(rc == -1) perror(#STMT);}
#define SYSTEM(cmd) {int rc = system(cmd); if(rc != 0) {fprintf(stderr, "Failed executing command '%s'\n", cmd); exit(1);}}

#define PANIC() { fprintf(stderr, "PANIC! [%s:%d]\n", __FILE__, __LINE__); abort(); }
#define ERR(MSG...) {fprintf(stderr, MSG); fprintf(stderr, "\n"); exit(1);}
#define REQUIRE( STMT ) if( !(STMT) ) {ERR("Required statement failed: %s", #STMT);}
#define ERRIF( STMT, MSG... ) if( STMT ) ERR(MSG)

#define WARN_ONCE(msg) {static bool __warned = false; if(!__warned) {fprintf(stderr, "%s\n", msg); __warned = true;}}
