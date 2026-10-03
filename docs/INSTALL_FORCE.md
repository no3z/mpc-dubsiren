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

```sh
python3 -m zipfile -e dist/Dub-Force-Siren-1.0.1-mpc-armv7.zip build/install
scp -r build/install/Dub-Force-Siren-1.0.1 root@192.168.2.31:/tmp/
ssh root@192.168.2.31 sh /tmp/Dub-Force-Siren-1.0.1/install.sh -y -t /media/AKAI_SSD/Synths
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
Select **Dub Force Siren** in the instrument/plugin browser. The main screen
contains the complete sound path. Both primary/secondary Q-Link banks
reuse the same screen. UTILITY contains EQ and optional modifiers.

Uninstall only this plugin using the generated `uninstall.sh` in the unpacked
package (it also stops/starts MPC). Review its destination/settings before
running. To restore the previous plugin, use the verified
[plugin-only rollback script](ROLLBACK.md), which updates only this registration
and preserves unrelated current settings.

The tested device Python lacks zlib, so unzip locally before SCP. Do not install
extra device packages just to unpack the ZIP. The installer verifies all copied
file checksums before stopping MPC.

The 1.0.0 folder was saved at
`/media/AKAI_SSD/Dub-Force-Siren-backups/1.0.0-before-1.0.1/` before this upgrade.
The matching pre-update settings copy is
`/media/AKAI_SSD/Dub-Force-Siren-backups/MPC.settings-before-1.0.1`.
The installer also saved
`MPC.settings.bak-dub_force_siren-20261003-012001` beside the live settings file.
