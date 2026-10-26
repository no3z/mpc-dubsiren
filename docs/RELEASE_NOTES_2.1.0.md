Dub Force Siren 2.1.0 fixes physical Q-Links on Track Edit → Automation and unstable WAVE/LFO SHAPE selection. All twelve factory sounds are now available in the native top-bar PRESET selector. The duplicate PATCH panel is removed, Q-Link 15 is unassigned and OUTPUT remains on 16.

PITCH now spans 10 Hz–8 kHz logarithmically; with full LFO depth the siren sweep reaches 16 kHz. Existing normalized PITCH automation changes scale: review it before reusing old projects. Factory sounds and physical-Hz saved states retain their values. The plugin name, UID, binary, parameter indices and standard upstream installer are preserved.

Tested by the maintainer on Akai Force, software 3.9.1.2, on 2026-10-26. The maintainer confirmed the final installed candidate works, including Q-Links, WAVE/SHAPE, native presets and project reload. Host ASan/UBSan suites, ARM tests under QEMU, SIMD/scalar equivalence, UI audit and catalog validation also pass. This is the exact tested ZIP, not a rebuild; no new hardware CPU benchmark is claimed.

### Requirements

Modded standalone Force/MPC with root access, ARMv7 hard-float and a 44.1 kHz host. This package requires no newer than GLIBC_2.29. The hardware test covers the Force; no Gen2/aarch64 build is included.

### Install

Save the current project first: the standard installer stops and restarts the MPC application. Download and unzip `Dub-Force-Siren-2.1.0-mpc-armv7.zip` on your computer. Back up the installed plugin folder and MPC settings, then copy the extracted `Dub-Force-Siren-2.1.0` folder to the device and run:

```sh
sh /path/to/Dub-Force-Siren-2.1.0/install.sh -y -t /path/to/Synths
```

For the maintainer's Force, the verified Synths path is `/media/AKAI_SSD/Synths`. Other devices should use their registered Synths directory. The extracted `INSTALL.md` contains the standard installer and manual-install instructions. The repository's [Force upgrade and rollback guide](https://github.com/no3z/mpc-dubsiren/blob/v2.1.0/docs/RELEASE_2.1.0.md) includes the tested paths and backup steps. Keep the 2.0.0 ZIP and backups for rollback.

ZIP SHA-256: `bf12f387e66481c076e5cea5d89de94703c6692f27bfc4589c0f8e3d0da84059`
