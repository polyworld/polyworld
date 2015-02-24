# Install Dependencies

## Install XCode command-line tools
If you already have XCode command-line tools installed, then typing `clang++` at the terminal should produce output like the following:
```
clang: error: no input files
```
If they aren't installed, then the `clang++` command should automagically initiate the installation for you.

## Install Qt
The latest version of Qt has some known issues, so it is recommended that you install version 4.8.6, which can be obtained from [https://download.qt.io/archive/qt/4.8/4.8.6/qt-opensource-mac-4.8.6-1.dmg](https://download.qt.io/archive/qt/4.8/4.8.6/qt-opensource-mac-4.8.6-1.dmg). Once you have downloaded the dmg, simply open it from the download manager and then double-click the file within it named `Qt.mpkg`.

Once the installation is complete, open a terminal and verify that typing `qmake --version` produces output like the following:
```
QMake version 2.01a
Using Qt version 4.8.6 in /Library/Frameworks
```

If the qmake command is not found, or if it doesn't report it's using Qt version 4.8.6, try `/Developer/Tools/Qt/qmake --version`.

## Install GSL
Install the GNU Scientific Library, which can be found at [http://www.gnu.org/software/gsl](http://www.gnu.org/software/gsl/).

You may try the following sequence of commands in your terminal, which is the procedure I personally used to download, build, and install:
```
cd /tmp && mkdir gsl && cd gsl
curl ftp://ftp.gnu.org/gnu/gsl/gsl-latest.tar.gz > gsl.tgz
tar xf gsl.tgz && cd gsl-*
./configure && make && sudo make install
```

# Perform POSIX Install Procedure
The OSX-specific install is now complete, so proceed to the [POSIX installation procedure](./Installing-on-POSIX).