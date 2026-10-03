# Force verification — 2026-10-03

## Verified remotely

- Device 192.168.2.31 identified as Akai Professional Force, armv7l, inMusic az01
  Linux 5.0.17; actual MPC startup log reports MPC3.9.1.
- Root SSH reached through an interactive/transient credential. No credential is
  stored in this repository, scripts, documentation or deployment logs.
- Existing SynthContentLocations includes `/media/AKAI_SSD/Synths`; default
  `/sdcard/Synths` was absent. Destination was discovered rather than guessed.
- ARM ELF32 little-endian machine 40/hard-float shared object, 58,952 bytes;
  GLIBC 2.29 maximum requirement; only VSTPluginMain exported. Device's own loader
  resolves its dependencies and processes stereo audio through an offline host.
- Package SHA256 integrity validated locally; all 108 installed resource files
  passed remote SHA256 verification after installation.
- Exactly one plugin UID 44625372 registered. All other plugin registration
  attributes/entries match the pre-install settings backup.
- Upstream installer stopped/started only acvs, then new MPC process 1620 stayed
  running. No reboot, firmware, boot, network or password changes occurred.
- Actual startup log: `Found, link: Dub Force Siren`. Native imports exist at
  `/usr/share/Akai/Content/Synths/Generic/Generic Knob Overlay.json` and
  `Generic Menu Overlay.json`. Host audio is 44100 Hz / 128 frames.
- Installed-path offline host opened the plugin in 2.6 ms, sent MIDI, exercised
  Q-Link sweeps, processed audio and closed without loader/crash errors.

Binary SHA256:
`f2d92642e6a3b50824032c62f0c76ccfa8857153b9727898d0a75de239e2707a`.

Settings changed:
`/media/az01-internal/Settings/MPC/MPC.settings`.

Settings backup retained:
`/media/az01-internal/Settings/MPC/MPC.settings.bak-dub_force_siren-20261003-003737`.
No prior version of this plugin existed at its confirmed target folder.

## CPU evidence

Final binary, actual Force CPU, offline VST host at 44100 Hz / 128 frames, 2 seconds
per stage including all-parameter storm:

| Stage | Mean block budget | p99 | Maximum |
|---|---:|---:|---:|
| Fresh idle |13.5%|14.6%|15.0%|
| One held note |15.2%|16.3%|16.9%|
| Four held notes, monophonic engine |15.3%|16.3%|16.6%|
| All-parameter sweep |17.5%|19.6%|20.8%|
| Release tail |14.9%|16.3%|16.6%|

Worst p99=19.6%, maximum20.8% of 2902µs. Upstream verdict**WARN**, above its 15%
p99 target for inexpensive multiple instances and below its 35% failure-side
warning ceiling. No plugin background threads. Independent installed-path
one-second stages measured worstp99=17.2%, max17.5%. These are threadCPU timings,
not proof of glitch-free deadlines in a busy liveMPC project. Fresh idle still
runs the modulation/effects engine and consumes CPU; optimizing that is useful.

## Errors and recovery

The first remote ZIP extraction failed because the device Python lacks zlib.
That attempt did not stop MPC or modify plugin/settings files. Recovery: unpack
locally, SCP the full package folder and run its checksum-validating installer.
No extra device package was installed.

Startup logs contain existing/general Sentry crashpad/JSON-parser warnings,
a duplicate Hype preset link and inefficient audio-path warnings. No error names
Dub Force Siren or reports its unresolved symbols, architecture mismatch or
crash. These unrelated application issues were not modified.

The source package's catalog check passed with two non-distribution metadata
warnings (source_repo/license absent). Source references are recorded in
NOTICE.md/VENDORED.md. No public catalog upload occurred.

## Requires human operation and listening

At the last remote check, the plugin was discovered by the MPC content scanner,
but was not yet mapped in the MPC process; it had been loaded by the standalone
host harness. Native editor interaction/audio routing through an actual project
therefore remains unverified. A human check was requested in chat.

1. Add **Dub Force Siren** as a plugin instrument and open its editor. Confirm the
   complete main panel, displayed values, FIRE/LATCH/STOP and popup selectors.
2. Tap FIRE; hold/release MIDI pads; enable then disable LATCH. Verify STOP silences
   voice and discards echo/reverb tails. Touch FIRE is a bounded tap, not a hold.
3. DragPitch/LFO/Delay/Feedback sliders; test all five waves, SIREN/ZAP and repeat.
4. Change CORE/MOTION/ECHO/SOURCE/PERFORM Q-Link banks: main controls must stay on
   the same visible panel. Check the suggested primary eight mappings.
5. Select CLEAN/DESK/SMOKE/ORBIT/CLASH; listen for their actual tone/modulation/
   width/flange differences. TestHP/LP through multiple repeats, INVERT, 8BIT,
   FREEZE/BEND, nested motion and chop. Release all toggles afterward.
6. Try all twelve presets at moderate monitoring volume; save/reload a project
   and check state/preset/EQ restore. Test within an ordinary busy project for CPU.
7. Capture browser and device audio if an audible equivalence comparison is
   wanted. No real Force output was recorded or listened to remotely.

## Exact installed plugin files

Every following absolute path passed remote SHA256 verification. The settings
and backup paths above are additional deployment artifacts. Temporary deployment
ZIP/package, loaderbench and scratchso are under/tmp and are not plugin resources.

- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/Q-Links - 8by1.json`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/Q-Links.json`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/TUI.json`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_bg_0.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_bg_1.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_btn_bend_BEND_off.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_btn_bend_BEND_on.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_btn_crush_8_BIT_off.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_btn_crush_8_BIT_on.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_btn_fast_FAST_off.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_btn_fast_FAST_on.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_btn_filter_hold_FILTER_FX_off.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_btn_filter_hold_FILTER_FX_on.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_btn_fire_FIRE_SIREN_off.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_btn_fire_FIRE_SIREN_on.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_btn_freeze_FREEZE_off.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_btn_freeze_FREEZE_on.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_btn_invert_INVERT_off.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_btn_invert_INVERT_on.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_btn_kill_KILL_off.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_btn_kill_KILL_on.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_btn_latch_LATCH_off.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_btn_latch_LATCH_on.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_btn_oct_down_OCT_DOWN_off.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_btn_oct_down_OCT_DOWN_on.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_btn_oct_up_OCT_UP_off.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_btn_oct_up_OCT_UP_on.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_btn_slow_SLOW_off.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_btn_slow_SLOW_on.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_btn_stop_STOP_off.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_btn_stop_STOP_on.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_pop_0_character.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_pop_0_filter_type.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_pop_0_lfo_wave.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_pop_0_mode.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_pop_0_preset.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_pop_0_wave.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_character_0_off.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_character_0_on.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_character_1_off.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_character_1_on.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_character_2_off.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_character_2_on.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_character_3_off.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_character_3_on.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_character_4_off.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_character_4_on.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_filter_type_0_off.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_filter_type_0_on.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_filter_type_1_off.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_filter_type_1_on.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_filter_type_2_off.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_filter_type_2_on.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_lfo_wave_0_off.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_lfo_wave_0_on.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_lfo_wave_1_off.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_lfo_wave_1_on.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_lfo_wave_2_off.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_lfo_wave_2_on.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_lfo_wave_3_off.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_lfo_wave_3_on.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_mode_0_off.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_mode_0_on.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_mode_1_off.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_mode_1_on.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_preset_0_off.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_preset_0_on.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_preset_10_off.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_preset_10_on.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_preset_11_off.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_preset_11_on.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_preset_1_off.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_preset_1_on.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_preset_2_off.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_preset_2_on.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_preset_3_off.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_preset_3_on.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_preset_4_off.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_preset_4_on.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_preset_5_off.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_preset_5_on.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_preset_6_off.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_preset_6_on.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_preset_7_off.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_preset_7_on.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_preset_8_off.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_preset_8_on.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_preset_9_off.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_preset_9_on.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_wave_0_off.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_wave_0_on.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_wave_1_off.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_wave_1_on.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_wave_2_off.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_wave_2_on.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_wave_3_off.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_wave_3_on.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_wave_4_off.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_popopt_0_wave_4_on.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_slider_v_30x48.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/Plugin Skins/sh_slider_v_30x64.png`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/dub_force_siren.so`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/notices/NOTICE.md`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/notices/VENDORED.md`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/notices/force-shadow-LICENSE.txt`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/plugin-meta.xml`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/presets/factory.json`
- `/media/AKAI_SSD/Synths/Dub Force - VST - Dub Force Siren/version.xml`
