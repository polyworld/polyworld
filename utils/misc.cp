/********************************************************************/
/* PolyWorld:  An Artificial Life Ecological Simulator              */
/* by Larry Yaeger                                                  */
/* Copyright Apple Computer 1990,1991,1992                          */
/********************************************************************/

// misc.cp: miscellaneous useful short procedures

// Self
#include "misc.h"

// System
#include <mach/mach_time.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/resource.h>

char* concat(const char* s1, const char* s2)
{
    char* s = new char[strlen(s1)+strlen(s2)+1];
    strcpy(s,s1);
    strcat(s,s2);
    return s;
}


char* concat(const char* s1, const char* s2, const char* s3)
{
    char* s = new char[strlen(s1)+strlen(s2)+strlen(s3)+1];
    strcpy(s,s1);
    strcat(s,s2);
    strcat(s,s3);
    return s;
}


char* concat(const char* s1, const char* s2, const char* s3, const char* s4)
{
    char* s = new char[strlen(s1)+strlen(s2)+strlen(s3)+strlen(s4)+1];
    strcpy(s,s1);
    strcat(s,s2);
    strcat(s,s3);
    strcat(s,s4);
    return s;
}


char* concat(const char* s1, const char* s2, const char* s3, const char* s4, const char* s5)
{
    char* s= new char[strlen(s1)+strlen(s2)+strlen(s3)+strlen(s4)+strlen(s5)+1];
    strcpy(s,s1);
    strcat(s,s2);
    strcat(s,s3);
    strcat(s,s4);
    strcat(s,s5);
    return s;
}

char* concat(const char* s1, const char* s2, const char* s3, const char* s4, const char* s5, const char* s6)
{
    char* s= new char[strlen(s1)+strlen(s2)+strlen(s3)+strlen(s4)+strlen(s5)+
                      strlen(s6)+1];
    strcpy(s,s1);
    strcat(s,s2);
    strcat(s,s3);
    strcat(s,s4);
    strcat(s,s5);
    strcat(s,s6);
    return s;
}


char* concat(const char* s1, const char* s2, const char* s3, const char* s4, const char* s5, const char* s6, const char* s7)
{
    char* s= new char[strlen(s1)+strlen(s2)+strlen(s3)+strlen(s4)+strlen(s5)+
                      strlen(s6)+strlen(s7)+1];
    strcpy(s,s1);
    strcat(s,s2);
    strcat(s,s3);
    strcat(s,s4);
    strcat(s,s5);
    strcat(s,s6);
    strcat(s,s7);
    return s;
}


char* itoa(long i)
{
    char* b = new char[256];
    sprintf(b,"%ld", i);
    char* a = new char[strlen(b)+1];
    strcpy(a, b);
    delete b;
    return a;
}


char* ftoa(float f)
{
    char* b = new char[256];
    sprintf(b,"%g", f);
    char* a = new char[strlen(b)+1];
    strcpy(a ,b);
    delete b;
    return a;
}


float logistic(float x, float slope)
{
    return (1.0 / (1.0 + exp(-1 * x * slope)));
}

double hirestime( void )
{
    static uint32_t num = 0;
    static uint32_t denom = 0;
    uint64_t now;

    if (denom == 0) {
            struct mach_timebase_info tbi;
            kern_return_t r;
            r = mach_timebase_info(&tbi);
            if (r != KERN_SUCCESS) {
                    abort();
            }
            num = tbi.numer;
            denom = tbi.denom;
    }
    now = mach_absolute_time();
    return (double)(now * (double)num / denom / NSEC_PER_SEC);
}

int SetMaximumFiles( long filecount )
{
    struct rlimit lim;
	
	lim.rlim_cur = lim.rlim_max = (rlim_t) filecount;
	if( setrlimit( RLIMIT_NOFILE, &lim ) == 0 )
		return 0;
	else
		return errno;
}

int GetMaximumFiles( long *filecount )
{
	struct rlimit lim;
	
	if( getrlimit( RLIMIT_NOFILE, &lim ) == 0 )
	{
		*filecount = (long) lim.rlim_max;
		return 0;
	}
	else
		return errno;
}
