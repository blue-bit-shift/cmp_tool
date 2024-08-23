# Installation Instructions
## Getting started

### Install git and python 3.7+

If you're on Linux, you probably already have these. On macOS and Windows, you can use the
[official Python installer](https://www.python.org/downloads).

### Install meson and ninja

Meson 0.63 or newer is required.  
You can get meson through your package manager or using:

```
pip3 install meson
```

Check if meson is included in your PATH.

You should get `ninja` using your [package manager](https://github.com/ninja-build/ninja/wiki/Pre-built-Ninja-packages) or download the [official
release](https://github.com/ninja-build/ninja/releases) and put the `ninja`
binary in your PATH.

### Get the Source Code

We use the version control system [git](https://git-scm.com/downloads) to get a copy of the source code.

```
git clone https://gitlab.phaidra.org/loidoltd15/cmp_tool.git  
cd cmp_tool
```
## Build the cmp\_tool
### Build the cmp\_tool for Debugging

First, we create the `builddir` directory. Everything we build will be inside this directory.

```
meson setup builddir
```

We change to the build directory and build the cmp_tool:

```
cd builddir
meson compile cmp_tool
```

Now you should find the cmp\_tool executable in the programs folder.

### Release Build

If you want to create an optimized release version, we can create a build directory for it:

```
meson setup build_relase_dir --buildtype=release
cd build_relase_dir
meson compile cmp_tool
```

You find the build cmp\_tool executable in the `build_relase_dir/programs` directory.

### Build for Windows

Unfortunately, the cmp\_tool does not support the Microsoft MSVC compiler. To build the cmp\_tool for Windows you can use the Mingw-w64 GCC compiler.
For this, you need the [Mingw-w64 toolchain](https://www.mingw-w64.org/downloads/). To compile on Windows, do this in the Cygwin64 Terminal:

```
meson setup builddir_win --native-file=mingw-w64-64.txt
cd builddir_win
meson compile cmp_tool
```

### Cross-compile for Windows
Cross-compile for Windows is also possible with the [Mingw-w64 toolchain](https://www.mingw-w64.org/downloads/). To cross-compile for Windows use the following commands: 

```
meson setup builddir_cross_win --cross-file=mingw-w64-64.txt
cd builddir_cross_win
meson compile cmp_tool
```

## Tests
### External dependencies

To run the unit tests you need the [ruby interpreter](https://www.ruby-lang.org/en/documentation/installation/).  
To run the cmp\_tool interface test you need the [pytest](https://docs.pytest.org/en/7.0.x/index.html) framework. The easiest way to install pytest is with `pip3`:

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

First, ensure that either `gcovr` or `lcov` is installed as a dependency for generating coverage reports.

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
### Benchmarking

To run the compression speed test bench, follow these steps:

```
cd <name of the build directory>
meson test --benchmark
```


### Fuzzing
If you’re unfamiliar with fuzzing and libFuzzer, you can find a tutorial [here](https://github.com/google/fuzzing/blob/master/tutorial/libFuzzerTutorial.md). 
To perform fuzzing with libFuzzer, AddressSanitizer, and UndefinedBehaviorSanitizer, follow these steps:

Set up your build directory with the necessary configurations. Note that you’ll need a clang version that has libFuzzer support. Use the following command:

```
CC=clang CXX=clang++ \
meson setup builddir_fuzzing \
  --buildtype=plain \
  -Dfuzzer=enabled \
  -Dfuzzer_ldflags=-fsanitize=fuzzer \
  -Dc_args="-O1 -gline-tables-only -fsanitize-address-use-after-scope -fsanitize=fuzzer-no-link" \
  -Db_sanitize=address,undefined \
  -Ddebug_level=0 \
  -Ddefault_library=static \
  -Db_lundef=false
```

The `builddir_fuzzing` directory is now configured for fuzzing. Execute different fuzz targets using the meson test command. For example:

```
cd builddir_fuzzing
# List available tests
meson test --list

# Run a specific fuzz target (e.g., fuzz_round_trip for 10 minutes)
meson test fuzz_round_trip\ 10\ min --verbose
```

Happy fuzzing! 🚀

To reset the coverage data, use the following command:

```
ninja clean-gcda
```

## Documentation 
### External dependencies
To generate the documentation you need the [Doxygen](https://www.doxygen.nl/index.html) program.
Optional you can install the "dot" tool from [graphviz](http://www.graphviz.org/) to generate more advanced diagrams and graphs.  

### Generate Documentation

To generate the documentation you need to run:

```
git submodule update --init
cd <name of the build directory>
meson compile doc
```

You can find the documentation in the directory **generated_documentation**.
