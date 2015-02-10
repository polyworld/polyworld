/********************************************************************/
/* PolyWorld:  An Artificial Life Ecological Simulator              */
/* by Larry Yaeger                                                  */
/* Copyright Apple Computer 1990,1991,1992                          */
/********************************************************************/
// dumpload.C - these routines dump and load the state of the world

#include "agent.h"
#include "error.h"
#include "graphics.h"
#include "Simulation.h"

extern Domain domains[MAXDOMAINS];

extern short moncritrank;
extern short moncritrankold;
extern bool tracking;
extern bool alternatebraindisplay;
extern short overheadrank;
extern short overheadrankold;

extern genome** fittest;
extern float* fitness;
extern short numfit;
extern short ifit;
extern short jfit;

extern chartwindow* bornwin;
extern chartwindow* fitwin;
extern chartwindow* fewin;
extern chartwindow* popwin;
extern binchartwindow* gswin;

void dump()
{
    filebuf fb;

    if (fb.open("pw.dump",output) == 0)
    {
        error(1,"unable to dump state to \"pw.dump\" file");
        return;
    }

    ostream out(&fb);
    char version[16];
    sprintf(version,"%s","pw1");

    out << version nl;
    out << age nl;
    out << camangle nl;
    out << numcreated nl;
    out << numcreatedrandom nl;
    out << numcreated1fit nl;
    out << numcreated2fit nl;
    out << numdied nl;
    out << numdiedage nl;
    out << numdiedenergy nl;
    out << numdiedfight nl;
    out << numdiededge nl;
    out << numborn nl;
    out << numfights nl;
    out << miscnobirth nl;
    out << lastcreate nl;
    out << maxgapcreate nl;
    out << numbornsincecreated nl;

    out << moncritrank nl;
    out << moncritrankold nl;
    out << tracking nl;
    out << alternatebraindisplay nl;
    out << overheadrank nl;
    out << overheadrankold nl;
    out << genesepmax nl;
    out << genesepmin nl;
    out << genesepavg nl;
    out << maxfitness nl;
    out << avgfitness nl;
    out << avgfoodenergyin nl;
    out << avgfoodenergyout nl;
    out << totfoodenergyin nl;
    out << totfoodenergyout nl;

    agentdump(out);

    out << xsortedfood.count() nl;
    food *f = NULL;
    xsortedfood.reset();
    while (xsortedfood.next(f))
        f->dump(out);

    out << numfit nl;
    out << ifit nl;
    out << jfit nl;
    for (register short i = 0; i < numfit; i++)
    {
        out << fitness[i] nl;
        fittest[i]->dump(out);
    }

    out << numdomains nl;
    for (register short id = 0; id < numdomains; id++)
    {
        out << domains[id].numagents nl;
        out << domains[id].numcreated nl;
        out << domains[id].numborn nl;
        out << domains[id].numbornsincecreated nl;
        out << domains[id].numdied nl;
        out << domains[id].lastcreate nl;
        out << domains[id].maxgapcreate nl;
        out << domains[id].foodCount nl;
        out << domains[id].ifit nl;
        out << domains[id].jfit nl;

        short numfitid = 0;
        if (domains[id].fittest)
        {
            for (i = 0; i < numfit; i++)
            {
                if (domains[id].fittest[i])
                    numfitid++;
                else
                    break;
            }
            out << numfitid nl;
            for (i = 0; i < numfitid; i++)
            {
                out << domains[id].fitness[i] nl;
                domains[id].fittest[i]->dump(out);
            }
        }
        else
            out << numfitid nl;
    }

    bornwin->dump(out);
    fitwin->dump(out);
    fewin->dump(out);
    popwin->dump(out);
    gswin->dump(out);

    out.flush();
    fb.close();
}


void load()
{
    filebuf fb;

    if (fb.open("pw.dump",input) == 0)
    {
        error(2,"unable to load state from file \"pw.dump\"");
        exit(0);
    }

    istream in(&fb);
    char version[16];

    in >> version;
    if (strcmp(version,"pw1"))
        error(2,"dump/load file is wrong version (\"",version,
              "\" should be \"pw1\")");

    in >> age;
    in >> camangle;
    in >> numcreated;
    in >> numcreatedrandom;
    in >> numcreated1fit;
    in >> numcreated2fit;
    in >> numdied;
    in >> numdiedage;
    in >> numdiedenergy;
    in >> numdiedfight;
    in >> numdiededge;
    in >> numborn;
    in >> numfights;
    in >> miscnobirth;
    in >> lastcreate;
    in >> maxgapcreate;
    in >> numbornsincecreated;

    in >> moncritrank;
    in >> moncritrankold;
    in >> tracking;
    in >> alternatebraindisplay;
    in >> overheadrank;
    in >> overheadrankold;
    in >> genesepmax;
    in >> genesepmin;
    in >> genesepavg;
    in >> maxfitness;
    in >> avgfitness;
    in >> avgfoodenergyin;
    in >> avgfoodenergyout;
    in >> totfoodenergyin;
    in >> totfoodenergyout;

    agentload(in);

    food* f = NULL;
    if (xsortedfood.count())
        error(2,"xsortedfood list not empty during load");
    long foodCount = 0;
    in >> foodCount;
    for (register long i = 0; i < foodCount; i++)
    {
        f = new food;
        f->load(in);
        xsortedfood.append(f);
        worldstage.addobject(f);
    }

    short newnumfit = 0;
    in >> newnumfit;
    in >> ifit;
    in >> jfit;
    if (newnumfit != numfit)
    {
        char errmsg[256];
        sprintf(errmsg,"attempted to change numfit from %d to %d",
            numfit,newnumfit);
        error(2,errmsg);
    }
    for (i = 0; i < numfit; i++)
    {
        in >> fitness[i];
        fittest[i]->load(in);
    }

    short newnumdomains;
    in >> newnumdomains;
    if (newnumdomains > MAXDOMAINS)
	error(2,"while loading dump file, numdomains > MAXDOMAINS");
    if (newnumdomains != numdomains)
        error(2,"numdomains different in worldfile and dumpfile");
    for (register short id = 0; id < numdomains; id++)
    {
        in >> domains[id].numagents;
        in >> domains[id].numcreated;
        in >> domains[id].numborn;
        in >> domains[id].numbornsincecreated;
        in >> domains[id].numdied;
        in >> domains[id].lastcreate;
        in >> domains[id].maxgapcreate;
        in >> domains[id].foodCount;
        in >> domains[id].ifit;
        in >> domains[id].jfit;

        if (!domains[id].fittest)
            error(2,"while loading dump file, domains[id].fittest == NULL");
        short numfitid;
        in >> numfitid;
        if (numfitid > numfit)
            error(2,"while loading dump file, numfitid > numfit");
        for (i = 0; i < numfitid; i++)
        {
            in >> domains[id].fitness[i];
            domains[id].fittest[i]->load(in);
        }
    }

    bornwin->load(in);
    fitwin->load(in);
    fewin->load(in);
    popwin->load(in);
    gswin->load(in);

    if (graphics)
    {
        bornwin->refresh();
        fitwin->refresh();
        fewin->refresh();
        popwin->refresh();
        gswin->refresh();
    }

    fb.close();
}
