#define MOVEMENT2

#include "basicincludes.h"
#include "indexlist.C"
void error(cint i, cchar* c) { if (i > -999) cout << "Error: " << c nlf; }

main(int argc,char *argv[])
{

    cout << "Hello" nlf;

    indexlist list(0,19);
    long index[20];

    for (int i = 0; i < 10; i++)
    {
        index[i] = list.getindex();
        cout << "i, index[i] = " << i cms index[i] nl;
    }
    list.print();
    cout << "*****" nlf;
  
    index[10] = list.getindex();
    for (i = 0; i < 10; i += 2)
        list.freeindex(index[i]);
    for (i = 0; i < 10; i += 2)
        index[i] = list.getindex();
    for (i = 0; i < 11; i++)
    {
        cout << "i, index[i] = " << i cms index[i] nl;
    }
    list.print();
    cout << "*****" nlf;

    index[11] = list.getindex();
    for (i = 0; i < 10; i += 2)
        list.freeindex(index[i]);
    for (i = 8; i >= 0; i -= 2)
        index[i] = list.getindex();
    for (i = 0; i < 12; i++)
    {
        cout << "i, index[i] = " << i cms index[i] nl;
    }
    list.print();
    cout << "*****" nlf;

    index[12] = list.getindex();
    for (i = 8; i >= 0; i -= 2)
        list.freeindex(index[i]);
    for (i = 0; i < 10; i += 2)
        index[i] = list.getindex();
    for (i = 0; i < 13; i++)
    {
        cout << "i, index[i] = " << i cms index[i] nl;
    }
    list.print();
    cout << "*****" nlf;

    cout << "Goodbye" nlf;

    if (1) exit(0);

}
