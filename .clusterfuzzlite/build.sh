#!/bin/bash -eu

BUILD=$WORK/build

# cleanup
rm -rf "$BUILD"
mkdir -p "$BUILD"

# setup project
meson setup "$BUILD" \
  --buildtype=plain \
  -Dfuzzer=enabled \
  -Dfuzzer_ldflags="$LIB_FUZZING_ENGINE" \
  -Ddebug_level=0 \
  -Ddefault_library=static \
  -Db_lundef=false

# build fuzzers
ninja -v -C "$BUILD" test/fuzz/{fuzz_round_trip,fuzz_compression,fuzz_decompression,fuzz_cmp_tool}
find "$BUILD/test/fuzz" -maxdepth 1 -executable -type f -exec cp "{}" "$OUT" \;

#TODO prepare corps
