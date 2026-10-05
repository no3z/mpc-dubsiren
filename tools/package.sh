#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
[[ -f build/arm/dub_force_siren.so ]] || { echo 'Run tools/build.sh arm first' >&2; exit 1; }
./tools/skin.sh
mkdir -p build/release-notices
cp NOTICE.md VENDORED.md build/release-notices/
cp vendor/mpc-vst-plugins/tools/vendor/force-shadow/LICENSE build/release-notices/force-shadow-LICENSE.txt
BENCH_ARGS=()
# Only device-measured benchmarks are published with a release.
if [[ -f resources/force-bench-2.0.0.json ]]; then BENCH_ARGS=(--bench resources/force-bench-2.0.0.json); fi
python3 vendor/mpc-vst-plugins/tools/release.py \
    --so build/arm/dub_force_siren.so \
    --skin 'build/skin/Dub Force - VST - Dub Force Siren' \
    --entry build/pluginlist-entry.xml --version 2.0.0 \
    --id dub-force-siren \
    --repo no3z/mpc-dubsiren \
    --extra presets:presets --extra build/release-notices:notices \
    --about 'Native BARZINE-derived dub siren with tape echo; one page mirroring two banks of eight Q-Links.' \
    "${BENCH_ARGS[@]}" -o dist
python3 vendor/mpc-vst-plugins/tools/catalog_check.py dist/Dub-Force-Siren-2.0.0-mpc-armv7.zip
