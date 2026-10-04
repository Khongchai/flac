#!/usr/bin/env bash
set -euo pipefail

SOURCES="src/libFLAC/bitmath.c src/libFLAC/bitreader.c src/libFLAC/cpu.c src/libFLAC/crc.c src/libFLAC/fixed.c src/libFLAC/float.c src/libFLAC/format.c src/libFLAC/lpc.c src/libFLAC/md5.c src/libFLAC/memory.c src/libFLAC/stream_decoder.c"

FUNCTIONS="['_variation_flac_convert']"

emcc ${OPT:--O3} variation_flac_decoder.c $SOURCES \
  -include wasm/variation_flac_config.h -DNDEBUG \
  -I include -I src/libFLAC/include \
  -s MODULARIZE=1 -s EXPORT_NAME="createFlacModule" \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s EXPORTED_FUNCTIONS="$FUNCTIONS" \
  -s EXPORTED_RUNTIME_METHODS=HEAPU8,HEAPF32 \
  -o flac.js
