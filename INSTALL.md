## Installation Instructions

### Install git and python 3.6+

If you're on Linux, you probably already have these. On macOS and Windows, you can use the
[official Python installer](https://www.python.org/downloads/).

### Install meson and ninja

Meson 0.56 or newer is required.

You can get meson through the Python package manager or using:

```pip3 install meson```

Check if meson is included in your PATH.

You should get `ninja` using your [package manager](https://github.com/ninja-build/ninja/wiki/Pre-built-Ninja-packages) or download the [official
release](https://github.com/ninja-build/ninja/releases) and put the `ninja`
binary in your PATH.

### Get the Source Code

We use the version control system git to get a copy of the source code.

```
git clone https://gitlab.phaidra.org/loidoltd15/cmp_tool.git  
cd cmp_tool
```

### Build the cmp_tool for Debugging

You can build the cmp_tool running:

```
meson builddir
```

This will automatically create the `builddir` directory and build **everything** **inside** it.

```
cd builddir
meson compile
```

Now you should find the cmp_tool executable in the folder.

### Release Build

If you want to build an optimized release build run:

```
meson build_relase_dir --buildtype=release
cd build_relase_dir
meson compile
```

You find the build executable in the `build_relase_dir` directory

### Cross-compiling to native Windows

To build the cmp_tool you can use the [Mingw-w64](https://www.mingw-w64.org).
Unfortunately, the cmp_tool does not support the Microsoft MSVC compiler. But with the Mingw-w64 GCC compiler, we can compile the cmp_tool for Windows. For this, you need the [Mingw-w64 tool chain](https://www.mingw-w64.org/downloads/). This also works on Linux and macOS. To compile for Windows, do this:

```
meson setup buiddir_win --cross-file=mingw-w64-64.txt
cd buiddir_win
meson compile
```

## Tests
### External dependencies

To run the unit tests you need the [c unit testing framework](https://sourceforge.net/projects/cunit/).  
To run the integration tests you need the [pytest](https://docs.pytest.org/en/7.0.x/index.html) framework. The easiest way to install pytest is with `pip3`:

```
pip3 install pytest
```
### Run tests
First, cd in the build directory:

```
cd <name of the build directory>
```

You can easily run the test of all the components:

```
meson test
```

To list all available tests:

```
meson test --list
```

Meson also supports running the tests under GDB. Just doing this:

```
meson test --gdb <testname>
```

### Producing a coverage report

First, configure the build with this command.

```
cd <name of the build directory>
meson configure -Db_coverage=true
```

Then issue the following commands.

```
meson test
ninja coverage-html
```

The coverage report can be found in the `meson-logs/coveragereport` subdirectory.

## Documentation 
### External dependencies
To generate the documentation you need the [Doxygen](https://www.doxygen.nl/index.html) program.
Optional you can install the "dot" tool from [graphviz](http://www.graphviz.org/) to generate more advanced diagrams and graphs.  

### Generate Documentation

To generate the documentation you need to run:

```
cd <name of the build directory>
meson compile doc
```

