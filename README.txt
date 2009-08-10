Welcome to the modern version of Polyworld, an Artificial Life
system designed as an approach to Artificial Intelligence.  The
original version was developed while I was working for Alan Kay's
Vivarium Program at Apple Computer.  This version was heavily updated
and adapted by Gene Ragan, Nicolas Zinovieff, and myself, to further
this line of research, now that I am a Professor of Informatics at
Indiana University, and am finally able to once again devote my
time to this subject.  Virgil Griffith has been an important
contributor and collaborator, particularly in the implementation
of Olaf Sporns's complexity measure and various scripts.  Matt
Whitehead implemented FoodPatches and consolidated the multiple
x-sorted lists into a single x-sorted list.  Matt Ira implemented
the first version of a spiking neuron model in Polyworld after
Izhikevich.

Polyworld currently runs atop Qt 4.5 (or later) from Trolltech
<http://www.trolltech.com/>.  It uses the "Qt Open Source Edition",
in keeping with its open source nature.  Polyworld itself is open
sourced under the Apple Public Source License (see the accompanying
file named LICENSE) through SourceForge.net, in keeping with its
original copyright by Apple and open source nature.

To setup and build Polyworld, you should follow the step-by-step
instructions at:

http://beanblossom.in.us/larryy/BuildingPolyworld.html

Running atop Qt, Polyworld should be fully cross-platform (Mac OS
X, Windows, and Linux), but currently is only routinely built and
tested on Mac OS X and Linux.  At a minimum, polyworld.pro needs to be
tailored to the platform, in order to replace OS X's -framework notation
with -l library invocations; other libraries may also need to be added.

Running Polyworld - Keyboard Controls:
	r	- Toggle Camera Rotation
	1-5	- Select 1st through 5th fittest critter for overhead monitoring
	t	- Track currently selected agent until it dies (rather than switching when the most-fit critters change)
	+/=	- Zoom in Overhead view
	-	- Zoom out Overhead view

---

Technical details of the algorithms used in Polyworld may be found
here: <http://pobox.com/~larryy/PolyWorld.html>, particularly in
the technical paper here: <http://www.beanblossom.in.us/larryy/Yaeger.ALife3.pdf>

For more details about using cvs to check out source code from
SourceForge.net, see <https://sourceforge.net/docman/display_doc.php?docid=14033&group_id=1>.

CVS tagged revisions:
pw08_qt331 - A mostly functional 0.8 version of Polyworld targeted to Qt 3.3.1

