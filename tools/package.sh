#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
[[ -f build/arm/dub_force_siren.so ]] || { echo 'Run tools/build.sh arm first' >&2; exit 1; }
./tools/skin.sh
mkdir -p build/release-notices
cp NOTICE.md VENDORED.md build/release-notices/
cp vendor/mpc-vst-plugins/tools/vendor/force-shadow/LICENSE build/release-notices/force-shadow-LICENSE.txt
BENCH_ARGS=()
if [[ -f resources/force-bench-1.0.2.json ]]; then BENCH_ARGS=(--bench resources/force-bench-1.0.2.json); fi
python3 vendor/mpc-vst-plugins/tools/release.py \
    --so build/arm/dub_force_siren.so \
    --skin 'build/skin/Dub Force - VST - Dub Force Siren' \
    --entry build/pluginlist-entry.xml --version 1.0.2 \
    --id dub-force-siren \
    --repo no3z/mpc-dubsiren \
    --extra presets:presets --extra build/release-notices:notices \
    --about 'Native BARZINE-derived dub siren, five echo characters and one-panel MPC performance controls.' \
    "${BENCH_ARGS[@]}" -o dist
python3 vendor/mpc-vst-plugins/tools/catalog_check.py dist/Dub-Force-Siren-1.0.2-mpc-armv7.zip
