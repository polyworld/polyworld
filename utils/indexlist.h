/********************************************************************/
/* PolyWorld:  An Artificial Life Ecological Simulator              */
/* by Larry Yaeger                                                  */
/* Copyright Apple Computer 1990,1991,1992                          */
/********************************************************************/

// indexlist.h: declaration of indexlist classes

#ifndef INDEXLIST_H
#define INDEXLIST_H

// System
#include <stdlib.h>
#include <iostream>

class indexlist
{
public:
    indexlist(long l, long h);
    ~indexlist();
    
	void dump(std::ostream& out);
	void load(std::istream& in);
	
    long getindex();
    void freeindex(long i);
    bool isone(long i);
    
    void print(long loindex, long hiindex);
    void print() { print(lo,hi); }

private:
    long lo;
    long hi;
    long next;
    unsigned char* pind;
    long numbytes;
};

#endif

