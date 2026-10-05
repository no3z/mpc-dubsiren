# Dub Force Siren

[x] Inspect actual public BARZINE DSP/UI; record source findings.
[x] Inspect sd88me ABI, native UI, build and documented deployment.
[x] Consolidated architecture and ordered parameter contract.
[x] Smallest native oscillator VST2 builds; MIDI audio and state verified.
[x] Core siren, free-running nested LFOs, envelope, latch and ZAP/repeat.
[x] Chop, 8bit, LP/BP/HP, source/master EQ and modifier controls.
[x] Source-topology echo, feedback HP/LP, five actual character DSP modes.
[x] Lightweight stereo reverb and output protection.
[x] Complete primary touchscreen panel, Q-Link banks, original generated assets.
[x] Twelve integrated factory presets and versioned project state.
[x] 29sanitizer integration checks, effects tests, fuzz/concurrency and allocation checks.
[x] ARM hard-float build and dependency/entry-symbol verification.
[x] Portable package/checksums/installer checks.
[x] Install on Force; verify baseline payload files, settings, discovery and application restart.
[x] Installed-path device VST loading/MIDI processing/CPU and startup log checks.
[ ] Human native MPC editor touch/Q-Link/pad checks and listening.
[ ] Human project save/reload and busy-project audio test.
[ ] Optional browser/device capture comparison and tighter DSP matching.
[x] Profile and optimize CPU: installed storm p99 11.0%, PASS, below 12% target.
[x] Three inspected UI redesign iterations; native geometry and assets validated.
[x] Back up baseline plugin/settings, install 1.0.1 and verify 116 payload files.
[x] Release hardening: full state/UI/Q-Link/preset audits, level/DC checks and long stress.
[x] Two matching clean builds; fixed reproducible CPU metadata packaging.
[x] Verify real rollback backup and plugin-only rollback in an isolated fixture.
[x] Prepare numbered 10–15 minute physical Force test card.
[ ] Complete docs/MANUAL_FORCE_TEST_CARD.md on the actual Force.

[x] 1.0.2 deeper profiling/CPU optimization; installed storm p99 8.3%.
[ ] Hold <=8% storm p99 consistently in the ten-second follow-up (short run is 7.8%).
[x] Logarithmic 10–2400 Hz Pitch; base/ZAP floor audit and low-frequency extremes.
[x] Primary discrete named Wave Q-Link and secondary native factory Preset selector.
[x] Actual 1.0.1 chunk fixtures, rapid preset/wave safety and allocation tests.
[x] ARM/ZIP reproduced independently; 1.0.1 backup/rollback fixture and 1.0.2 installed verification.

[x] 2.0.0: remove reverb, echo characters and every control without a Q-Link.
[x] 2.0.0: sixteen Q-Links in knob order, grouped by function; single page mirroring both banks.
[x] 2.0.0: NEON/SSE block engine; SIMD/scalar render equivalence; suites pass under qemu ARM.
[x] 2.0.0: LFO Depth relative to Pitch, so Pitch works across 10–2400 Hz.
[x] 2.0.0: keyed DFS2 state; 37 real 1.0.1 DFS1 chunks migrate by key.
[ ] 2.0.0: back up 1.0.2 on the Force, install, and run docs/MANUAL_FORCE_TEST_CARD.md.
[ ] 2.0.0: device benchmark (docs/PERFORMANCE_2.0.0.md commands); add resources/force-bench-2.0.0.json.
[ ] 2.0.0: exercise `rollback_force.sh --check 1.0.2` against the real backup.
