# Force verification — 1.0.1

Historical 1.0.1 record. Current 1.0.2 results and compatibility limits are in
[PERFORMANCE_1.0.2.md](PERFORMANCE_1.0.2.md).

The current build is installed at
`/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/` on the existing
Force at 192.168.2.31. Existing deployment tooling and target were reused.
MPC service is `acvs`; restart completed and PID 2156 was active afterward.
Startup log at 01:20:07 reports `Found, link: Dub Force Siren`.

Automated installed-path verification:

- ARM ELF32 hard-float, GLIBC at most 2.29; sole export `VSTPluginMain`.
- All 116 payload files match [installed hashes](../resources/installed-1.0.1.sha256).
  Hashes are relative to the installed plugin directory.
- Exactly one registration for uid 44625372; count stays 29. All 28 other plugin
  registration attribute sets are unchanged.
- Actual installed library opens in the device VST harness (2.6 ms), renders
  notes and passes the original all-parameter benchmark.
- Installed 2-second original workload: p99 11.2%, max 11.4%. With representative
  workloads and all 12 presets appended, worst p99 remains 11.2%.
- Installed 10-second original workload: p99 11.0%, max 11.8%, all-param mean 9.9%,
  p95 10.6%. Original baseline was p99 19.6%, max 20.8%. See PERFORMANCE.md.
- No plugin-specific loader/crash errors observed. Existing startup Sentry and
  MockbaMagic firmware-version messages are outside this plugin change.

The main deliverable is a real binary and native skin, not a standalone UI mock.
Native JSON/assets have been inspected and checked, but this does not prove actual
MPC editor interaction. At the final remote check the MPC process had not mapped
this plugin; the offline device harness had loaded it independently. Discovery
and harness loading are reported separately from selecting the instrument in MPC.

Direct framebuffer capture returned a black image. Actual Force UI screenshot,
physical touch, Q-Links, pad performance, real output listening and project
save/reload remain manual checks. Offline UI previews are in UI_REDESIGN.md.

## Backups and evidence

The old complete plugin folder is
`/media/AKAI_SSD/Dub-Force-Siren-backups/1.0.0-before-1.0.1/`; its binary hash
matches the original baseline. Pre-update settings are also in that backup
folder's parent as `MPC.settings-before-1.0.1`. The installer-created backup is
`/media/az01-internal/Settings/MPC/MPC.settings.bak-dub_force_siren-20261003-012001`.

Raw evidence remains under `build/performance/`: installed-profiles.txt,
installed-10s.txt, installed-file-checks.txt, settings-verification.txt,
backup-verification.txt, device-startup.txt and mpc-plugin-maps.txt.
Installer output is `build/device-update-install.txt`. No credentials are stored.
The initial build's device record is preserved in BASELINE_DEVICE_TEST.md.

## Physical checklist

1. Insert Dub Force Siren; verify main and utility panels, readable dynamic values,
   preset overlay, all five waves, all five echo characters, every primary fader.
2. Use FIRE, LATCH, MIDI pads, STOP, Freeze/Bend/Invert and feedback at hot values;
   listen for clicks, runaway feedback, incorrect silence or truncated tails.
3. Turn both Q-Link banks; confirm the complete main panel remains visible and
   mappings match UI_REDESIGN.md.
4. Save/reload a project and try a typical busy project. Device harness CPU does
   not measure MPC's own load or guarantee busy-project deadlines.
