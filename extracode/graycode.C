/********************************************************************/
/* PolyWorld:  An Artificial Life Ecological Simulator              */
/* by Larry Yaeger                                                  */
/* Copyright Apple Computer 1990,1991,1992                          */
/********************************************************************/
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
#include <stdio.h>
#include <signal.h>
#include <malloc.h>


#define bit(a,b) (((a)>>(b))&1)

/*/
 * Set NUMBITS to your desired number of bits
 * Set PRINTOUT to 1 (or 0) to see (or not) the bitfields
 * Set MAKEINCLUDE to 1 (or 0) to make (or not) the graycode.h file
 * that defines the binofgray[] array
/*/

#define NUMBITS 8
#define PRINTOUT 1
#define MAKEINCLUDE 1


unsigned long b2g[NUMBITS][NUMBITS];
unsigned long g2b[NUMBITS][NUMBITS];


unsigned long bin2gray(unsigned long binarybyte)
{
    unsigned long i,ii,j,jj,graybit,graybyte;

    graybyte = 0;
    for (i = 0, ii = (NUMBITS-1); i < NUMBITS; i++, ii--)
    {
        graybit = 0;
        for (j = 0, jj = (NUMBITS-1); j < NUMBITS; j++, jj--)
            graybit ^= ((binarybyte>>jj)&1) & b2g[i][j];
        graybyte |= graybit<<ii;
    }
    return graybyte;
}


unsigned long gray2bin(unsigned long graybyte)
{
    unsigned long i,ii,j,jj,binarybit,binarybyte;

    binarybyte = 0;
    for (i = 0, ii = (NUMBITS-1); i < NUMBITS; i++, ii--)
    {
        binarybit = 0;
        for (j = 0, jj = (NUMBITS-1); j < NUMBITS; j++, jj--)
            binarybit ^= ((graybyte>>jj)&1) & g2b[i][j];
        binarybyte |= binarybit<<ii;
    }
    return binarybyte;
}


main(int argc,char *argv[])
{
    unsigned long i,j,binarybit,graybit,binarybyte,graybyte,recoveredbyte;
    unsigned long ii,ilooplimit,jlooplimit;
    char binary[NUMBITS+1];
    char gray[NUMBITS+1];
    FILE *includefile;
    unsigned long *binofgray;
    unsigned long two2numbits;
    char type[16];

    two2numbits = 1<<NUMBITS;
    binofgray = (unsigned long*)malloc((unsigned int)(two2numbits*4));
    if (!binofgray)
        exit(1);

    binary[NUMBITS] = '\0';
    gray[NUMBITS] = '\0';

    for (i = 0; i < NUMBITS; i++)
    {
        for (j = 0; j < NUMBITS; j++)
        {
            b2g[i][j] = 0;
            if (j <= i)
                g2b[i][j] = 1;
            else
                g2b[i][j] = 0;
        }
        b2g[i][i] = 1;
        if (i > 0)
            b2g[i][i-1] = 1;
    }

    if (PRINTOUT)
        printf("Binary to Gray:\n\n");
    if (PRINTOUT)
        printf(" Int    Binary      Gray   (GrayInt)  RecoveredInt\n\n");

    for (ii = 0, binarybyte = 0; ii < two2numbits; ii++, binarybyte++)
    {
        for (i = 0; i <= (NUMBITS-1); i++)
        {
            binarybit = bit(binarybyte,i);
            binary[NUMBITS-1-i] = '0' + (char)(binarybit);
        }

        graybyte = bin2gray(binarybyte);

        binofgray[graybyte] = binarybyte;

        for (i = 0; i <= (NUMBITS-1); i++)
        {
            graybit = bit(graybyte,i);
            gray[NUMBITS-1-i] = '0' + (char)(graybit);
        }

        recoveredbyte = gray2bin(graybyte);

        if (PRINTOUT)
            printf(" %5d   %s   %s    %5d       %5d\n",
                binarybyte,binary,gray,graybyte,recoveredbyte);
    }

    if (MAKEINCLUDE)
    {
        includefile = fopen("graycode.h","w");
        if (NUMBITS <= 8)
            sprintf(type,"unsigned char\0");
        else if (NUMBITS <= 16)
            sprintf(type,"unsigned short\0");
        else
            sprintf(type,"unsigned long\0");
        graybyte = 0;
        ilooplimit = (NUMBITS < 3) ? 1 : (two2numbits/8);
        for (i = 0; i < ilooplimit; i++)
        {
            if (i == 0)
                fprintf(includefile,"%s binofgray[%d] = \n    { ",
                        type,two2numbits);
            else
                fprintf(includefile,"\n      ");

            jlooplimit = (NUMBITS < 3) ? (two2numbits-1) : 7;
            for (j = 0; j < jlooplimit; j++)
                fprintf(includefile,"%5d, ",binofgray[graybyte++]);
            if (i == (ilooplimit-1))
                fprintf(includefile,"%5d  ",binofgray[graybyte++]);
            else
                fprintf(includefile,"%5d, ",binofgray[graybyte++]);
        }
        fprintf(includefile,"};\n");
        fclose(includefile);
    }

    free(binofgray);
}
