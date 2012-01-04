/********************************************************************/
/* PolyWorld:  An Artificial Life Ecological Simulator              */
/* by Larry Yaeger                                                  */
/* Copyright Apple Computer 1990,1991,1992                          */
/********************************************************************/
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>


#define bit(a,b) (((a)>>(b))&1)

/*/
 * Set NUMBITS to your desired number of bits
 * Set PRINTOUT to true (or false) to see (or not) the bitfields
 * Set MAKEINCLUDE to true (or false) to make (or not) the graycode.h file
 * that defines the binofgray[] array
/*/

#define NUMBITS 8
#define PRINTOUT true
#define MAKEINCLUDE true


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
    unsigned long two2numbits;
    char type[64];

    two2numbits = 1<<NUMBITS;
	unsigned long binofgray[two2numbits];
	unsigned long grayofbin[two2numbits];

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
		grayofbin[binarybyte] = graybyte;

        for (i = 0; i <= (NUMBITS-1); i++)
        {
            graybit = bit(graybyte,i);
            gray[NUMBITS-1-i] = '0' + (char)(graybit);
        }

        recoveredbyte = gray2bin(graybyte);
		assert( recoveredbyte == binarybyte );

        if (PRINTOUT)
            printf(" %5lu   %s   %s    %5lu       %5lu\n",
                binarybyte,binary,gray,graybyte,recoveredbyte);
    }

	// Do some sanity checks.
	for(i = 0; i < two2numbits; i++)
	{
		assert( binofgray[ grayofbin[i] ] == i );

		unsigned long gray = grayofbin[i];
		unsigned long prevgray = i == 0 ? grayofbin[two2numbits - 1] : grayofbin[i - 1];
		int ntoggled = 0;
		for(int ibit = 0; ibit < NUMBITS; ibit++)
		{
			if( bit(gray, ibit) != bit(prevgray, ibit) )
				ntoggled++;
		}
		assert( ntoggled == 1 );
	}

    if (MAKEINCLUDE)
    {
        includefile = fopen("graybin.h","w");

		fprintf( includefile, "#pragma once\n" );

        if (NUMBITS <= 8)
            sprintf(type,"static unsigned char");
        else if (NUMBITS <= 16)
            sprintf(type,"static unsigned short");
        else
            sprintf(type,"static unsigned long");

        ilooplimit = (NUMBITS < 3) ? 1 : (two2numbits/8);

		// ---
		// --- Generate binofgray[]
		// ---
        graybyte = 0;
        for (i = 0; i < ilooplimit; i++)
        {
            if (i == 0)
                fprintf(includefile,"%s binofgray[%lu] = \n    { ",
                        type,two2numbits);
            else
                fprintf(includefile,"\n      ");

            jlooplimit = (NUMBITS < 3) ? (two2numbits-1) : 7;
            for (j = 0; j < jlooplimit; j++)
                fprintf(includefile,"%5lu, ",binofgray[graybyte++]);
            if (i == (ilooplimit-1))
                fprintf(includefile,"%5lu  ",binofgray[graybyte++]);
            else
                fprintf(includefile,"%5lu, ",binofgray[graybyte++]);
        }
        fprintf(includefile,"};\n\n");

		// ---
		// --- Generate binofgray[]
		// ---
        binarybyte = 0;
        for (i = 0; i < ilooplimit; i++)
        {
            if (i == 0)
                fprintf(includefile,"%s grayofbin[%lu] = \n    { ",
                        type,two2numbits);
            else
                fprintf(includefile,"\n      ");

            jlooplimit = (NUMBITS < 3) ? (two2numbits-1) : 7;
            for (j = 0; j < jlooplimit; j++)
                fprintf(includefile,"%5lu, ",grayofbin[binarybyte++]);
            if (i == (ilooplimit-1))
                fprintf(includefile,"%5lu  ",grayofbin[binarybyte++]);
            else
                fprintf(includefile,"%5lu, ",grayofbin[binarybyte++]);
        }
        fprintf(includefile,"};\n\n");



        fclose(includefile);
    }
}
