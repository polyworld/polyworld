Welcome to the modern version of Polyworld, an Artificial Life
system designed as an approach to Artificial Intelligence.  The
original version was developed while I was working for Alan Kay's
Vivarium Program at Apple Computer.  This version was heavily updated
and adapted by Gene Ragan, Nicolas Zinovieff, and myself, to further
this line of research, now that I am a Professor of Informatics at
Indiana University, and am finally able to once again devote my
time to this subject.

Polyworld currently runs atop Qt from Trolltech
<http://www.trolltech.com/>.  It uses the "Qt Open Source Edition",
in keeping with its open source nature.  Polyworld itself is open
sourced under the Apple Public Source License (see the accompanying
file named LICENSE) through SourceForge.net, in keeping with its
original copyright by Apple and open source nature.

To build Polyworld, you will need to:

Download and configure Qt (Open Source Edition)
* visit <http://www.trolltech.com/download/opensource.html> and
  download the version appropriate to your development platform
  (a free Windows version will be available in July 2005)
* use gunzip and gnutar (or use StuffIt Expander) to unpack the
  archive someplace convenient (like ~/src)
* the Qt documentation suggests renaming the resulting directory
  to simply "qt", and then within that directory creating a
  symbolic link:
     ln -s doc/man man
  (or, equivalently, after setting up the environment variables
  below, typing 'ln -s $QTDIR/doc/man $QTDIR/man')
* reading the Qt documentation you will see that you must also set
  some environment variables (such as in your .login or .cshrc file):
     setenv QTDIR <path to Qt main directory>
     setenv PATH $QTDIR/bin:$PATH
     setenv MANPATH $QTDIR/man:`manpath`
     setenv DYLD_LIBRARY_PATH $QTDIR/lib
* Polyworld adds one additional required environment variable:
     setenv QT_INCLUDE_DIR <path to Qt include dir>
  (Normally this is just $(QTDIR)/include, but making it explicit
  allows us to support some alternative Qt configurations, such
  as a "darwinports" installation)
* configure Qt by typing something like './configure -thread -static'
  (I am currently using -static, so the application can be run on
  machines without Qt installed, and -thread, though threading is
  is not currently used in Polyworld.  Type './configure -help' for
  other options.) NOTE: This can take several minutes to complete
* build Qt by typing 'make'
  NOTE: This can take an hour or more to complete!
  
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

Build Polyworld (Qt method)
* enter the polyworld directory by typing 'cd polyworld'
* type './buildit'

Build Polyworld (Xcode method)
* Add some key Qt paths in the Xcode "Source Trees" preferences:
	- QtInc should point to the include dir of the Qt distribution 
	- QtLib should point to the lib dir of the Qt distribution
	- QtBin should point to the bin dir of the Qt distribution
	("Setting Name" and "Display Name" can be the same; the "Path"
	depends on your Qt installation.)
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
tested on Mac OS X.

A build has been successful on Linux, but it required a modification
to variables INCPATH and LIBS in the Polyworld Makefile (resulting
from the qmake step in the buildit script), so they would point to
the Linux headers and libs instead of the OS X frameworks.  I believe
this is fixed now, but have not yet tested it.  It was also necessary
to replace my definition of dprintf (with, for example, dbprintf) in
app/debug.h to avoid a conflict with a system-defined dprintf.  I will
try to normalize the OS X and Linux builds soon.

Technical details of the algorithms used in Polyworld may be found
here: <http://pobox.com/~larryy/PolyWorld.html>, particularly in
the technical paper here:
<ftp://ftp.beanblossom.in.us/pub/PolyWorld/Yaeger.ALife3.pdf>.

For more details about using cvs to check out source code from
SourceForge.net, see
<https://sourceforge.net/docman/display_doc.php?docid=14033&group_id=1>.

