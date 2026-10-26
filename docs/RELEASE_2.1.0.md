# Dub Sirens 2.1.0: Force installation and acceptance

Fixes issue [#1](https://github.com/no3z/mpc-dubsiren/issues/1) and adds the twelve
factory sounds to the native PRESET menu in the top bar. The native menu selects the existing factory snapshots; the duplicate PATCH
control has been removed from the panel and Q-Link 15 is unassigned. The UID, 20 parameter indices, hidden popup
index and DFS2 state remain compatible with 2.0.0. Selecting a different factory
preset resets sound controls and releases LATCH, as in the previous factory selector.

This ARMv7 candidate is installed on the Force; manual acceptance is pending. Do not mark it stable,
close the issue or add a `tested.json` hardware claim until the checklist passes.
Requires a modded Force with root access and a 44.1 kHz host. The IP and Synths
path below are the previously documented device; substitute them if changed.

## Candidate revision after the first Force installation

WAVE and LFO SHAPE now use upstream direction-aware enum settling: a continuous
ascending or descending input no longer alternates between adjacent options.
Repeated inputs stay stable, exact selections still work and Q-Link turns can
step in either direction. Host-position history resets on factory preset and
chunk restore. Q-Link 15 is unassigned; the PRESET parameter retains index 14,
and OUTPUT remains on Q-Link 16. Presets are available only in the native top-bar menu; the duplicate PATCH
control and popup are removed. The hidden popup parameter remains reserved
for ABI compatibility. Installed on the Force on 2026-10-26 after backing up the first 2.1.0 candidate
at `/media/AKAI_SSD/Dub-Force-Siren-backups/2.1.0-before-enum-fix-20261026-135039/`.
The subsequent UI-only update was backed up at
`2.1.0-before-remove-patch-20261026-135258/` in the same backup directory.
The installer restarted `acvs`; installed payload hashes match the revised ZIP.
This verifies deployment only: physical control and listening retests are pending.

## Name and extended pitch range

The current candidate is **Dub Sirens**. UID `44625372`, binary
`dub_force_siren.so`, catalog id `dub-force-siren`, skin content identifier and
parameter indices remain unchanged. The new skin folder is
`Dub Force - VST - Dub Sirens`. After validating registration and the copied
binary, the installer moves the previous `Dub Force - VST - Dub Force Siren`
folder outside Synths to `/media/AKAI_SSD/plugin-rename-backups/`. It checks the
old folder's UID first and refuses to move another plugin. Reinstallation is
idempotent; no second plugin entry is created. The rename fixture tests this,
including preservation of user files and unrelated registrations.

The renamed candidate was installed on 2026-10-26. Its 42 payload files match
the ZIP, `acvs` is active, and exactly one `Dub Sirens` registration points to
the new folder. The old folder was archived at
`/media/AKAI_SSD/plugin-rename-backups/Dub Force - VST - Dub Force Siren-20261026-140223-3057/`.
The pre-rename copy and settings also remain in the existing backup directory
as `2.1.0-before-dub-sirens-20261026-140221`.

PITCH now spans **10–8000 Hz** logarithmically. With DEPTH at 100%, siren
modulation reaches 16 kHz, below the oscillator's 17.64 kHz frequency clamp at
44.1 kHz. Existing factory presets and physical-Hz DFS2 states retain their
values. Existing normalized PITCH automation changes scale; review it before
reusing a project. Upper-range host tests include 4, 6 and 8 kHz.

## Offline validation (2026-10-26)

- Host integration, effects and control suites: ASan/UBSan clean; zero intercepted
  heap allocations/frees during rendering.
- Port host test: PASSED, including 32 plain writes with zero UpdateDisplay calls.
- Isolated explicit-refresh/stepper fixture: PASSED; native program notifications
  are deferred and consumed once.
- All twelve native programs match existing factory snapshots; menu readback,
  preset-parameter synchronization, custom edits and chunk restore pass on host and ARM.
- ARM integration, controls and effects pass under QEMU user emulation. ARM ELF32,
  hard-float, maximum GLIBC_2.29, sole public export VSTPluginMain.
- SIMD/scalar rendering: 3,175,200 samples, maximum difference 0 LSB.
- Skin audit: 19 panel controls, 15 assigned Q-Links, 27 touch regions. Catalog check passes; one expected warning identifies the reviewed rename-aware installer.
- Package payload is byte-identical to the tested release ARM binary. ZIP SHA-256:
  `2cecc2fb8e27a89685a0056be090a6a24ef72d4b6c6888a53c829d6990e3d401`.

These are offline results; hardware menu appearance, physical Q-Links, listening
and real-project CPU remain for the acceptance checklist below.

## Install the candidate

Save the current project first: the installer restarts the Force application.
On the computer, from the repository root:

```sh
sha256sum -c dist/Dub-Sirens-2.1.0-mpc-armv7.zip.sha256
ssh root@192.168.2.31 'set -eu
B=/media/AKAI_SSD/Dub-Force-Siren-backups
mkdir -p "$B"
test ! -e "$B/2.0.0-before-2.1.0"
test ! -e "$B/MPC.settings-before-2.1.0"
cp -a "/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren" "$B/2.0.0-before-2.1.0"
cp -a /media/az01-internal/Settings/MPC/MPC.settings "$B/MPC.settings-before-2.1.0"'
python3 -m zipfile -e dist/Dub-Sirens-2.1.0-mpc-armv7.zip build/install-2.1.0
scp -r build/install-2.1.0/Dub-Sirens-2.1.0 root@192.168.2.31:/tmp/
ssh root@192.168.2.31 sh /tmp/Dub-Sirens-2.1.0/install.sh -y -t /media/AKAI_SSD/Synths
```

Unzip on the computer: the documented Force Python has no zlib. Never overwrite
an existing backup. If the backup command reports an existing folder, retain it
and check that it is your original 2.0.0 backup before continuing.

## Test on the Force

Record the Force firmware version and test date alongside your results.

1. Load Dub Sirens. Open the top-bar **PRESET** selector: it must list
   Air Raid, Laser, Fog Horn, Police, UFO, Space Echo, Smoke, Clash, Heavy Dub,
   Deep Orbit, Feedback Madness and Sci-Fi Alarm. Select each and press FIRE.
2. Verify PATCH is absent from the plugin panel. Q-Link 15 must remain unassigned; OUTPUT stays on 16. Select sounds from the top bar and check control values. Change a sound control, save, close and reload the project:
   the edit and selected preset must survive.
3. Open **Track Edit → Automation**, select plugin parameters, and move physical
   Q-Links in both banks. Values must move continuously without snapping back.
   Record movements, then replay the automation and check the sound follows.
4. Return to the plugin panel. Check both banks, FIRE, LATCH,
   SIREN/ZAP and STOP. Load a copy of an existing 2.0.0 project and verify it.
5. Check playback and CPU in a real project. Report audible glitches or overloads;
   the historical 2.0.0 benchmark is not a measurement of this candidate.

## Restore 2.0.0 if needed

Save the project. Use the retained, checksummed 2.0.0 release with its own
installer to restore only this plugin's registration and payload:

```sh
python3 -m zipfile -e dist/Dub-Force-Siren-2.0.0-mpc-armv7.zip build/restore-2.0.0
scp -r build/restore-2.0.0/Dub-Force-Siren-2.0.0 root@192.168.2.31:/tmp/
ssh root@192.168.2.31 sh /tmp/Dub-Force-Siren-2.0.0/install.sh -y -t /media/AKAI_SSD/Synths
```

The original installed folder and settings backup remain available separately.
Do not replace the full live settings file: it may contain unrelated newer edits.
The older `rollback_force.sh` supports 1.0.x only; use the installer above for
2.0.0. Your saved 2.0.0 projects keep the same UID and parameter mapping.

When restoring 2.0.0 after the rename, move the `Dub Force - VST - Dub Sirens`
folder outside Synths too: the old 2.0.0 installer cannot recognize the newer
folder name. Save your project and stop `acvs` while moving it; use the old
installer with `-n` while stopped, and restart `acvs` afterwards. Keep both
backups until the restored plugin loads successfully.

## Catalog rename

The listing change is committed to `no3z/mpc-vst-plugins` on branch
`catalog/dub-sirens-2.1.0` (registry check: 152 entries, zero problems).
It changes the display name and description, preserving the existing id and
repo. The GitHub connector returned 403 when attempting an upstream draft PR.
[Open the prepared catalog PR](https://github.com/sd88me/mpc-vst-plugins/compare/main...no3z:mpc-vst-plugins:catalog/dub-sirens-2.1.0?expand=1).
Keep it in draft until stable v2.1.0 exists, because its screenshot URL uses
that tag.

## Publish after acceptance

Publish tag `v2.1.0` from the reviewed fix with exactly the tested ZIP and its
SHA-256. Include installation requirements and hardware results in the notes.
Add `tested.json` only with the actual firmware, device and date reported above.
Update `catalog/plugins/dub-force-siren.json` in sd88me/mpc-vst-plugins so its
screenshot points to `v2.1.0/resources/preview.png`; link the release and tests
in that PR. The existing catalog listing discovers stable releases automatically
on its next nightly build; no version/checksum fields belong in the registry.
Reply to @sd88me in issue #1 with the release and catalog PR links and close it
once the published fix is verified. Retain the 2.0.0 release for rollback.
