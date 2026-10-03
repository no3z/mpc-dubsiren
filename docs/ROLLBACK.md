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
settings and the other 28 plugin registrations are unchanged. It does **not**
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
