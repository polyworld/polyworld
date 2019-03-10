# Install Dependencies

## Install XCode command-line tools
If you already have XCode command-line tools installed, then typing `clang++` at the terminal should produce output like the following:
```
clang: error: no input files
```
If they aren't installed, then the `clang++` command should automagically initiate the installation for you.

## Install Qt
The latest version of Qt as of this writing -- Qt 5.12.1 -- should work fine.  Obtain it from [https://qt.io/](https://qt.io/).  Once you have downloaded the dmg, simply open it and double-click the install app it contains.

Once the installation is complete, you will probably need to add the Qt bin directory containing qmake to your PATH. Using Qt's default installation location that path on Larry's system is:

`/Users/larryy/Qt5.12.1/5.12.1/clang_64/bin`

Then open a terminal and verify that typing `qmake --version` produces output like the following:
```
QMake version 3.1
Using Qt version 5.12.1 in /Users/larryy/Qt5.12.1/5.12.1/clang_64/lib
```

## Install GSL
Install the GNU Scientific Library, which can be found at [http://www.gnu.org/software/gsl](http://www.gnu.org/software/gsl/).

You may try the following sequence of commands in your terminal, which is the procedure Sean uses to download, build, and install:
```
cd /tmp && mkdir gsl && cd gsl
curl ftp://ftp.gnu.org/gnu/gsl/gsl-latest.tar.gz > gsl.tgz
tar xf gsl.tgz && cd gsl-*
./configure && make && sudo make install
```

# Perform POSIX Install Procedure
The OSX-specific install is now complete, so proceed to the [POSIX installation procedure](./Installing-on-POSIX).
