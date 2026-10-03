#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"
MODE="${1:-host}"
ARM_IMAGE="${DUB_ARM_IMAGE:-mnm-armhf-builder-glibc231:latest}"
SOURCES=(src/siren.cpp src/engine.cpp)
if [[ -f src/effects.cpp ]]; then SOURCES+=(src/effects.cpp); fi
mkdir -p build
python3 vendor/mpc-vst-plugins/tools/gen_vst.py vst.json --params-h

build_target() {
  local target="$1" cc="$2" cxx="$3" extra="$4"
  local out="build/$target"
  mkdir -p "$out"
  local common=(-O3 -flto -fno-math-errno -g -Wall -Wextra -Wno-unused-parameter -fPIC -fvisibility=hidden
    -DDUB_TYPED_PARAMETERS -Ibuild -Isrc -Ivendor/mpc-vst-plugins/wrapper)
  local special=()
  if [[ "$target" == test ]]; then
    common=(-O1 -g -Wall -Wextra -Wno-unused-parameter -fPIC
      -DDUB_TYPED_PARAMETERS -Ibuild -Isrc -Ivendor/mpc-vst-plugins/wrapper)
    special=(-fsanitize=address,undefined -fno-omit-frame-pointer)
  fi
  local arch=()
  if [[ -n "$extra" ]]; then read -r -a arch <<< "$extra"; fi
  "$cc" -std=gnu11 "${common[@]}" "${special[@]}" "${arch[@]}" -c \
    vendor/mpc-vst-plugins/wrapper/vst2_wrap.c -o "$out/wrapper.o"
  local objects=("$out/wrapper.o") source object
  for source in "${SOURCES[@]}"; do
    object="$out/$(basename "${source%.*}").o"
    "$cxx" -std=c++17 "${common[@]}" "${special[@]}" "${arch[@]}" -c "$source" -o "$object"
    objects+=("$object")
  done
  local link_opt=(-O3 -flto)
  if [[ "$target" == test ]]; then link_opt=(); fi
  "$cxx" -shared "${link_opt[@]}" "${special[@]}" "${arch[@]}" "${objects[@]}" -lm \
    -Wl,--no-undefined -o "$out/dub_force_siren.so"
  if [[ "$target" == test ]]; then
    "$cxx" -std=c++17 "${common[@]}" "${special[@]}" -c tests/integration.cpp -o "$out/integration.o"
    "$cxx" "${special[@]}" "${objects[@]}" "$out/integration.o" -lm \
      -pthread -Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc,--wrap=free -o "$out/integration_test"
    ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 "$out/integration_test"
    "$cxx" -std=c++17 "${common[@]}" "${special[@]}" -Itests tests/performance_controls.cpp \
      "${objects[@]}" -lm -ldl -o "$out/performance_controls"
    ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 "$out/performance_controls"
    if [[ -f src/effects.cpp && -f tests/effects_test.cpp ]]; then
      "$cxx" -std=c++17 "${common[@]}" "${special[@]}" src/effects.cpp tests/effects_test.cpp \
        -lm -o "$out/effects_test"
      ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 "$out/effects_test"
    fi
  fi
  if [[ "$target" == arm ]]; then
    arm-linux-gnueabihf-strip "$out/dub_force_siren.so"
    arm-linux-gnueabihf-readelf -h "$out/dub_force_siren.so" | sed -n '/Class:/p;/Machine:/p;/Flags:/p'
    local highest
    highest="$(arm-linux-gnueabihf-readelf -V "$out/dub_force_siren.so" | \
      grep -o 'GLIBC_[0-9.]\+' | sort -Vu | tail -1)"
    printf 'Highest required glibc: %s\n' "$highest"
    python3 - "$highest" <<'PY'
import sys
version = tuple(map(int, sys.argv[1].removeprefix('GLIBC_').split('.')))
if version > (2, 32):
    raise SystemExit('ARM artifact requires glibc newer than supported 2.32')
PY
    arm-linux-gnueabihf-readelf --dyn-syms -W "$out/dub_force_siren.so" | \
      awk '$5=="GLOBAL" && $7!="UND" && $8!="" {print "Export:", $8}'
  fi
  printf 'Built %s/dub_force_siren.so\n' "$out"
}

case "$MODE" in
  host) build_target host gcc g++ "" ;;
  test) build_target test gcc g++ "" ;;
  arm) docker run --rm --user "$(id -u):$(id -g)" -v "$ROOT:/work" -w /work \
    "$ARM_IMAGE" bash tools/build.sh arm-internal ;;
  arm-internal) build_target arm arm-linux-gnueabihf-gcc arm-linux-gnueabihf-g++ \
    '-mcpu=cortex-a17 -mfpu=neon-vfpv4 -mfloat-abi=hard' ;;
  all) bash tools/build.sh host; bash tools/build.sh test; bash tools/build.sh arm ;;
  *) printf 'Usage: %s [host|test|arm|all]\n' "$0" >&2; exit 2 ;;
esac
