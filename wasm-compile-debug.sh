#!/usr/bin/env bash
set -euo pipefail
OPT="-O0 -g" bash "$(dirname "${BASH_SOURCE[0]}")/wasm-compile.sh"
