/********************************************************************/
/* PolyWorld:  An Artificial Life Ecological Simulator              */
/* by Larry Yaeger                                                  */
/* Copyright Apple Computer 1990,1991,1992                          */
/********************************************************************/
// globals.h - global variables for pw (PolyWorld)

#ifndef GLOBALS_H
#define GLOBALS_H

// System
#include <stdio.h>

// STL
#include <map>
#include <string>

// qt
#include <qevent.h>

// Local
#include "AbstractFile.h"
#include "GenomeLayout.h"
#include "graphics.h"

// Forward declarations
class agent;

// Custom events
const int kUpdateEventType = QEvent::User + 1;

static const int kMenuBarHeight = 22;

// [TODO] 
static const long XMAXSCREEN = 1024;
static const long YMAXSCREEN = 768;
static const long xscreensize = XMAXSCREEN + 1;
static const long yscreensize = YMAXSCREEN + 1;
static const int MAXLIGHTS = 10;

// Prefs
//static const char kPrefPath[] = "edu.indiana.polyworld";
//static const char kPrefSection[] = "polyworld";
//static const char kAppSettingsName[] = "polyworld";
static const char kWindowsGroupSettingsName[] = "windows";

class globals
{
public:
	static float	worldsize;
	static bool		wraparound;
	static bool		blockedEdges;
	static bool     stickyEdges;
	static int      numEnergyTypes;
	static AbstractFile::ConcreteFileType recordFileType;
};

#endif
