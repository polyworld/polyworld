/********************************************************************/
/* PolyWorld:  An Artificial Life Ecological Simulator              */
/* by Larry Yaeger                                                  */
/* Copyright Apple Computer 1990,1991,1992                          */
/********************************************************************/

// misc.h: miscellaneous useful defines & short procedures

#ifndef MISC_H
#define MISC_H

#include <math.h>

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

#define forever for (;;)

#define nint(a) (long((a)+(((a)<0.0)?-0.499999999:0.499999999)))

#define interp(x,ylo,yhi) ((ylo)+(x)*((yhi)-(ylo)))

#define rrand(lo,hi) (interp(drand48(),(lo),(hi)))

#define index2(i,j,nj) ((i)*(nj)+(j))
#define index3(i,j,k,nj,nk) ((i)*(nj)*(nk)+(j)*(nk)+(k))
#define index4(i,j,k,l,nj,nk,nl) ((i)*(nj)*(nk)*(nl)+(j)*(nk)*(nl)+(k)*(nl)+(l))

#define getbit(a,b) (((a)>>(b))&1)


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


char* itoa(long i);
char* ftoa(float f);


inline float fmax(float f1, float f2)
    { return (f2>f1) ? f2 : f1; }
inline float fmax(float f1, float f2, float f3)
    { return (f2>f1) ? fmax(f2,f3) : fmax(f1,f3); }
inline float fmax(float f1, float f2, float f3, float f4)
    { return (f2>f1) ? fmax(f2,f3,f4) : fmax(f1,f3,f4); }
inline float fmax(float f1, float f2, float f3, float f4, float f5)
    { return (f2>f1) ? fmax(f2,f3,f4,f5) : fmax(f1,f3,f4,f5); }


inline float fmin(float f1, float f2)
    { return (f2<f1) ? f2 : f1; }
inline float fmin(float f1, float f2, float f3)
    { return (f2<f1) ? fmin(f2,f3) : fmin(f1,f3); }
inline float fmin(float f1, float f2, float f3, float f4)
    { return (f2<f1) ? fmin(f2,f3,f4) : fmin(f1,f3,f4); }
inline float fmin(float f1, float f2, float f3, float f4, float f5)
    { return (f2 < f1) ? fmin(f2, f3, f4, f5) : fmin(f1, f3, f4, f5); }


float logistic(float x, float slope);

#ifndef PI
#define PI M_PI
#endif PI

#define TWOPI 6.28318530717059647602
#define HPI M_PI_2
#define RADTODEG 57.29577951
#define DEGTORAD 0.017453292

#endif

