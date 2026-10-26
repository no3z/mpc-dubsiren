# Sources

Dub Force Siren is based on BARZINE Siren Deck:
https://barzine.news/siren-deck/ and https://barzine.news/siren/.
Native DSP follows its equations and configuration; native approximations are
identified in the DSP reference.

Selected integration/build/skin/release files come from
https://github.com/sd88me/mpc-vst-plugins at
`faae55acfbf3ccac88d15df8ab3c45f74002e91c`.
Existing comments and notices are preserved. Project-specific changes are
recorded in `docs/BUILD.md` and the source.

The vendored force-shadow renderer is MIT licensed, copyright (c) 2026 sd88me.
Its LICENSE and embedded stb_truetype license are retained in
`vendor/mpc-vst-plugins/tools/vendor/force-shadow/`.

For 2.1.0, the wrapper/generator selectively backport the Q-Link refresh fix
from upstream PR #486 (merge `d611f56f3a064c886fd8046112838315205d0c2d`),
parameter index locking from `tools/gen_vst.py`, and option-backed native VST
programs. Local atomic safety, typed parameters, log mapping and popup ABI are
retained. The vendored host test adds deferred refresh regressions;
`test_port.sh` uses C++17 and passes the port's flags to the wrapper as well.

The revised 2.1.0 candidate also ports upstream's direction-aware enum `settle`
behavior with atomic previous-host positions, reset when presets/state change.
This fixes alternating WAVE/LFO selections during continuous host sweeps.

The port's release tool/installer adds `--previous-skin` to archive the old
Dub Force Siren folder outside Synths after successful rename registration.
The old UID is checked before stopping MPC; the copied new binary is compared
before archiving. `tests/installer_rename.py` checks migration, repeat installs,
wrong-UID rejection and preservation of unrelated settings and user files.
This intentional installer extension triggers the upstream catalog checker's
modified-template warning and is submitted for maintainer review.
