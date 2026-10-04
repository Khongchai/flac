#!/usr/bin/env bash
set -euo pipefail

SOURCES="src/libFLAC/bitmath.c src/libFLAC/bitreader.c src/libFLAC/cpu.c src/libFLAC/crc.c src/libFLAC/fixed.c src/libFLAC/float.c src/libFLAC/format.c src/libFLAC/lpc.c src/libFLAC/md5.c src/libFLAC/memory.c src/libFLAC/stream_decoder.c src/libFLAC/stream_encoder.c src/libFLAC/stream_encoder_framing.c src/libFLAC/window.c src/libFLAC/bitwriter.c"

FUNCTIONS="['_malloc', '_free', '_variation_flac_convert', '_variation_flac_encoder_create', '_variation_flac_encoder_destroy', '_variation_flac_encoder_encode', '_variation_flac_encoder_finish', '_variation_flac_encoder_header', '_variation_flac_encoder_header_length', '_variation_flac_encoder_output', '_variation_flac_encoder_output_length', '_variation_flac_encoder_clear_output']"

emcc ${OPT:--O3} variation_flac_decoder.c variation_flac_encoder.c $SOURCES \
  -include wasm/variation_flac_config.h -DNDEBUG \
  -I include -I src/libFLAC/include \
  -s MODULARIZE=1 -s EXPORT_NAME="createFlacModule" \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s EXPORTED_FUNCTIONS="$FUNCTIONS" \
  -s EXPORTED_RUNTIME_METHODS=HEAPU8,HEAP32,HEAPF32 \
  -o flac.js
