# 1.0.1 release hardening — 2026-10-03

Historical 1.0.1 record. Current 1.0.2 results and compatibility limits are in
[PERFORMANCE_1.0.2.md](PERFORMANCE_1.0.2.md).

The installed release candidate remains **1.0.1**, untouched. No DSP, UI,
parameter, preset or device configuration change was justified. One packaging
reproducibility issue was fixed: CPU metadata now comes from a checked-in verified
file rather than optional ignored build output. Clean packages match the original
ZIP exactly; no rebuild deployment or version churn was required.

The published ZIPs subsequently received refreshed source credits and checksums.
Their binaries are unchanged. Current download hashes are recorded in
[REPRODUCIBILITY.md](REPRODUCIBILITY.md); installed-device manifests continue to
refer to the original, untouched payload.

Final read-only device verification: **all 116 installed file checksums still
match** the baseline manifest; the binary retains the recorded SHA-256, `acvs`
is active and MPC remains PID 2156. No application restart occurred in this audit.

| Audit | Result / remaining limit |
| --- | --- |
| State | 256 randomized fresh/in-place round trips and 12 factory states; 26,724 parameter comparisons pass. Transient events/audio history excluded intentionally. Actual Force project reload pending. |
| UI / Q-Links | 77 visible controls, all 51 public IDs, complete enum options, both main Q-Link sets and Utility verified from actual native files/source. No wrong/reversed/missing bindings found. |
| Finger review | Main/Utility previews inspected; FIRE 140×64, LATCH 60×64. Auxiliary tracks as small as 92×20 require physical touch checks. No proven defect justifies another redesign. |
| Presets | All 12 finite, in range, different snapshots and audible in automated renders; source export equals checked-in presets. Maximum 8-second preset peak .435087, no hard-bound frames. Subjective quality pending listening. |
| Output | Eight 30-second maximum-control cases finite, peak ≤.523295 with no hard-bound frames. Random stress peak .98; 1,096 / 79,379,968 frames reached the existing bound (0.00138%). No runaway found. |
| Production stress | 30 minutes simulated audio, 620,156 blocks, 79,379,968 frames, 74.168 seconds wall time. No crash/nonfinite output/state mismatch. RSS +128 KiB; sampled 2,440–2,472 pages. |
| Sanitizer stress | 10 minutes simulated audio, 206,718 blocks; ASan/UBSan/leak detection pass, plus existing 29 integration assertions and effects tests. RSS +768 KiB under sanitizer runtime; no leak report. |
| Build | Two clean source copies produce identical ARM and ZIP hashes matching installed 1.0.1. Reproducibility scoped to recorded compiler/fonts/Pillow. |
| Rollback | Real backup's 108 files verified; real-device read-only check and isolated full apply pass. Other registrations/settings preserved; device rollback not performed. |

**DC investigation:** an extreme deeply modulated/crushed/chopped case had a
10-second mean near −.031. Four follow-up 180-second renders showed window means
changing sign; full means were roughly −.002 for the modulated case and +.000008
with modulation/chop disabled. Removing crush did not remove the excursion.
This points to low-frequency bias under extreme FM rather than stationary DC;
it does not prove absence of every possible offset. Random long-run means were
L +.000008 / R +.000005. Keep extreme low-frequency behavior in physical
listening review; output protection and desired feedback were left intact.

Delay wrap/rapid modulation and invalid indices remain covered by existing
effects/fuzz tests; stress additionally restores and compares 51 physical targets
at 152 production checkpoints. Callback allocation/free checks remain zero.
Memory measurements are small bounded changes over these runs, not a proof of
unlimited-duration memory stability. Desktop renders do not substitute for a
30-minute realtime hardware soak or the native editor.

Evidence and repeatable commands:

- [State audit](STATE_AUDIT.md), [77-control wiring table](UI_WIRING_AUDIT.md),
  [12-preset values and levels](PRESET_AUDIT.md).
- [Clean build commands/hashes](REPRODUCIBILITY.md), [safe rollback](ROLLBACK.md).
- Tests: `tests/rc_audit.cpp`, `tools/rc_audit.sh`, `tools/audit_ui.py`.
- Local logs: `build/rc-audit/stress-release.txt`, `stress.txt`, `sanitizers.txt`,
  `dc-probe.txt`, `rollback-device.txt`, `rollback-fixture.txt`.

**Next step:** Complete the numbered
[10–15 minute manual Force test card](MANUAL_FORCE_TEST_CARD.md), especially
finger alignment, primary Q-Links, listening and actual project save/reload.
Those checks remain pending; no physical result has been inferred from harnesses.
