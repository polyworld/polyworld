/********************************************************************/
/* PolyWorld:  An Artificial Life Ecological Simulator              */
/* by Larry Yaeger                                                  */
/* Copyright Apple Computer 1990,1991,1992                          */
/********************************************************************/

// misc.cp: miscellaneous useful short procedures

// Self
#include "misc.h"

// System

#ifdef __APPLE__
	#include <mach/mach_time.h>
#endif

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <set>
#include <string>
#include <vector>
#include <iostream>

using namespace std;

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


string operator+( const char *a, const std::string &b )
{
	return string(a) + b;
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

float biasedLogistic(float x, float bias, float slope)
{
    return (1.0 / (1.0 + exp(-1 * (x+bias) * slope)));
}


float gaussian( float x, float mean, float variance )
{
    return( exp( -(x-mean)*(x-mean) / variance ) );
}

bool exists( const string &path )
{
	struct stat _stat;
	return 0 == stat(path.c_str(), &_stat);
}

string dirname( const string &path )
{
    size_t end = path.length() - 1;

    if( path[end] == '/' )
        end--;

    end = path.rfind( '/', end );

    return path.substr( 0, end );
}

void makeDirs( const string &path )
{
#pragma omp critical(alreadyMade)
	{
		static set<string> alreadyMade;

		if( alreadyMade.find(path) == alreadyMade.end() )
		{
			char cmd[1024];
			sprintf( cmd, "mkdir -p %s", path.c_str() );
			if( 0 != system(cmd) )
				exit( 1 );

			alreadyMade.insert( path );
		}
	}
}

void makeParentDir( const string &path )
{
	makeDirs( dirname(path) );
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


vector<string> split( const string& str, const string& delimiters )
{
	vector<string> parts;
	
    // Skip delimiters at beginning.
    string::size_type lastPos = str.find_first_not_of( delimiters, 0 );
    // Find first delimiter after non-delimiters.
    string::size_type pos     = str.find_first_of( delimiters, lastPos );

    while (string::npos != pos || string::npos != lastPos)
    {
        // Found a token, add it to the vector.
        parts.push_back( str.substr( lastPos, pos - lastPos ) );
        // Skip delimiters.
        lastPos = str.find_first_not_of( delimiters, pos );
        // Find next delimiter after non-delimiters
        pos = str.find_first_of( delimiters, lastPos );
    }
    
    return( parts );
}

// int main( int argc, char** argv )
// {
// 	string sentence( "Now is the time for all good men" );
// 	
// 	vector<string> parts = split( sentence );
// 	
// 	itfor( vector<string>, parts, it )
// 	{
// 		cout << *it << endl;
// 	}
// }
