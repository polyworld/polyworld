#ifndef PWTYPES_H
#define PWTYPES_H

class genome;

struct domainstruct
{
    float xleft;
    float xright;
    float xsize;
    long minnumcritters;
    long maxnumcritters;
    long initnumcritters;
    long minfoodcount;
    long maxfoodcount;
    long maxfoodgrown;
    long initfoodcount;
    long numcritters;
    long numcreated;
    long numborn;
    long numbornsincecreated;
    long numdied;
    long lastcreate;
    long maxgapcreate;
    long foodcount;
    short ifit;
    short jfit;
    genome** fittest;
    float* fitness;
	int fNumLeastFit;
	int fMaxNumLeastFit;
	int fNumSmited;
	critter** fLeastFit;
};

#endif