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

* download Qt (Open Source Edition)
* configure Qt (I am currently using -static, so the application
  can be run on machines without Qt installed, and -thread, though
  currently threading is not used in Polyworld)
* mkdir projects
* cd projects
* cvs co polyworld (use
  -d:pserver:anonymous@cvs.sf.net:/cvsroot/polyworld unless you are
  an authorized polyworld developer, in which case you should do an
  'export CVS_RSH=ssh' and use
  -d:ext:username@cvs.sf.net:/cvsroot/polyworld, substituting your
  own username)
* ./buildit
* ln -s .moc moc (only needed if you want to use the Xcode project)

If you are using Mac OS X, you may at this point prefer to switch
to using the Polyworld.xcode project for development.  You will
need to create a symbolic link of worldfile in your build products
directory (next to the newly built Polyworld.app).  Also be aware
that the 'buildit' step above produces a Polyworld.app/ in the
polyworld project directory.  Xcode will also attempt to produce a
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
X, Windows, and Linux), but has, so far, only been built and tested
on Mac OS X.

Technical details of the algorithms used in Polyworld may be found
here: <http://pobox.com/~larryy/PolyWorld.html>, particularly in
the technical paper here:
<ftp://ftp.beanblossom.in.us/pub/PolyWorld/Yaeger.ALife3.pdf>.

For more details about using cvs to check out source code from
SourceForge.net, see
<https://sourceforge.net/docman/display_doc.php?docid=14033&group_id=1>.

