#!/bin/bash -eu

BUILD=$WORK/build

# cleanup
rm -rf "$BUILD"
mkdir -p "$BUILD"

echo "$LIB_FUZZING_ENGINE"

# setup project
meson setup "$BUILD" \
  --buildtype=plain \
  -Dfuzzer=enabled \
  -Dfuzzer_ldflags="$LIB_FUZZING_ENGINE" \
  -Ddebug_level=0 \
  -Ddefault_library=static \
  --wrap-mode=nodownload

# build fuzzers
ninja -v -C "$BUILD" test/fuzz/fuzz_{round_trip,compression}
find "$BUILD/test/fuzz" -maxdepth 1 -executable -type f -exec cp "{}" "$OUT" \;

#TODO prepare corps
