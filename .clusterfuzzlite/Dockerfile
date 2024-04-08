FROM gcr.io/oss-fuzz-base/base-builder:v1                         # Base image with clang toolchain
RUN apt-get update && apt-get install -y python3-pip ninja-build  # install required packages to build your project
RUN pip install -U --pre meson
COPY . $SRC/cmp_tool                                              # Copy your project's source code.
WORKDIR $SRC/cmp_tool                                             # Working directory for build.sh.
COPY .clusterfuzzlite/build.sh $SRC/                              # Copy build.sh into $SRC dir.
