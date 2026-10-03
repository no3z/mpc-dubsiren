#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build/rc-audit
./tools/build.sh test > build/rc-audit/sanitizers.txt
COMMON=(-O1 -g -std=c++17 -Ibuild -Isrc -Itests -Ivendor/mpc-vst-plugins/wrapper -fsanitize=address,undefined -fno-omit-frame-pointer)
g++ "${COMMON[@]}" tests/rc_audit.cpp build/test/{wrapper,siren,engine,effects}.o -lm -o build/rc-audit/rc_audit
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 \
 build/rc-audit/rc_audit "${1:-1800}" | tee build/rc-audit/stress.txt
