# Base image with clang toolchain
FROM gcr.io/oss-fuzz-base/base-builder:v1@sha256:f161edec0c10ad31ced9fcec3d4d0d1dd8a36a22de9ebef136a41dcfac34d1ba
# install required packages to build your project
RUN apt-get update && apt-get install -y python3-pip ninja-build
RUN pip install -U --pre meson
# Copy your project's source code
COPY . $SRC/cmp_tool
# Working directory for build.sh.
WORKDIR $SRC/cmp_tool
# Copy build.sh into $SRC dir.
COPY .clusterfuzzlite/build.sh $SRC/
