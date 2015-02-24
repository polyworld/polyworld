# Download Source
First, use `git` to download the repository. If you're downloading the official repository, then issue
the following command in the terminal:
```
git clone --recursive https://github.com/polyworld/polyworld.git
```
This will create a directory `polyworld` under the directory from which you issued the command.

If you're downloading a developer's forked repository, simply substitute that repository, e.g.:
```
git clone --recursive https://github.com/sean-dougherty/polyworld.git
```

Now, move to the directory containing the source:
```
cd polyworld
```

# Configure
Next, issue the following command from the terminal:
```
./configure
```

Note that this command has several options for which brief documentation can be obtained via the following command:
```
./configure --help
```
As of this writing, the help message is as follows:
```
usage: configure [-h] [-g] [-c CXX] [-q QMAKE] [--toolchain TOOLCHAIN] [--os OS]

optional arguments:
  -h, --help            show this help message and exit
  -g, --debug           debug build
  -c CXX, --cxx CXX     specify compiler with optional path (e.g. clang++, /usr/bin/g++-4.9)
  -q QMAKE, --qmake QMAKE
                        specify qmake with optional path (e.g. qmake, /usr/bin/qmake)
  --toolchain TOOLCHAIN
                        specify toolchain (gcc,llvm)
  --os OS               specify OS (linux,darwin)
```

These options can be helpful if you need to use a specific compiler or qmake.

# Build
Now compile the source:
```
make
```

## Build Errors
If you get build errors, first make sure you've properly installed the dependencies for your OS (e.g. GSL and Qt). It's also possible that the incorrect compiler or Qt is being used. You can use the `configure` script's `--cxx` and `--qmake` options to explicitly point to the correct versions of your compiler and qmake.

# Run a simple experiment
You should now be able to run a simple experiment with the following command:
```
./Polyworld worldfiles/hello.wf
```