#ifndef GWINDOW_C
#define GWINDOW_C
/********************************************************************/
/* PolyWorld:  An Artificial Life Ecological Simulator              */
/* by Larry Yaeger                                                  */
/* Copyright Apple Computer 1990,1991,1992                          */
/********************************************************************/

// gwindow.C: implementation of gwindow classes


//#define WDELAY
//#define WDELAYLENGTH 2
//#define WHEADERS
//#define WTRAILERS
//#define ANNOUNCECLEARS

#include "graphics.h"

long gwindow::windowsever;
long gwindow::windowsactive;
long gwindow::windowseveropened;
long gwindow::windowsopen;

#ifdef ANNOUNCECLEARS
extern Boolean announceclears;
#endif ANNOUNCECLEARS

inline void matcpy(Matrix mleft,Matrix mright)
{
    for (int i = 0; i < 15; i++)
        mleft[0][i] = mright[0][i];
}
long matcmp(Matrix m1,Matrix m2)
{
    long differences = 0;
    for (int i = 0; i < 15; i++)
        if (m1[0][i] != m2[0][i])
            differences++;
    return differences;
}

/*
extern Matrix mastermatrix;
void makemastermatrix(Matrix m) { matcpy(mastermatrix,m); }
void printmatrix(Matrix m)
{
    for (int i = 0; i < 4; i++)
    {
        for (int j = 0; j < 4; j++)
            cout sp2 m[i][j];
        cout nl;
    }
    cout.flush();
}
void checkprojectionmatrix(cchar* msg)
{
    Matrix viewingmatrix;
    Matrix projectionmatrix;
    getmatrix(viewingmatrix);
    mmode(MPROJECTION);
    getmatrix(projectionmatrix);
    mmode(MVIEWING);
    loadmatrix(viewingmatrix);  // put things back the way they were (but what about stack depth?)
    if (matcmp(projectionmatrix,mastermatrix))  // then they are different
    {
        cout << "projection matrix has changed by " << msg nl;
        cout << "original =" nl;
        printmatrix(mastermatrix);
        cout << "new =" nl;
        printmatrix(projectionmatrix);
    }
}
*/


void gwindow::init()
{
#ifdef WHEADERS
    cout << "entering gwindow::init()" nlf;
    cout << "constructing " << ++windowsactive << " active window, of "
         << ++windowsever << " ever" nlf;
#endif WHEADERS
    visible = TRUE;
    minsizeset = FALSE;
    maxsizeset = FALSE;
    keepaspectset = FALSE;
    prefsizeset = FALSE;
    prefposset = FALSE;
    border = FALSE;
    framewidth = 3;  // set to 0 to turn off frame
    titlebar = TRUE;  // if there's a border, of course (THIS DOESN'T WORK)
    name = NULL;
    id = 0;
    saveid = 0;
    zbuffered = TRUE;
    doublebuffered = TRUE;
    drawbuffer = 'd';  // 'f' = front, 'b' = back, anything else = default
    pwinscene = NULL;
    bg.r = bg.g = bg.b = 0.;
    frm.r = frm.g = frm.b = .8;
#ifdef WTRAILERS
    cout << "exiting gwindow::init()" nlf;
#endif WTRAILERS
#ifdef WDELAY
    sleep(WDELAYLENGTH);
#endif WDELAY
}


void gwindow::draw()
{
#ifdef WHEADERS
    cout << "entering gwindow::draw() for window " << gettitle() nlf;
    cout << "about to draw window id = " << id nlf;
#endif WHEADERS
    if (isopen())
    {
        makecurrentwindow();
        justdraw();
        if (doublebuffered && (drawbuffer != 'f'))
            justswapbuffers();
        restorecurrentwindow();
    } // if (isopen())
#ifdef WTRAILERS
    cout << "exiting gwindow::draw() for window " << gettitle() nlf;
#endif WTRAILERS
#ifdef WDELAY
    sleep(WDELAYLENGTH);
#endif WDELAY
}


void gwindow::draw(const frustumXZ* fxz)
{
#ifdef WHEADERS
    cout << "entering gwindow::draw() for window " << gettitle() nlf;
    cout << "about to draw window id = " << id nlf;
#endif WHEADERS
    if (isopen())
    {
        makecurrentwindow();
        justdraw(fxz);
        if (doublebuffered && (drawbuffer != 'f'))
            justswapbuffers();
        restorecurrentwindow();
    } // if (isopen())
#ifdef WTRAILERS
    cout << "exiting gwindow::draw() for window " << gettitle() nlf;
#endif WTRAILERS
#ifdef WDELAY
    sleep(WDELAYLENGTH);
#endif WDELAY
}


void gwindow::drawnoswap()
{
#ifdef WHEADERS
    cout << "entering gwindow::drawnoswap() for window " << gettitle() nlf;
#endif WHEADERS
    if (isopen())
    {
        makecurrentwindow();
        justdraw();
        restorecurrentwindow();
    } // if (isopen())
#ifdef WTRAILERS
    cout << "exiting gwindow::drawnoswap() for window " << gettitle() nlf;
#endif WTRAILERS
#ifdef WDELAY
    sleep(WDELAYLENGTH);
#endif WDELAY
}


void gwindow::drawnoswap(const frustumXZ* fxz)
{
#ifdef WHEADERS
    cout << "entering gwindow::drawnoswap() for window " << gettitle() nlf;
#endif WHEADERS
    if (isopen())
    {
        makecurrentwindow();
        justdraw(fxz);
        restorecurrentwindow();
    } // if (isopen())
#ifdef WTRAILERS
    cout << "exiting gwindow::drawnoswap() for window " << gettitle() nlf;
#endif WTRAILERS
#ifdef WDELAY
    sleep(WDELAYLENGTH);
#endif WDELAY
}


inline void gwindow::justdraw()
{
#ifdef WHEADERS
    cout << "entering gwindow::justdraw() for window " << gettitle() nlf;
#endif WHEADERS
//  reshapeviewport(); // NEED FLAG THAT SAYS WHEN THIS IS NEEDED!!!
//  if (framewidth) adjustscrmask();
#ifdef ANNOUNCECLEARS
    if (announceclears)
        error(0,"justdraw(): about to czclear '",name,"' window");
#endif ANNOUNCECLEARS
    czclear(lcol(bg.r,bg.g,bg.b),zbfar);
    if (pwinscene) pwinscene->draw();
#ifdef WTRAILERS
    cout << "exiting gwindow::justdraw() for window " << gettitle() nlf;
#endif WTRAILERS
#ifdef WDELAY
    sleep(WDELAYLENGTH);
#endif WDELAY
}


inline void gwindow::justdraw(const frustumXZ* fxz)
{
#ifdef WHEADERS
    cout << "entering gwindow::justdraw() for window " << gettitle() nlf;
#endif WHEADERS
//  reshapeviewport(); // NEED FLAG THAT SAYS WHEN THIS IS NEEDED!!!
//  if (framewidth) adjustscrmask();
#ifdef ANNOUNCECLEARS
    if (announceclears)
        error(0,"justdraw(fxz): about to czclear '",name,"' window");
#endif ANNOUNCECLEARS
    czclear(lcol(bg.r,bg.g,bg.b),zbfar);
    if (pwinscene) pwinscene->draw(fxz);
#ifdef WTRAILERS
    cout << "exiting gwindow::justdraw() for window " << gettitle() nlf;
#endif WTRAILERS
#ifdef WDELAY
    sleep(WDELAYLENGTH);
#endif WDELAY
}


void gwindow::refresh()
{
    if (isopen())
    {
        makecurrentwindow();
        pop();
        resetscrmask();
        doubleclearcz();
        if (framewidth) drawframe();
    }
}


void gwindow::print()
{
    cout << "\nFor window titled: \"" << name << "\"...\n";
    if (isopen())
    {
	cout << "  the window is open with id = " << id << "\n";
        long savewin = winget();
        winset(id);
        lpt origin; getorigin(&origin.x,&origin.y);
        lpt size; getsize(&size.x,&size.y);
        winset(savewin);
	cout << "  its origin is at position (x,y) = (" 
             << origin.x << "," << origin.y << ")\n";
        cout << "  its size (x,y) = (" << size.x << "," << size.y << ")\n";
    }
    else
	cout << "  the window is NOT open\n";
    cout << "  it is " << (visible ? "" : "NOT ") << "supposed to be visible\n";
    if (minsizeset)
	cout << "  minsize (x,y) set to (" << minsiz.x << ","
                                           << minsiz.y << ")\n";
    else
	cout << "  minsize is not set; default (x,y) = (80,40)\n";
    if (maxsizeset)
	cout << "  maxsize (x,y) set to (" << maxsiz.x << ","
					   << maxsiz.y << ")\n";
    else
        cout << "  maxsize is not set; default (x,y) = (1280,1024)\n";
    if (keepaspectset)
	cout << "  keepaspect (x,y) set to (" << aspect.x << ","
                                              << aspect.y << ")\n";
    else
	cout << "  keepaspect is not set\n";
    if (prefsizeset)
	cout << "  prefsize (x,y) set to (" << prefsiz.x << ","
					    << prefsiz.y << ")\n";
    else
	cout << "  prefsize is not set\n";
    if (prefposset)
	cout << "  prefposition (x1,x2,y1,y2) set to (" 
	     << prefpos1.x << "," << prefpos2.x << ","
             << prefpos1.y << "," << prefpos2.y << ")\n";
    else
	cout << "  prefposition is not set\n";
    if (border)
	cout << "  there is a border\n";
    else
	cout << "  there is no border\n";
    if (framewidth)
	cout << "  there is a frame of width " << framewidth
             << " colored (" << frm.r cm frm.g cm frm.b pnl;
    else
	cout << "  there is no frame\n";
    cout << "  its background color is (r,g,b) = ("
         << bg.r cm bg.g cm bg.b pnl;
    if (zbuffered)
        cout << "  it is zbuffered\n";
    else
        cout << "  it is not zbuffered\n";
    if (doublebuffered)
        cout << "  it is doublebuffered\n";
    else
        cout << "  it is singlebuffered\n";
    switch (drawbuffer)
    {
    case 'f':
        cout << "  it will draw to the front buffer\n";
        break;
    case 'b':
        cout << "  it will draw to the back buffer\n";
        break;
    default:
        cout << "  it will draw to the default buffer\n";
    }
    if (pwinscene)
    {
        cout << "  it will draw the scene pointed to by " << pwinscene
             << ", whose characteristics are:" nl;
        pwinscene->print();
    }
    else
        cout << "  no scene is currently attached\n";
    cout.flush();
}


void gwindow::setminsize(const long x, const long y)
{
    minsiz.x = x; minsiz.y = y; minsizeset = TRUE;
    if (isopen()) applyconstraints();  // window already open, so apply now
}


void gwindow::setmaxsize(const long x, const long y)
{
    maxsiz.x = x; maxsiz.y = y; maxsizeset = TRUE;
    if (isopen()) applyconstraints();  // window already open, so apply now
}


void gwindow::setkeepaspect(const long x, const long y)
{
    aspect.x = x; aspect.y = y; keepaspectset = TRUE;
    if (isopen()) applyconstraints();  // window already open, so apply now
}


void gwindow::setprefsize(const long x, const long y)
{
    if (prefposset) // avoid possible conflict between prefsize & prefposition
        setprefposition(prefpos1.x,prefpos1.x+x-1,
                        prefpos1.y,prefpos1.y+y-1);
    else
    {
        prefsiz.x = x; prefsiz.y = y; prefsizeset = TRUE;
    }
    if (isopen()) applyconstraints();  // window already open, so apply now
}


void gwindow::setprefposition(const long x1, const long x2,
                             const long y1, const long y2)
{
    prefpos1.x = x1; prefpos1.y = y1;
    prefpos2.x = x2; prefpos2.y = y2; prefposset = TRUE;
    if (isopen()) applyconstraints();  // window already open, so apply now
}


void gwindow::setborder(const Boolean b)
{
    border = b;
    if (isopen()) applyconstraints();  // window already open, so apply now
}


void gwindow::setframewidth(const short width)
{
    framewidth = width;
    if (isopen())  // window already open, so apply now
    {
        if (framewidth)
            drawframe();
        else
            resetscrmask();
    }
}


void gwindow::setframecolor(cfloat* c)
{
    frm.r = c[0]; frm.g = c[1]; frm.b = c[2];
    if (framewidth && isopen()) drawframe();  // window already open, apply now
}


void gwindow::setframecolor(cfloat r, cfloat g, cfloat b)
{
    frm.r = r; frm.g = g; frm.b = b;
    if (framewidth && isopen()) drawframe();  // window already open, apply now
}


void gwindow::drawframe()
{
#ifdef WHEADERS
    cout << "entering gwindow::drawframe() for window " << gettitle() nlf;
    cout << "  framewidth = " << framewidth nlf;
    cout << "  framecolor = (" << frm.r cm frm.g cm frm.b pnlf;
#endif WHEADERS
    long x;  long y;
    getsize(&x,&y);
    if (perspectiveset())
        ortho2(0.0,float(x),0.0,float(y));
//  cout << "size = " << x cms y nlf;
//  sleep(1);
    scrmask(0,short(x-1),0,short(y-1));
//  cout << "scrmask set to 0, " << (x-1) cms 0 cms (y-1) nlf;
//  sleep(1);
    short savelwidth = short(getlwidth());
//  cout << "old linewidth = " << savelwidth nlf;
//  sleep(1);
    linewidth(framewidth);
//  cout << "new linewidth = " << framewidth nlf;
//  sleep(1);
    makecurrentcolor(&frm.r);
//  cout << "current color set to frame color = ("
//       << frm.r cm frm.g cm frm.b pnlf;
//  sleep(1);
    if (doublebuffered) frontbuffer(TRUE);
//  cout << "drawing to frontbuffer enabled" nlf;
//  sleep(1);
//  sboxi(framewidth/2,framewidth/2,x-framewidth/2-1,y-framewidth/2-1);
    recti(framewidth/2,framewidth/2,x-framewidth/2-1,y-framewidth/2-1);
//  recti(10,10,20,20); recti(30,10,40,20); recti(50,10,60,20);
//  cout << "frame drawn at (" << (framewidth/2) cm (framewidth/2)
//       cm (x-framewidth/2-1) cm (y-framewidth/2-1) pnlf;
//  sleep(1);
    if (doublebuffered) frontbuffer(FALSE);
//  cout << "drawing to frontbuffer disabled" nlf;
//  sleep(1);
    restorecurrentcolor();
    linewidth(savelwidth);
    adjustscrmask(x,y);
    if (perspectiveset())
        useperspective();
#ifdef WTRAILERS
    cout << "exiting gwindow::drawframe() for window " << gettitle() nlf;
#endif WTRAILERS
#ifdef WDELAY
    sleep(WDELAYLENGTH);
#endif WDELAY
}


void gwindow::adjustscrmask()
{
    long x;  long y;
    getsize(&x,&y);
    adjustscrmask(x,y);
}


void gwindow::adjustscrmask(long x, long y)
{
#ifdef WHEADERS
    cout << "entering gwindow::adjustscrmask() for window " << gettitle() nlf;
    cout << "  framewidth = " << framewidth nlf;
    cout << "  x, y = " << x cms y nlf;
#endif WHEADERS
    scrmask(framewidth,short(x-framewidth-1),framewidth,short(y-framewidth-1));
#ifdef WTRAILERS
    cout << "exiting gwindow::adjustscrmask() for window " << gettitle() nlf;
#endif WTRAILERS
#ifdef WDELAY
    sleep(WDELAYLENGTH);
#endif WDELAY
}


void gwindow::resetscrmask()
{
    long x;  long y;
    getsize(&x,&y);
    scrmask(0,short(x-1),0,short(y-1));
}


void gwindow::settitle(const char* pwtitle)
{
    name = pwtitle;
    if (isopen())  // window already open, so apply now
    {
        makecurrentwindow();
        wintitle((String)name);
        restorecurrentwindow();
    }
}


void gwindow::setposition(const long x1, const long x2,
                      const long y1, const long y2)
{
    if (!isopen())  // window not open yet, so convert to a setprefposition
    {
        setprefposition(x1,x2,y1,y2);
    }
    else  // window already open, so apply now
    {
        makecurrentwindow();
        winposition(x1,x2,y1,y2);
        restorecurrentwindow();
    }
}


void gwindow::move(const long x, const long y)
{
    if (isopen("move"))
    {
        makecurrentwindow();
        winmove(x,y);
        restorecurrentwindow();
    }
}


void gwindow::applyconstraints()
{
#ifdef WHEADERS
    cout << "entering gwindow::applyconstraints() for window " << gettitle() nlf;
#endif WHEADERS
    if (isopen())  // only if the window is already open
    {
        makecurrentwindow();
        winconstraints();  // this sets everything back to their defaults
    }
    if (minsizeset) minsize(minsiz.x,minsiz.y);
    if (maxsizeset) maxsize(maxsiz.x,maxsiz.y);
    if (keepaspectset) keepaspect(aspect.x,aspect.y);
    if (prefsizeset) prefsize(prefsiz.x,prefsiz.y);
    if (prefposset) prefposition(prefpos1.x,prefpos2.x,prefpos1.y,prefpos2.y);
    if (!border) noborder();
    if (isopen())  // only if the window is already open
    {
        winconstraints();  // this applies all our new constraints
        restorecurrentwindow();
    }
#ifdef WTRAILERS
    cout << "exiting gwindow::applyconstraints() for window " << gettitle() nlf;
#endif WTRAILERS
#ifdef WDELAY
    sleep(WDELAYLENGTH);
#endif WDELAY
}


void gwindow::open()
{
#ifdef WHEADERS
    cout << "entering gwindow::open() for window " << gettitle() nlf;
    cout << "opening " << ++windowsopen << " window, of "
         << ++windowseveropened << " ever opened" nlf;
#endif WHEADERS
    if (isopen())  // error - open called for a window that's already open
    {
        char errmsg[256];
        strcpy(errmsg,"trying to open a window (");
        strcat(errmsg,name);
        strcat(errmsg,") that is already open");
        error(1,errmsg);
    }
    else
    {
        applyconstraints();
        id = winopen((String)name);
        if (id == -1)  // uh oh, failed to open the window
        {
            char errmsg[256];
            strcpy(errmsg,"unable to open a window (");
            strcat(errmsg,name);
            strcat(errmsg,")");
            error(2,errmsg);  // will cause an exit(2)
        }
        if (!titlebar) wintitle("");  // (THIS DOESN'T WORK)
        if (!visible)
        {
            winpush();
//          sleep(1);  // to allow window manager to notice in time to not draw!
        }
        RGBmode();  // all my windows are going to be RGB, not index-mapped
        Matrix m; m[0][0] = 1.;  // set for c++ compiler only!!
        getmatrix(m);
        mmode(MPROJECTION);
        loadmatrix(m);
        mmode(MVIEWING);  // always want to support lighting calculations
        loadmatrix(identmat);
        zbuffer(zbuffered);
        lsetdepth(zbnear,zbfar);
        if (zbnear > zbfar) zfunction(ZF_GEQUAL);
        setdoublebuffer(doublebuffered);  // invokes gconfig & setdrawbuffer
        czclear(lcol(bg.r,bg.g,bg.b),zbfar);
        if (doublebuffered)
            { justswapbuffers(); czclear(lcol(bg.r,bg.g,bg.b),zbfar); }
        if (framewidth) drawframe();
#ifdef WTRAILERS
    cout << "exiting gwindow::open() for window " << gettitle() nlf;
#endif WTRAILERS
#ifdef WDELAY
    sleep(WDELAYLENGTH);
#endif WDELAY
    }
}


void gwindow::setzbuffer(const Boolean zb)
{
    zbuffered = zb;
    if (isopen())  // window is open, so apply it now
        zbuffer(zb);
}


void gwindow::setdoublebuffer(const Boolean db)
{
    doublebuffered = db;
    if (isopen())  // window is open, so apply it now
    {
        if (db)
            doublebuffer();
        else
            singlebuffer();
        gconfig();
        setdrawbuffer(drawbuffer);  // because gconfig calls frontbuffer(FALSE)
     }
}


void gwindow::setdrawbuffer(const char b)
{
    // 'f' = front, 'b' = back, anything else = default
    if ((drawbuffer == 'b') && (!doublebuffered))
        error(0,"trying to draw to backbuffer of a singlebuffered window");
    drawbuffer = b;
    if (isopen())  // window is open, so apply now
    {
        switch (b)
        {
        case 'f':
            frontbuffer(TRUE);
            backbuffer(FALSE);
            break;
        case 'b':
            frontbuffer(FALSE);
            backbuffer(TRUE);
            break;
        }
    }
}


void gwindow::clearc()
{
#ifdef WHEADERS
    cout << "entering gwindow::clearc() for window " << gettitle() nlf;
#endif WHEADERS
    if (isopen("clearc"))
    {
        makecurrentwindow();
        makecurrentcolor();
#ifdef ANNOUNCECLEARS
        if (announceclears)
            error(0,"clearc(): about to clear '",name,"' window");
#endif ANNOUNCECLEARS
        clear();
//      if (framewidth) drawframe();
        restorecurrentwindow();
        restorecurrentcolor();
    }
#ifdef WTRAILERS
    cout << "exiting gwindow::clearc() for window " << gettitle() nlf;
#endif WTRAILERS
#ifdef WDELAY
    sleep(WDELAYLENGTH);
#endif WDELAY
}


void gwindow::clearcs()
{
#ifdef WHEADERS
    cout << "entering gwindow::clearcs() for window " << gettitle() nlf;
#endif WHEADERS
    if (isopen("clearcs"))
    {
        makecurrentwindow();
        makecurrentcolor();
#ifdef ANNOUNCECLEARS
        if (announceclears)
            error(0,"clearcs(): about to clear '",name,"' window");
#endif ANNOUNCECLEARS
        clear();
//      if (framewidth) drawframe();
        justswapbuffers();  // only difference from clearc()
        restorecurrentwindow();
        restorecurrentcolor();
    }
#ifdef WTRAILERS
    cout << "exiting gwindow::clearcs() for window " << gettitle() nlf;
#endif WTRAILERS
#ifdef WDELAY
    sleep(WDELAYLENGTH);
#endif WDELAY
}


void gwindow::doubleclearc()
{
#ifdef WHEADERS
    cout << "entering gwindow::doubleclearc() for window " << gettitle() nlf;
#endif WHEADERS
    if (isopen("doubleclearc"))
    {
        makecurrentwindow();
        makecurrentcolor();
#ifdef ANNOUNCECLEARS
        if (announceclears)
            error(0,"doubleclearc(): about to clear '",name,"' window 1st time");
#endif ANNOUNCECLEARS
        clear();
//      if (framewidth) drawframe();
        if (doublebuffered)  // why else would you call this member!?
        {
            justswapbuffers();
#ifdef ANNOUNCECLEARS
        if (announceclears)
            error(0,"doubleclearc(): about to clear '",name,"' window 2nd time");
#endif ANNOUNCECLEARS
            clear();
//          if (framewidth) drawframe();
        }
        restorecurrentwindow();
        restorecurrentcolor();
    }
#ifdef WTRAILERS
    cout << "exiting gwindow::doubleclearc() for window " << gettitle() nlf;
#endif WTRAILERS
#ifdef WDELAY
    sleep(WDELAYLENGTH);
#endif WDELAY
}


void gwindow::doubleclearcz()
{
#ifdef WHEADERS
    cout << "entering gwindow::doubleclearcz() for window " << gettitle() nlf;
#endif WHEADERS
    if (isopen("doubleclearcz"))
    {
        makecurrentwindow();
#ifdef ANNOUNCECLEARS
        if (announceclears)
            error(0,"doubleclearcz(): about to czclear '",name,"' window 1st time");
#endif ANNOUNCECLEARS
        czclear(lcol(bg.r,bg.g,bg.b),zbfar);
        if (doublebuffered)  // why else would you call this member!?
        {
            justswapbuffers();
#ifdef ANNOUNCECLEARS
        if (announceclears)
            error(0,"doubleclearcz(): about to czclear '",name,"' window 2nd time");
#endif ANNOUNCECLEARS
            czclear(lcol(bg.r,bg.g,bg.b),zbfar);
        }
        restorecurrentwindow();
    }
#ifdef WTRAILERS
    cout << "exiting gwindow::doubleclearcz() for window " << gettitle() nlf;
#endif WTRAILERS
#ifdef WDELAY
    sleep(WDELAYLENGTH);
#endif WDELAY
}


void gwindow::settitlebar(const Boolean t) // (THIS DOESN'T WORK)
{
    titlebar = t;
    if (isopen())
    {
        if (t)
        {
            if (name == NULL)
                wintitle(" ");
            else
                wintitle((String)name);
        }
        else
            wintitle("");
    }
}


void gwindow::pop()
{
#ifdef WHEADERS
    cout << "entering gwindow::pop() for window " << gettitle() nlf;
#endif WHEADERS
    if (isopen())
        { makecurrentwindow(); winpop(); restorecurrentwindow(); }
    else
        setvisibility(TRUE);
#ifdef WTRAILERS
    cout << "exiting gwindow::pop() for window " << gettitle() nlf;
#endif WTRAILERS
#ifdef WDELAY
    sleep(WDELAYLENGTH);
#endif DELAY
}


void gwindow::push()
{
#ifdef WHEADERS
    cout << "entering gwindow::push() for window " << gettitle() nlf;
#endif WHEADERS
    if (isopen())
        { makecurrentwindow(); winpush(); restorecurrentwindow(); }
    else
        setvisibility(FALSE);
#ifdef WTRAILERS
    cout << "exiting gwindow::push() for window " << gettitle() nlf;
#endif WTRAILERS
#ifdef WDELAY
    sleep(WDELAYLENGTH);
#endif WDELAY
}


void gwindow::setvisibility(const Boolean vis)
{
#ifdef WHEADERS
    cout << "entering gwindow::setvisibility() for window " << gettitle() nlf;
#endif WHEADERS
    visible = vis;
    if (isopen()) { if (vis) pop(); else push(); }
#ifdef WTRAILERS
    cout << "exiting gwindow::setvisibility() for window " << gettitle() nlf;
#endif WTRAILERS
#ifdef WDELAY
    sleep(WDELAYLENGTH);
#endif WDELAY
}


void gwindow::dumbtext(const char* pc)
{
#ifdef WHEADERS
    cout << "entering gwindow::dumbtext() for window " << gettitle() nlf;
#endif WHEADERS
    if (isopen("dumbtext"))
    {
        makecurrentwindow();
        makecurrentcolor(1.,1.,1.);
        lpt size;
        getsize(&size.x,&size.y);
        long strx = strwidth((String)pc);
//      cout << "in dumbtext, pc = " << pc nlf;
//      cout << "  strwidth = " << strx nlf;
//      cout << "  id = " << id nlf;
//      cout << "  size = " << size.x cm size.y nlf;
        if (strx <= size.x)
        {
            cmov2i((size.x-strx)/2,size.y/2);
            charstr((String)pc);
        }
        else
        {
            char ctemp[256];
            long cheight = getheight() + 2;
            long numlines = (strx/size.x) + 1;
            int charperline = int(strlen(pc) / numlines);
            for (int i = 0; i < numlines; i++)
            {
                strncpy(ctemp,pc+i*charperline,charperline);
                ctemp[charperline] = '\0';
                strx = strwidth(ctemp);
                cmov2i((size.x-strx)/2,(size.y+numlines*cheight)/2-i*cheight);
                charstr(ctemp);
            }
        }
        restorecurrentwindow();
        restorecurrentcolor();
    }
#ifdef WTRAILERS
    cout << "exiting gwindow::dumbtext() for window " << gettitle() nlf;
#endif WTRAILERS
#ifdef WDELAY
    sleep(WDELAYLENGTH);
#endif WDELAY
}


void gwindow::swapbuffers()
{
    makecurrentwindow();
    justswapbuffers();
    restorecurrentwindow();
}


void gscreen::init0()
{
    setvisibility(TRUE);
    setzbuffer(FALSE);
    setdoublebuffer(FALSE);
    setframewidth(0);
//  open();
//  clearc();
}
void gscreen::init()
{
    setprefposition(0,1279,0,1023);
    init0();
}
void gscreen::init(cint width, cint height)
{
    setprefposition(0,width-1,0,height-1);
    init0();
}


void chartwindow::setrange(cshort ic, cfloat valmin, cfloat valmax)
{
    if (valmin < valmax)
    {
        vlo[ic] = valmin;
        vhi[ic] = valmax;
    }
    else if (valmax < valmin)
    {
        vlo[ic] = valmax;
        vhi[ic] = valmin;
    }
    else
    {
        error(1,"attempt to define null range for chart window");
        vlo[ic] = valmin;
        vhi[ic] = valmin+1.;
    }
    dydv[ic] = (yhi - ylo) / (vhi[ic] - vlo[ic]);
}


void chartwindow::setcolor(cshort ic, const colf col)
{
    color[ic].r = col.r;
    color[ic].g = col.g;
    color[ic].b = col.b;
}


void chartwindow::setcolor(cshort ic, cfloat r, cfloat g, cfloat b)
{
    color[ic].r = r;
    color[ic].g = g;
    color[ic].b = b;
}


void chartwindow::refresh()
{
    gwindow::refresh();
    drawaxes();
    plotpoints();
}


void chartwindow::redraw()
{
    doubleclearc();
    drawaxes();
    plotpoints();
}


void chartwindow::dump(ostream& out)
{
    out << numcurves nl;
    out << decimation nl;
    for (register long ic = 0; ic < numcurves; ic++)
    {
        out << numpoints[ic] nl;
        for (register long i = 0; i < numpoints[ic]; i++)
        {
            out << y[(ic*maxpoints)+i] nl;
        }
    }
}


void chartwindow::load(istream& in)
{
    long newnumcurves;
    in >> newnumcurves;
    if (newnumcurves != numcurves)
        error(2,"while loading from dump file, numcurves redefined");
    in >> decimation;
    if (y == NULL)  // first time must allocate space
    {
        y = new long[maxpoints*numcurves];
        if (y == NULL)
        {
            error(2,"insufficient space for chartwindow points during load");
            return;
        }
    }
    for (register long ic = 0; ic < numcurves; ic++)
    {
        in >> numpoints[ic];
        for (register long i = 0; i < numpoints[ic]; i++)
        {
            in >> y[(ic*maxpoints)+i];
        }
    }
}


void chartwindow::addpoint(cshort ic, cfloat val)
{
    long i;
    long j;
    if (y == NULL)  // first time must allocate space
    {
        y = new long[maxpoints*numcurves];
        if (y == NULL)
        {
            error(2,"unable to allocate space for chartwindow points");
            return;
        }
    }
    if (numpoints[ic] == maxpoints)
    {
        // cut the number (and resolution) of points stored in half
        for (long jc = 0; jc < numcurves; jc++)
        {
            for (i=1, j=2; j < min(numpoints[jc],maxpoints); i++, j+=2)
                y[(jc*maxpoints)+i] = y[(jc*maxpoints)+j];
            numpoints[jc] = i;
        }
        decimation++;
        redraw();
    }
    y[(ic*maxpoints)+numpoints[ic]] = (val - vlo[ic]) * dydv[ic]  +  ylo;
    plotpoint(ic,numpoints[ic]+xlo,y[(ic*maxpoints)+numpoints[ic]]);
    numpoints[ic]++;
}


void chartwindow::drawaxes()
{
    char s[256];
    long y0;

    makecurrentwindow();
    if (doublebuffered) frontbuffer(TRUE);
    c3f(&whitecolf.r);
    if ( (vlo[0]<=0.0) && (vhi[0]>=0.0) )
        y0 = (yhi-ylo)*(0.0-vlo[0])/(vhi[0]-vlo[0]) + ylo;
    else
        y0 = ylo;

    // x-axis
    move2i(xlo-2,y0);
    draw2i(xhi,y0);
    // y-axis
    move2i(xlo,ylo);
    draw2i(xlo,yhi);

    // label the y-axis (but not the x)
    move2i(xlo-2,ylo);
    draw2i(xlo+2,ylo);
    if ( (vhi[0] > 1.0) &&
         (float(long(vhi[0])) == vhi[0]) &&
         (float(long(vlo[0])) == vlo[0]) )
        sprintf(s,"%d",long(vlo[0]));
    else
        sprintf(s,"%.2f",vlo[0]);
    cmov2i(xlo-strwidth(s)-2,ylo-6);
    charstr(s);

    move2i(xlo-2,yhi);
    draw2i(xlo+2,yhi);
    if ( (vhi[0] > 1.0) &&
         (float(long(vhi[0])) == vhi[0]) )
        sprintf(s,"%d",long(vhi[0]));
    else
        sprintf(s,"%.2f",vhi[0]);
    cmov2i(xlo-strwidth(s)-2,yhi-6);
    charstr(s);

    // display the window title
    if (name != NULL)
    {
        cmov2i((xlo+xhi-strwidth((String)name))/2,ytitle);
        charstr((String)name);
    }

    if (decimation)
    {
        register long x = (xlo+xhi)/2;
        long y = ylo - 2;
        while ((y+4) <= (ytitle-2))
        {
            move2i(x,y);
            draw2i(x,y+4);
            y += 8;
        }
    }
    if (doublebuffered) frontbuffer(FALSE);
}


void chartwindow::plotpoints()
{
    makecurrentwindow();
    if (doublebuffered) frontbuffer(TRUE);
    for (short ic = 0; ic < numcurves; ic++)
    {
        c3f(&color[ic].r);
        for (long i=0; i < numpoints[ic]; i++)
            if ( (y[(ic*maxpoints)+i]>=ylo) && (y[(ic*maxpoints)+i]<=yhi) )
                pnt2i(i+xlo,y[(ic*maxpoints)+i]);
    }
    if (doublebuffered) frontbuffer(FALSE);
}


void chartwindow::plotpoint(cshort ic, clong x, clong y)
{
    if ( (y>=ylo) && (y<=yhi) )
    {
        makecurrentwindow();
        if (doublebuffered) frontbuffer(TRUE);
        c3f(&color[ic].r);
        pnt2i(x,y);
        if (doublebuffered) frontbuffer(FALSE);
    }
}


void chartwindow::setprefsize(clong x, clong y)
{
    gwindow::setprefsize(x,y);
    xlo = framewidth + 50;
    xhi = x - framewidth - 10;
    ylo = framewidth + 10;
    yhi = y - framewidth - 10;
    ytitle = yhi - 6;
    maxpoints = xhi - xlo + 1;
    for (short ic = 0; ic < numcurves; ic++)
    {
        dydv[ic] = float(yhi - ylo) / (vhi[ic] - vlo[ic]);
    }
}


void chartwindow::setprefposition(clong x1, clong x2,
                                    clong y1, clong y2)
{
    gwindow::setprefposition(x1,x2,y1,y2);
    xlo = framewidth + 50;
    xhi = (x2-x1+1) - framewidth - 10;
    ylo = framewidth + 10;
    yhi = (y2-y1+1) - framewidth - 10;
    ytitle = yhi - 6;
    maxpoints = xhi - xlo + 1;
    for (short ic = 0; ic < numcurves; ic++)
    {
        dydv[ic] = float(yhi - ylo) / (vhi[ic] - vlo[ic]);
    }
}


void chartwindow::open()
{
    gwindow::open();
    drawaxes();
}


void chartwindow::init(short ncurves)
{
    setzbuffer(FALSE);
    setdoublebuffer(FALSE);
    setframewidth(2);
    setframecolor(1.,1.,1.);
    setborder(FALSE);
    xlo = 0;
    xhi = 100;
    ylo = 0;
    yhi = 100;
    maxpoints = xhi - xlo + 1;
    decimation = 0;
    numcurves = ncurves;
    y = NULL;
    numpoints = new long[numcurves];
    vlo = new float[numcurves];
    vhi = new float[numcurves];
    dydv = new float[numcurves];
    color = new colf[numcurves];
    for (short ic = 0; ic < numcurves; ic++)
    {
        numpoints[ic] = 0;
        vlo[ic] = 0.;
        vhi[ic] = 1.;
        dydv[ic] = float(yhi - ylo) / (vhi[ic] - vlo[ic]);
        if (numcurves > 1)
        {
            color[ic].r = float(ic) / float(numcurves-1);
            color[ic].g =      1.  -  0.5 * float(ic) / float(numcurves-1);
            color[ic].b = fabs(1.  -  2.0 * float(ic) / float(numcurves-1));
        }
        else
        {
            color[ic].r = 0.;
            color[ic].g = 1.;
            color[ic].b = 1.;
        }
    }
}


void chartwindow::terminate()
{
    if (y) delete y;
    if (numpoints) delete numpoints;
    if (vlo) delete vlo;
    if (vhi) delete vhi;
    if (dydv) delete dydv;
    if (color) delete color;
}


void binchartwindow::setprefsize(clong x, clong y)
{
    gwindow::setprefsize(x,y);
    xlo = framewidth + 50;
    xhi = x - framewidth - 10;
    ylo = framewidth + 10;
    yhi = y - framewidth - 22;
    ytitle = yhi + 5;
    maxpoints = xhi - xlo + 1;
    numbins = yhi - ylo + 1;
    dydv[0] = float(yhi - ylo) / (vhi[0] - vlo[0]);
}


void binchartwindow::setprefposition(clong x1, clong x2, clong y1, clong y2)
{
    gwindow::setprefposition(x1,x2,y1,y2);
    xlo = framewidth + 50;
    xhi = (x2-x1+1) - framewidth - 10;
    ylo = framewidth + 10;
    yhi = (y2-y1+1) - framewidth - 22;
    ytitle = yhi + 5;
    maxpoints = xhi - xlo + 1;
    numbins = yhi - ylo + 1;
    dydv[0] = float(yhi - ylo) / (vhi[0] - vlo[0]);
}


void binchartwindow::init()
{
//  chartwindow::init(1); // done automatically???
    color[0].r = 1.; color[0].g = 1.; color[0].b = 1.;
    fy = NULL;
    numbins = yhi - ylo + 1;
    myexp = 1.0;
}


void binchartwindow::plotpoints()
{
    makecurrentwindow();
    if (doublebuffered) frontbuffer(TRUE);

    for (long i = 0; i < numpoints[0]; i++)
        plotpoint(xlo+i,&fy[i*numbins]);

    if (doublebuffered) frontbuffer(FALSE);
}


void binchartwindow::plotpoint(clong x, cfloat* y)
{
    colf c;
    for (long i = 0; i < numbins; i++)
    {
        c.r = y[i]*color[0].r; c.g = y[i]*color[0].g; c.b = y[i]*color[0].b;
        c3f(&c.r);
        pnt2i(x,ylo+i);
    }
}


void binchartwindow::dump(ostream& out)
{
    out << numpoints[0] nl;
    out << numbins nl;
    for (register long i = 0; i < numpoints[0]; i++)
    {
        for (register long j = 0; j < numbins; j++)
        {
            out << fy[(i*numbins)+j] nl;
        }
    }
}


void binchartwindow::load(istream& in)
{
    in >> numpoints[0];
    if (numpoints[0] > maxpoints)
        error(2,
            "while loading binchartwindow from dump file, numpoints too big");
    long newnumbins;
    in >> newnumbins;
    if (newnumbins != numbins)
        error(2,
            "while loading binchartwindow from dump file, numbins redefined");
    if (y == NULL)  // first time must allocate space
    {
        y = new long[maxpoints*numbins];
        if (y == NULL)
        {
            error(2,"insufficient space for binchartwindow points duringload");
            return;
        }
        fy = (float*)y;
    }
    for (register long i = 0; i < numpoints[0]; i++)
    {
        for (register long j = 0; j < numbins; j++)
        {
            in >> fy[(i*numbins)+j];
        }
    }
}


void binchartwindow::addpoint(cfloat* val, clong numval)
{
    long i,j,k;
    long ymx = 0;

    if (y == NULL)  // first time must allocate space
    {
        y = new long[maxpoints*numbins];
        if (y == NULL)
        {
            error(2,"unable to allocate space for binchartwindow points");
            return;
        }
        fy = (float*)y;
    }

    if (numpoints[0] == maxpoints)
    {
        // cut the number (and resolution) of points stored in half
        for (i=1, j=2; j < maxpoints; i++, j+=2)
        {
            for (k = 0; k < numbins; k++)
                fy[i*numbins+k] = fy[j*numbins+k];
        }
        numpoints[0] = i;
        decimation++;
        redraw();
    }

    register long koff = numpoints[0] * numbins;

    for (k = 0; k < numbins; k++)
        y[koff+k] = 0;

    for (i = 0; i < numval; i++)
    {
        k = (val[i] - vlo[0]) * dydv[0];
        k = max(0,min(numbins-1,k));
        y[koff+k]++;
        ymx = max(ymx,y[koff+k]);
    }
    for (k = 0; k < numbins; k++)
        fy[koff+k] = pow(float(y[koff+k])/float(ymx),myexp);

    makecurrentwindow();
    if (doublebuffered) frontbuffer(TRUE);

    plotpoint(numpoints[0]+xlo,&fy[koff]);

    if (doublebuffered) frontbuffer(FALSE);

    numpoints[0]++;
}


// end of gwindow.C

#endif GWINDOW_C
