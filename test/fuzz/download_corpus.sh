#!/bin/bash

bin_path=${1:-}
if [[ -z "$bin_path" ]]; then
    echo "usage: $0 <fuzz target>"
    exit 1
fi

if [[ "$OSTYPE" == "darwin"* ]]; then
    DOWNLOAD="curl --silent -L -o"
else
    DOWNLOAD="wget --quiet --output-document"
fi

ROOT_DIR=${MESON_BUILD_ROOT:-.}"/"
CORPORA_URL=https://gitlab.phaidra.org/loidoltd15/cmp_tool_storage/-/archive/main/cmp_tool_storage-main.tar.bz2
CORPUS_COMPRESSED=$ROOT_DIR"cmp_tool_storage.tar.bz2"
CORPUS="cmp_tool_storage-main"

CORPUS_ID=$(git ls-remote https://gitlab.phaidra.org/loidoltd15/cmp_tool_storage.git HEAD | head -c 8)
CORPUS_DIR="$ROOT_DIR"corpus__"$CORPUS_ID"

if [ ! -d "$CORPUS_DIR" ]; then
    $DOWNLOAD "$CORPUS_COMPRESSED" "$CORPORA_URL"
    mkdir "$CORPUS_DIR"
    tar -xf "$CORPUS_COMPRESSED" -C "$CORPUS_DIR"
fi
rm -f "$CORPUS_COMPRESSED"

for arg in "$@" 
do
    find "$CORPUS_DIR"/"$CORPUS"/corpus -name "$arg"
done
