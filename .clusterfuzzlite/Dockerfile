# Base image with clang toolchain
FROM gcr.io/oss-fuzz-base/base-builder:v1
# install required packages to build your project
RUN apt-get update && apt-get install -y python3-pip ninja-build
RUN pip install -U --pre meson
# Copy your project's source code
COPY . $SRC/cmp_tool
# Working directory for build.sh.
WORKDIR $SRC/cmp_tool
# Copy build.sh into $SRC dir.
COPY .clusterfuzzlite/build.sh $SRC/
