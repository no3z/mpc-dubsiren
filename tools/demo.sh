#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p build
python3 vendor/mpc-vst-plugins/tools/gen_vst.py vst.json --params-h
for name in export_presets render_demo; do
    g++ -std=c++17 -O2 -Ibuild -Isrc "tools/$name.cpp" src/siren.cpp src/effects.cpp -lm -o "build/$name"
done
./build/export_presets > presets/factory.json
./build/render_demo build/demo.wav
printf 'Generated presets/factory.json and build/demo.wav (12 presets, 36 seconds)\n'
