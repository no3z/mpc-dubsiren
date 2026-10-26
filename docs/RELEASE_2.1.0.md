# 2.1.0 candidate: Force installation and acceptance

Fixes issue [#1](https://github.com/no3z/mpc-dubsiren/issues/1) and adds the twelve
factory sounds to the native PRESET menu in the top bar. PATCH and that menu
select the same factory snapshots. The UID, 20 parameter indices, hidden popup
index and DFS2 state remain compatible with 2.0.0. Selecting a different factory
preset resets sound controls and releases LATCH, as PATCH already does.

This ARMv7 candidate has not yet been tested on a Force. Do not mark it stable,
close the issue or add a `tested.json` hardware claim until the checklist passes.
Requires a modded Force with root access and a 44.1 kHz host. The IP and Synths
path below are the previously documented device; substitute them if changed.

## Offline validation (2026-10-26)

- Host integration, effects and control suites: ASan/UBSan clean; zero intercepted
  heap allocations/frees during rendering.
- Port host test: PASSED, including 32 plain writes with zero UpdateDisplay calls.
- Isolated explicit-refresh/stepper fixture: PASSED; native program notifications
  are deferred and consumed once.
- All twelve native programs match existing factory snapshots; menu readback,
  PATCH synchronization, custom edits and chunk restore pass on host and ARM.
- ARM integration, controls and effects pass under QEMU user emulation. ARM ELF32,
  hard-float, maximum GLIBC_2.29, sole public export VSTPluginMain.
- SIMD/scalar rendering: 3,175,200 samples, maximum difference 0 LSB.
- Skin audit: 20 controls, 16 Q-Links, 28 touch regions. Catalog check: zero warnings.
- Package payload is byte-identical to the tested release ARM binary. ZIP SHA-256:
  `08a90cc4c727be0331012a83c4280b1e6c4faf97cfff154494992601439b7e76`.

These are offline results; hardware menu appearance, physical Q-Links, listening
and real-project CPU remain for the acceptance checklist below.

## Install the candidate

Save the current project first: the installer restarts the Force application.
On the computer, from the repository root:

```sh
sha256sum -c dist/Dub-Force-Siren-2.1.0-mpc-armv7.zip.sha256
ssh root@192.168.2.31 'set -eu
B=/media/AKAI_SSD/Dub-Force-Siren-backups
mkdir -p "$B"
test ! -e "$B/2.0.0-before-2.1.0"
test ! -e "$B/MPC.settings-before-2.1.0"
cp -a "/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren" "$B/2.0.0-before-2.1.0"
cp -a /media/az01-internal/Settings/MPC/MPC.settings "$B/MPC.settings-before-2.1.0"'
python3 -m zipfile -e dist/Dub-Force-Siren-2.1.0-mpc-armv7.zip build/install-2.1.0
scp -r build/install-2.1.0/Dub-Force-Siren-2.1.0 root@192.168.2.31:/tmp/
ssh root@192.168.2.31 sh /tmp/Dub-Force-Siren-2.1.0/install.sh -y -t /media/AKAI_SSD/Synths
```

Unzip on the computer: the documented Force Python has no zlib. Never overwrite
an existing backup. If the backup command reports an existing folder, retain it
and check that it is your original 2.0.0 backup before continuing.

## Test on the Force

Record the Force firmware version and test date alongside your results.

1. Load Dub Force Siren. Open the top-bar **PRESET** selector: it must list
   Air Raid, Laser, Fog Horn, Police, UFO, Space Echo, Smoke, Clash, Heavy Dub,
   Deep Orbit, Feedback Madness and Sci-Fi Alarm. Select each and press FIRE.
2. Change PATCH by touch and Q-Link. Check native program readback, sound and
   control values. Change a sound control, save, close and reload the project:
   the edit and selected preset must survive.
3. Open **Track Edit → Automation**, select plugin parameters, and move physical
   Q-Links in both banks. Values must move continuously without snapping back.
   Record movements, then replay the automation and check the sound follows.
4. Return to the plugin panel. Check both banks, PATCH popup, FIRE, LATCH,
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
