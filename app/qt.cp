#include <libc.h>
#include <gl/gl.h>
#include <gl/get.h>
#include <gl/cg2vme.h>
#include <gl/addrs.h>
#include <device.h>
#include <math.h>
#include <string.h>
#include <sys/types.h>
#include <sys/param.h>
#include <ctype.h>
#include <stdarg.h>
#include <stream.h>
#include <stdio.h>
#include <signal.h>
#include "misc.h"

const char progname[] = "QuickTest";

main(int argc,char *argv[])
{
    cout << "Hello" nlf;

    float yucko[-3,2][-3,2];

    short i,j;

    for (i = -2; i < 2; i++)
    {
        for (j = -2; j < 2; j++)
        {
            yucko[i][j] = i+j;
        }
    }

    for (i = -2; i < 2; i++)
    {
        for (j = -2; j < 2; j++)
        {
            cout << "yucko[" << i << "][" << j << "] = " << yucko[i][j] nlf;
        }
    }

    cout << "Goodbye" nlf;
}
