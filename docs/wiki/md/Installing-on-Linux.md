The main requirements for running on Linux are:
* git
* C++11 compiler (e.g. g++ 4.9)
* OpenGL development files
* Qt 4.8 development files (Qt 5 has been found to cause problems)
* GSL (GNU Scientific Library) development & debug files
* Python 2.7

The exact install steps will vary between distros, but the following should work on a Debian/Ubuntu-like system:
```
sudo apt-get install git g++ mesa-common-dev libqt4-dev libgsl0-dev libgsl0-dbg
```

Once you have installed these dependencies, please proceed to the general [POSIX installation](./Installing-on-POSIX).