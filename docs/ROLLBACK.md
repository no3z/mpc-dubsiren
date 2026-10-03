# Plugin-only rollback

The real Force backup at
`/media/AKAI_SSD/Dub-Force-Siren-backups/1.0.0-before-1.0.1/` passed verification
against all **108** files in the original device-backup manifest
`resources/rollback-1.0.0.sha256`. Its binary SHA-256 is
`f2d92642e6a3b50824032c62f0c76ccfa8857153b9727898d0a75de239e2707a`.
The original full settings backup is retained separately for reference.

`tools/rollback_force.sh` restores only this plugin and its matching
registration, UID `44625372`. It verifies backup hashes before stopping `acvs`,
retains the current plugin/settings in a new timestamped backup, stages and swaps
the plugin folder, and updates only its registration. It verifies all other
settings and unrelated plugin registrations are unchanged. It does **not**
restore the entire old MPC.settings. Service restart is protected by a trap.

The read-only check passed on the real Force. Actual `--apply` passed in an
isolated filesystem fixture using both release packages and real settings;
rollback was **not** applied to the Force, which remains on 1.0.1. Published ZIP
source credits have since been refreshed; the script deliberately continues to
validate the original device backup, whose binary is unchanged.

From this repository on the computer, stage the three helper files:

```sh
ssh root@192.168.2.31 'mkdir -p /tmp/dub-force-rollback'
scp tools/rollback_force.sh resources/rollback-1.0.0.sha256 \
  vendor/mpc-vst-plugins/tools/release/plugin_list.awk \
  root@192.168.2.31:/tmp/dub-force-rollback/
ssh root@192.168.2.31 'sh /tmp/dub-force-rollback/rollback_force.sh --check'
```

Only when rollback is wanted, save the current Force project, then run:

```sh
ssh root@192.168.2.31 'sh /tmp/dub-force-rollback/rollback_force.sh --apply'
```

`--check` (also the default) writes only temporary scratch files. `--apply`
stops/restarts the Force application and restores 1.0.0. It does not load a
project automatically. Validate discovery and a FIRE after restarting.
Credentials are entered through SSH, never stored in these files.

To install the current 1.0.1 RC again **when needed**, use the retained ZIP and
the documented [installation procedure](INSTALL_FORCE.md). Its checksum is in
[REPRODUCIBILITY.md](REPRODUCIBILITY.md). No reinstallation is needed after this
audit because the installed payload is unchanged.

## 1.0.2 → 1.0.1

The original 1.0.1 device payload is retained at
`/media/AKAI_SSD/Dub-Force-Siren-backups/1.0.1-before-1.0.2/`. All **116** files
match `resources/rollback-1.0.1.sha256`; binary SHA-256:
`1c92678caafd8d4a9c50758f2ecf53d805d03579847df62e3dda277ebc190beb`.
The separate original settings snapshot is `MPC.settings-before-1.0.2`.

Stage `rollback_force.sh`, `plugin_list.awk` and `rollback-1.0.1.sha256` together,
then use `sh rollback_force.sh --check 1.0.1`. To restore when wanted, save the
project and use `sh rollback_force.sh --apply 1.0.1`. The version argument is
required for this backup; omitting it preserves the earlier 1.0.0 behavior.

Both original device backups pass read-only checks. A full 1.0.2→1.0.1 apply in
an isolated fixture restores all 116 hashes and preserves unrelated settings.
Verification ignores formatting whitespace between XML entries; it still checks
the contents of every unrelated entry and the rest of the settings tree.
Device rollback has not been performed. The 1.0.1 audit above is historical;
current deployment evidence is in [PERFORMANCE_1.0.2.md](PERFORMANCE_1.0.2.md).
