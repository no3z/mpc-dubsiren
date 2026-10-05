# Force installation

The documented sd88me installer is used. Its account is `root`; target device
was verified over SSH as Akai Professional Force, armv7l, inMusic az01 Linux.
Existing settings register `/media/AKAI_SSD/Synths`, which is the confirmed
installation destination. `/sdcard/Synths` was absent and is not used here.
No credential is recorded in this project. Use an interactive SSH password
prompt or your own SSH agent. Do not place passwords in commands or scripts.

Build/test/package first, then copy and unpack the ZIP in a temporary folder.
Save the current Force project: installation stops and starts the `acvs`
application service, which is required to register a new plugin. Firmware,
boot files, network settings and other plugins are not changed.

Before upgrading 1.0.2 to 2.0.0, back up the installed plugin folder and settings
so [rollback](ROLLBACK.md) can restore it (`resources/rollback-1.0.2.sha256` lists
the 116 installed 1.0.2 files):

```sh
ssh root@192.168.2.31 'B=/media/AKAI_SSD/Dub-Force-Siren-backups; mkdir -p $B &&
  cp -a "/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren" $B/1.0.2-before-2.0.0 &&
  cp -a /media/az01-internal/Settings/MPC/MPC.settings $B/MPC.settings-before-2.0.0'
python3 -m zipfile -e dist/Dub-Force-Siren-2.0.0-mpc-armv7.zip build/install
scp -r build/install/Dub-Force-Siren-2.0.0 root@192.168.2.31:/tmp/
ssh root@192.168.2.31 sh /tmp/Dub-Force-Siren-2.0.0/install.sh -y -t /media/AKAI_SSD/Synths
```

The checksummed upstream installer validates root/armv7/settings, selects the
existing MPC service, stops it, stages the single plugin folder, backs up
`/media/az01-internal/Settings/MPC/MPC.settings`, adds this UID/file registration,
validates XML and uniqueness, and starts MPC again. Files are root-owned;
binary/JSON/PNG resources need read permissions, directories traversal permissions.
The loader opens the shared library directly; it does not require executing it
as a process. Settings backups remain for rollback. On upgrades, also back up
this plugin folder before replacement; the upstream installer itself replaces
its staging/old folders and preserves user-data only when configured.

Plugin folder: `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/`.
Select **Dub Force Siren** in the instrument/plugin browser. Its single page
mirrors the Q-Links: the top row is bank 1 (knobs 1–8), the bottom row bank 2
(knobs 9–16). Projects saved with 1.0.x reopen with their state migrated;
automation recorded against 1.0.x parameter indices does not carry over.

Uninstall only this plugin using the generated `uninstall.sh` in the unpacked
package (it also stops/starts MPC). Review its destination/settings before
running. To restore the previous plugin, use the verified
[plugin-only rollback script](ROLLBACK.md), which updates only this registration
and preserves unrelated current settings.

The tested device Python lacks zlib, so unzip locally before SCP. Do not install
extra device packages just to unpack the ZIP. The installer verifies all copied
file checksums before stopping MPC.

2.0.0 was first installed on the Force on 2026-10-05 with the commands above (UID `44625372`). The 1.0.2 backup for this update is
`/media/AKAI_SSD/Dub-Force-Siren-backups/1.0.2-before-2.0.0/`, settings copy
`MPC.settings-before-2.0.0`. Still to do by hand: the
[test card](MANUAL_FORCE_TEST_CARD.md) (including the low-Pitch test) and a CPU
check inside a real project.

On 2026-10-05 the Force was updated to the 2.0.0 build with the fidelity fixes in
[FIDELITY.md](FIDELITY.md) (binary `c1f77d7f…f912`; the first 2.0.0 build, `87129103…7bc0`, ran
for a few hours). The 66 installed files match the package (`resources/installed-2.0.0.sha256`),
the plugin is registered once and MPC linked it. The first build is kept at
`/media/AKAI_SSD/Dub-Force-Siren-backups/2.0.0-first-build-before-fidelity/` with its settings
copy `MPC.settings-before-fidelity` (and `MPC.settings.bak-dub_force_siren-20261005-201000`
beside the live file); `rollback_force.sh` does not cover it, restore it by hand if ever needed.

The 1.0.0 folder was saved at
`/media/AKAI_SSD/Dub-Force-Siren-backups/1.0.0-before-1.0.1/` before this upgrade.
The matching pre-update settings copy is
`/media/AKAI_SSD/Dub-Force-Siren-backups/MPC.settings-before-1.0.1`.
The installer also saved
`MPC.settings.bak-dub_force_siren-20261003-012001` beside the live settings file.

The complete 1.0.1 backup for this update is at
`/media/AKAI_SSD/Dub-Force-Siren-backups/1.0.1-before-1.0.2/`, with all 116
original file hashes in `resources/rollback-1.0.1.sha256`. Its separate settings
copy is `MPC.settings-before-1.0.2` in the backups parent. Restore only this plugin
with `rollback_force.sh --apply 1.0.1`; check it first with `--check 1.0.1`.
