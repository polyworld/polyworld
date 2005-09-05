#ifndef PWTYPES_H
#define PWTYPES_H

class genome;
struct FitStruct;

struct domainstruct
{
    float xleft;
    float xright;
    float xsize;
    long minnumcritters;
    long maxnumcritters;
    long initnumcritters;
	long numberToSeed;
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
	float probabilityOfMutatingSeeds;
    short ifit;
    short jfit;
    FitStruct** fittest;
	int fNumLeastFit;
	int fMaxNumLeastFit;
	int fNumSmited;
	critter** fLeastFit;
};

#endif