#!/usr/bin/env bash
# ARM statistical diagnostic only; release DSP sources have no instrumentation.
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build/performance
python3 vendor/mpc-vst-plugins/tools/gen_vst.py vst.json --params-h
TASK_IMAGE="${DUB_ARM_IMAGE:-mnm-armhf-builder-glibc231:latest}"
docker run --rm --user "$(id -u):$(id -g)" -v "$PWD:/work" -w /work "$TASK_IMAGE" \
 arm-linux-gnueabihf-g++ -O3 -flto -g -no-pie -std=c++17 -fno-math-errno \
 -mcpu=cortex-a17 -mfpu=neon-vfpv4 -mfloat-abi=hard -Isrc -Ibuild \
 tools/profile_sample.cpp src/siren.cpp src/effects.cpp -lm -ldl -o build/performance/sample-arm
printf 'Copy sample-arm to Force /tmp; run with audio duration (120 default). Save stdout to samples.txt.\n'
printf 'Use matching ARM addr2line on executable PCs; library names in stdout expose math/libc costs.\n'
printf 'For wrapper profiling, build.sh arm first, then link profile_sample.cpp with build/arm objects and -DPROFILE_WRAPPER.\n'
