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
  # Scalar FP is not contracted: with LTO, automatic FMA fusion differs per
  # inlined call site. NEON FMA in src/simd.h is explicit and unaffected.
  local common=(-O3 -flto -fno-math-errno -ffp-contract=off -g -Wall -Wextra -Wno-unused-parameter -fPIC -fvisibility=hidden
    -DDUB_TYPED_PARAMETERS -Ibuild -Isrc -Ivendor/mpc-vst-plugins/wrapper)
  local special=() checks=0
  if [[ "$target" == test || "$target" == armtest ]]; then checks=1; fi
  if [[ "$target" == test ]]; then
    common=(-O1 -ffp-contract=off -g -Wall -Wextra -Wno-unused-parameter -fPIC
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
  # ARM checks run the release-optimized NEON code under qemu-user (binfmt).
  if [[ "$target" == armtest ]]; then export QEMU_LD_PREFIX=/usr/arm-linux-gnueabihf; fi
  "$cxx" -shared "${link_opt[@]}" "${special[@]}" "${arch[@]}" "${objects[@]}" -lm \
    -Wl,--no-undefined -o "$out/dub_force_siren.so"
  if (( checks )); then
    "$cxx" -std=c++17 "${common[@]}" "${special[@]}" "${arch[@]}" -c tests/integration.cpp -o "$out/integration.o"
    "$cxx" "${special[@]}" "${arch[@]}" "${objects[@]}" "$out/integration.o" -lm \
      -pthread -Wl,--wrap=malloc,--wrap=calloc,--wrap=realloc,--wrap=free -o "$out/integration_test"
    ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 "$out/integration_test"
    "$cxx" -std=c++17 "${common[@]}" "${special[@]}" "${arch[@]}" -Itests tests/performance_controls.cpp \
      "${objects[@]}" -lm -ldl -o "$out/performance_controls"
    ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 "$out/performance_controls"
    if [[ -f src/effects.cpp && -f tests/effects_test.cpp ]]; then
      "$cxx" -std=c++17 "${common[@]}" "${special[@]}" "${arch[@]}" src/effects.cpp tests/effects_test.cpp \
        -lm -o "$out/effects_test"
      ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 UBSAN_OPTIONS=halt_on_error=1 "$out/effects_test"
    fi
    if [[ "$target" == test ]]; then
      # The SIMD build and the portable scalar build render the same presets.
      local variant
      for variant in simd scalar; do
        "$cxx" -std=c++17 -O2 -ffp-contract=off -Ibuild -Isrc $([[ $variant == scalar ]] && echo -DDUB_SIMD_SCALAR) \
          tools/render_demo.cpp src/siren.cpp src/effects.cpp -lm -o "$out/render_$variant"
        "$out/render_$variant" "$out/render_$variant.wav"
      done
      python3 tools/compare_wav.py "$out/render_simd.wav" "$out/render_scalar.wav" 2
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
  arm-test) docker run --rm --user "$(id -u):$(id -g)" -v "$ROOT:/work" -w /work \
    "$ARM_IMAGE" bash tools/build.sh arm-test-internal ;;
  arm-test-internal) build_target armtest arm-linux-gnueabihf-gcc arm-linux-gnueabihf-g++ \
    '-mcpu=cortex-a17 -mfpu=neon-vfpv4 -mfloat-abi=hard' ;;
  all) bash tools/build.sh host; bash tools/build.sh test; bash tools/build.sh arm; bash tools/build.sh arm-test ;;
  *) printf 'Usage: %s [host|test|arm|arm-test|all]\n' "$0" >&2; exit 2 ;;
esac
