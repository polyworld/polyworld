Welcome to the modern version of Polyworld, an Artificial Life
system designed as an approach to Artificial Intelligence.  The
original version was developed while I was working for Alan Kay's
Vivarium Program at Apple Computer.  This version was heavily updated
and adapted by Gene Ragan, Nicolas Zinovieff, and myself, to further
this line of research, now that I am a Professor of Informatics at
Indiana University, and am finally able to once again devote my
time to this subject.

Polyworld currently runs atop Qt 4 from Trolltech
<http://www.trolltech.com/>.  It uses the "Qt Open Source Edition",
in keeping with its open source nature.  Polyworld itself is open
sourced under the Apple Public Source License (see the accompanying
file named LICENSE) through SourceForge.net, in keeping with its
original copyright by Apple and open source nature.

NOTE:  Polyworld now *requires* Qt 4.x, and will no longer build on
Qt 3.x.

NOTE:  Polyworld also currently *requires* gcc-3.3, not gcc-4.
(We hope to change this soon, especially since, as of Tiger,
gcc-4 is the default in Mac OS X.)

To build Polyworld, you will need to:

Download and configure Qt (Open Source Edition)
* visit <http://www.trolltech.com/download/opensource.html> and
  download the version appropriate to your development platform
  (open source versions are now available for all major platforms)
* use gunzip and gnutar (or use StuffIt Expander) to unpack the
  archive someplace convenient (like ~/src)
* you may wish to rename the resulting directory to simply "qt"
* it may no longer be necessary, but I still set some environment
  variables (in .login), and do NOT do the recommended "make install",
  because I prefer to keep all the qt files in one place:
     setenv QTDIR <path to Qt main directory>
     setenv PATH $QTDIR/bin:$PATH
     setenv DYLD_LIBRARY_PATH $QTDIR/lib
* Polyworld adds one additional required environment variable:
     setenv QT_INCLUDE_DIR <path to Qt include dir>
  (Normally this is just $(QTDIR)/include, but making it explicit
  allows us to support some alternative Qt configurations, such
  as a "darwinports" installation)
* configure Qt by typing something like './configure -static'
  (I am currently using -static, so the application can be run on
  machines without Qt installed.  Type './configure -help' for
  other options.) NOTE: This can take several minutes to complete
* build Qt by typing 'make'
  NOTE: This can take several hours to complete!  If you want to
  speed things up and can live without working copies of all the
  Qt demos and examples, you can instead type 'make src' and
  'make tools', to build all and only the pieces needed for
  actual development.  (You may have to 'cd srs; make' and
  'cd tools; make' instead of invoking make from the qt
   directory.)
  
Download Polyworld source (from SourceForge)
* in an appropriate directory (I use .../qt/projects/), type
     cvs <cvs-args> co polyworld
  For <cvs-args>, use:
     -d:pserver:anonymous@cvs.sf.net:/cvsroot/polyworld
  unless you are an authorized polyworld developer, in which case
  you should do an 'export CVS_RSH=ssh' and use:
     -d:ext:username@cvs.sf.net:/cvsroot/polyworld
  substituting your own username, of course).  This will
  create a "polyworld" directory for you, containing all
  the Polyworld source code.
  
  Alternatively, if you are a Polyworld developer, you may wish
  to add these two entries to your .login or .cshrc file (or
  equivalent for your preferred shell):
     setenv CVS_RSH ssh
     setenv CVSROOT :ext:username@cvs.sf.net:/cvsroot/polyworld
  after which you may simply type
     cvs co polyworld
  (cvs honors the CVS_RSH and CVSROOT environment variables, if
  they are defined, and uses the right communication protocol to
  talk to the right host)

Mac versus Linux Installs
* The default 'polyworld.pro' file works only on Mac.  A special
  polyworld_linux.pro has been made for Linux.  To successfully
  build under linux replace the polyworld.pro file by:
	mv polyworld.pro polyworld_mac.pro
	mv polyworld_linux.pro polyworld.pro 

Build Polyworld (Qt method)
* enter the polyworld directory by typing 'cd polyworld'
* type './buildit'
* The contents of the ./buldit script are simply 'qmake' and 'make'.
  If for some reason qmake cannot find your polyworld.pro file, in 
  your ./buildit script simply replace 'qmake' with 
  'qmake polyworld.pro'

Build Polyworld (Xcode method)
* Add some key Qt paths in the Xcode "Source Trees" preferences:
	- QtInc should point to the include dir of the Qt distribution 
	- QtLib should point to the lib dir of the Qt distribution
	- QtBin should point to the bin dir of the Qt distribution
	("Setting Name" and "Display Name" can be the same; the "Path"
	depends on your Qt installation.)
    ***IMPORTANT NOTE*** As of Tiger, Xcode has a bug that causes
    it to crash when opening the Polyworld.xcode project if these
    parameters are not defined.  However, you cannot define them
    without launching Xcode.  A nasty Catch 22.  Well, actually,
    you could hand edit ~/Library/Preferences/com.apple.xcode.plist,
    but, fortunately you don't have to go there...  The work-around
    is to open any other project, that does NOT depend on these
    paths, and define them in your preferences before attempting
    to open Polyworld.xcode.  The paths are preferences that persist
    across all projects for a given user, so this will make it
    possible to develop Polyworld in Xcode on Tiger. ***END NOTE***
* Build

Be aware that the 'buildit' step above produces a Polyworld.app/ in
the polyworld project directory.  Xcode will also attempt to produce a
Polyworld.app/.  By default I believe the app is created in a ./build
subdirectory, so there is no conflict.  And I have had to do enough
work that is boot-system dependent that I routinely point builds
to a /build directory on the boot disk, for which there is no
conflict.  But if your default build products directory is the same
as the one containing the .xcode project, then you could have a
conflict.

If someone provides an equivalent Codewarrior or Visual Studio (or
whatever) project, I will add it to the source directory.  Please
inform me of any possible conflicts with the Qt/buildit Polyworld.app/.

Running atop Qt, Polyworld should be fully cross-platform (Mac OS
X, Windows, and Linux), but currently is only routinely built and
tested on Mac OS X.  At a minimum, polyworld.pro needs to be tailored
to the platform, in order to replace OS X's -framework notation with
-l library invocations; other libraries may also need to be added.

A build has been successful on Linux, but it required a modification
to variables INCPATH and LIBS in the Polyworld Makefile (resulting
from the qmake step in the buildit script), so they would point to
the Linux headers and libs instead of the OS X frameworks.  I believe
this is fixed now, but have not yet tested it.  I will try to
normalize the OS X and Linux builds soon.

Technical details of the algorithms used in Polyworld may be found
here: <http://pobox.com/~larryy/PolyWorld.html>, particularly in
the technical paper here:
<ftp://ftp.beanblossom.in.us/pub/PolyWorld/Yaeger.ALife3.pdf>.

For more details about using cvs to check out source code from
SourceForge.net, see
<https://sourceforge.net/docman/display_doc.php?docid=14033&group_id=1>.

CVS tagged revisions:
pw08_qt331 - A mostly functional 0.8 version of Polyworld targeted to Qt 3.3.1
