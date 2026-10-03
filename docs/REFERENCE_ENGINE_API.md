# sd88me engine and VST2 integration reference

Research date: 2026-10-03. Source: read-only checkout of
`https://github.com/sd88me/mpc-vst-plugins`, commit
`faae55acfbf3ccac88d15df8ab3c45f74002e91c`, examined at
`/tmp/dub-force-research/mpc-vst-plugins`.

This document reports the actual source contract. It does not claim device verification
for this project. The upstream generic wrapper's restrictions are distinguished from
the host's capabilities.

## Authoritative files

| Subject | File in reference repository |
| --- | --- |
| Engine contract | `wrapper/engine.h` |
| Actual ABI, callbacks, conversion, lifecycle | `wrapper/vst2_wrap.c` |
| Runtime resource-directory discovery | `wrapper/plugin_dir.h` |
| Hidden popup state | `wrapper/popup.h` |
| Parameters and plugin descriptor | `tools/params.py`, `tools/gen_vst.py` |
| ARM build | `tools/build_port.sh` |
| Offline ABI test | `tools/test_port.sh`, `tools/host_test.c` |
| Native host-facing synth/effect examples | `poc/synth.c`, `poc/gain.c` |
| Compatible engine adapter | `adapters/schwung/schwung_engine.c` |
| Installer and packaging | `tools/release.py`, `tools/release/install.sh`, `tools/release/plugin_list.awk` |
| Observed host limitations | `docs/NOTES.md` |

Pinned upstream references: [engine interface](https://github.com/sd88me/mpc-vst-plugins/blob/faae55acfbf3ccac88d15df8ab3c45f74002e91c/wrapper/engine.h),
[wrapper implementation](https://github.com/sd88me/mpc-vst-plugins/blob/faae55acfbf3ccac88d15df8ab3c45f74002e91c/wrapper/vst2_wrap.c),
[build script](https://github.com/sd88me/mpc-vst-plugins/blob/faae55acfbf3ccac88d15df8ab3c45f74002e91c/tools/build_port.sh).

## Engine interface

The callback table is `mpc_engine_t`, returned by `const mpc_engine_t *mpc_engine(void)`.
The order is significant because upstream engines initialize it positionally.

| Order | Callback signature | Observed use and obligation |
| --- | --- | --- |
| 1 | `void *create(const char *data_dir)` | Called during entry-point construction; returns an instance or NULL. Own all DSP state per instance. Copy `data_dir` if retaining it: wrapper may pass a stack string. |
| 2 | `void destroy(void *inst)` | Called once by `effClose`; free all instance allocations. |
| 3 | `void midi(void *inst, const uint8_t *msg, int len)` | Called immediately from event dispatcher, with length 3. Do not retain the host-owned message pointer. |
| 4 | `void set_param(void *inst, const char *key, const char *val)` | String key/value control. Ordinary values use 64-byte wrapper stack buffers. Parse or copy immediately. |
| 5 | `int get_param(void *inst, const char *key, char *buf, int buf_len)` | Fill caller-owned buffer with NUL-terminated text; return positive on success. Unknown keys should return nonpositive. |
| 6 | `void render(void *inst, int16_t *out_lr, int frames)` | Synth output: interleaved signed 16-bit stereo, 44,100 Hz, 128 frames per upstream call. Write every output sample. |
| 7 | `void process(void *inst, const int16_t *in_lr, int16_t *out_lr, int frames)` | Effects only. Same fixed format. Buffers are distinct. Last field may be NULL for synths. |

The table has no version field, sample-rate setter, reset, prepare, host services,
event timestamp, latency reporting, or native float rendering callback. All callbacks
except the effect-only final field are used without individual NULL checks.

`create()` receives `MODULE_DIR` or NULL by default. With `MODULE_SUBDIR` it receives
`<loaded .so directory>/<subdir>` when directory discovery succeeds, else the
`MODULE_DIR` fallback. **Actual `plugin_dir.h` reads `/proc/self/maps`; it deliberately
avoids `dladdr()` to avoid GLIBC_2.34 dependencies.** `docs/PORTING.md` still mentions
`dladdr()` and is stale on this point.

The Schwung adapter maps these callbacks to `plugin_api_v2_t`, calls
`move_plugin_init_v2(NULL)`, supplies an empty data-directory string instead of NULL,
passes NULL JSON defaults, and tags incoming MIDI as external source 2. No Schwung
dependency is needed for an independently written engine.

## VST2 ABI and entry point

The exported entry point is `VSTPluginMain(audioMasterCallback)` returning `AEffect *`.
There is no Steinberg SDK dependency. The callback uses pointer-sized `intptr_t` for
its result and `value`, 32-bit opcode and index, a pointer, and a float option.

The source declares the following `AEffect` field order:

1. `int32_t magic`.
2. Function pointers `dispatcher`, legacy `process`, `setParameter`, `getParameter`.
3. `int32_t numPrograms, numParams, numInputs, numOutputs, flags`.
4. `intptr_t resvd1, resvd2`.
5. `int32_t initialDelay, realQualities, offQualities`; `float ioRatio`.
6. `void *object, *user`.
7. `int32_t uniqueID, version`.
8. Function pointers `processReplacing`, `processDoubleReplacing`.
9. `char future[56]`.

Use the native compiler layout; do not pack the structure. The layout depends on
the target pointer width. `magic` is `0x56737450` (`VstP`). The upstream instance is
zero-initialized, so unused fields including `numPrograms`, `initialDelay`, and
`processDoubleReplacing` remain zero/NULL. A synth reports zero inputs, two outputs,
flags `(1<<4)|(1<<5)|(1<<8)`; an effect reports two inputs/two outputs and omits
the synth bit. UID is the four-character descriptor interpreted in big-endian order.

## Lifecycle and dispatch

Entry point obtains the shared immutable engine callback table, allocates one
wrapper instance, creates its engine, sets callbacks and identity, and returns its
embedded `AEffect`. It performs initialization before `effOpen`; `effOpen` simply
returns 1. `effClose` destroys the engine then frees the wrapper. Host must cease
using that `AEffect` after close.

| Opcode | Number | Actual implementation |
| --- | --- | --- |
| `effOpen` / `effClose` | 0 / 1 | Acknowledge / destroy instance. |
| Parameter label/display/name | 6 / 7 / 8 | Text written to host buffers; limits 8 / 24 / 32 bytes respectively, including NUL. |
| `effSetSampleRate` / `effSetBlockSize` / `effMainsChanged` | 10 / 11 / 12 | Return 1; do not update DSP, buffers, or state. |
| `effGetChunk` / `effSetChunk` | 23 / 24 | Text state protocol described below. |
| `effProcessEvents` | 25 | Forward every MIDI event of type 1. |
| `effCanBeAutomated` | 26 | True for any valid parameter index. |
| `effGetPlugCategory` | 35 | 2 synth / 1 effect. |
| Effect name / vendor / product / version | 45 / 47 / 48 / 49 | Generated static identity, names limited to 32 bytes. |
| `effCanDo` | 51 | +1 for receiveVstEvents, receiveVstMidiEvent, receiveVstTimeInfo; -1 otherwise. |
| `effGetVstVersion` | 58 | 2400. |

Unknown opcodes return 0. There is no native editor-window callback, program/preset
dispatcher, reset-on-mains implementation, or double-precision audio path.

## MIDI and timing

`VstEvents` consists of an int32 count, pointer-sized reserved field, then event
pointers. Its declaration uses `events[2]` as the conventional trailing event array;
the dispatcher nevertheless iterates through the host's full count. `VstMidiEvent`
contains four leading int32 fields (type, byteSize, deltaFrames, flags), int32
noteLength/noteOffset, four MIDI bytes, and four single-byte trailing fields.

The wrapper forwards exactly three bytes and ignores `deltaFrames`, note timing,
source flags, and SysEx. All events apply before subsequent audio rendering. With
an arbitrary host block size, audio already stored in the synth FIFO can postpone
audible effects until the next 128-frame engine render. The reference does **not**
provide sample-accurate MIDI. An independent wrapper may implement a preallocated
timestamped event queue; this is our implementation choice, not upstream behavior.

With `HAS_LFO_BPM=1`, the audio callback asks `audioMasterGetTime` (opcode 7) for
`kVstTempoValid` (1<<10), validates a positive tempo, and calls `set_param("lfo_bpm",
"%.2f")` when tempo differs by over 0.01 BPM. No transport or PPQ information is
forwarded through the engine API.

## Audio buffers and processing

VST receives planar float buffers. Upstream synth wrapper maintains one 256-sample
int16 output block and a read position; requests engine blocks of exactly 128 frames,
then converts samples using `sample/32768.0f`. Arbitrary positive VST frame counts
are serviced from that FIFO. Native internal float processing is possible inside
the engine, but the interface quantizes the final output to 16-bit.

`processReplacing` overwrites the output. Legacy `process` adds to existing output;
both pointers are populated. The wrapper's synth callback ignores all input buffers.

Effects convert host input to int16 with clipping and `lrintf`. Complete aligned
128-frame blocks process immediately, with no added buffer latency. Other block
sizes are collected and processed one block late; initial samples are silent. The
wrapper does not report this conditional latency in `initialDelay`.

The generic wrapper has no resampling. Claiming support for 48/96 kHz solely because
the sample-rate dispatcher returns success would be incorrect. Its documented MPC
target is 44.1 kHz/128 frames. A float engine with arbitrary sample-rate support
requires an independently extended host integration.

## Parameters, defaults, triggers and text

`params.json` is an ordered list, or object containing `params` and optional
`sections`. Each item declares stable `key`, visible `name`, numeric min/max/unit
or string `options`, and optional physical `default`. VST indices are list positions.
Never reorder shipped indices. Popup hidden parameters are appended after the list.

`gen_vst.py` emits `params.h`: `param_t PARAMS[]`, count, plugin identity, and custom
defines. Numeric defaults are normalized to 0..1. The wrapper does **not** call
`set_param` for every default during construction; engine creation must establish
matching defaults. A failed `get_param` only makes the wrapper report the metadata
default and does not apply it to the DSP.

Numeric normalization is linear: `physical=min+(max-min)*clamp(normalized,0,1)`.
`display:"int"` rounds before sending to engine and retains a fractional shadow
position for fine encoder movement. Options are zero-based indices, rounded from
`normalized*(count-1)`. Intermediate encoder positions advance exactly one option;
exact option positions select that option. Returned option values can be decimal
indices or case-insensitive names. Engine must keep option values within the list;
display uses the returned index without a final array-bound clamp.

`momentary:true` schedules a host automation callback returning the trigger to zero
after `hold_ms*44.1` frames, or after one processing call when no hold is provided.
It **does not itself send a zero value back to the engine**. Trigger DSP state and
gate-release semantics must therefore be explicit. `step_of`/`step_delta` trigger
another numeric parameter by reading its current physical value, adding the step,
and clamping; the stepper's own key is never sent to the DSP.

`display:"string"` prevents nonnumeric text becoming zero through `atof` formatting.
Optional `dynamic_name` asks for `<key>_name`; `dynamic_display` asks for
`<key>_display`; string display selection may ask for `<key>_on`.

After a setter, wrapper defers `audioMasterUpdateDisplay` (42) and momentary
`audioMasterAutomate` (0) callbacks until audio processing to avoid reentering the
host's setter. Hidden popup open flags belong to wrapper only and are never sent
to engine or saved. Engine changes occurring independently of setters are not
automatically polled for display notifications.

## State chunks and memory ownership

`effGetChunk`: asks `get_param(inst,"state",chunk,8192)`. On success passes a pointer
to the wrapper-owned buffer to host and returns `strlen(chunk)+1`. Host must not
free it. State is text and must terminate within the buffer. The get callback's
reported positive length is only treated as success; actual chunk length uses
`strlen`.

`effSetChunk`: rejects lengths <=0 or >8192, copies into the wrapper's buffer,
forces the final supplied byte to zero, then calls `set_param(inst,"state",chunk)`.
This is a string protocol, not a general binary serialization format. The wrapper
does not validate the state schema, persist ordinary parameters automatically,
reset transient state on restore, or refresh the UI after restore.

Delay/reverb memory, oscillator state and control state belong to each engine
instance. Allocate all required working storage in create/prepare, release it in
destroy. Host-owned audio/event pointers and wrapper stack parameter/data paths
must not be retained. Source uses `volatile` for some shared flags, not atomics;
that does not establish a C/C++ thread-safety guarantee. An independent implementation
must explicitly handle control/audio-thread sharing and validate finite values.

## ARM build and descriptor

`tools/build_port.sh path/to/vst.json` runs Docker ARM containers via QEMU where
needed. Actual plugin image: `arm32v7/gcc:11-bullseye` (`--platform linux/arm/v7`),
glibc 2.31. Target is ARMv7 Linux hard-float, not ARM64. Upstream compatibility
limit is highest referenced GLIBC symbol <=2.32 (MPC OS 2.x); newer firmware is
documented as glibc 2.39. Confirm actual target architecture/loader on device.

C compiler flags: `-O2 -Wall -Wextra -Wno-unused-parameter -fPIC -shared
-fvisibility=hidden -std=gnu11`, generated-header include directory, configured
source/include/library flags, and `-Wl,--no-undefined`. Default library is `-lm`.
Strip the resulting `.so`; inspect exports with `readelf --dyn-syms -W` and glibc
versions with `readelf -V`. Only intended plugin export is `VSTPluginMain`.

When any source is `.cpp`, `.cc`, or `.cxx`, each source is compiled separately
with gcc or g++ (`-std=gnu++11`), the wrapper remains plain C, and linking uses g++.
No audio desktop SDK or graphics dependency is required in the plugin.

`vst.json` required identity fields: `name`, `vendor`, four-character `uid`, `so`;
also `params` (or adapter-specific `module`). Optional fields include version,
layout, defines, effect, custom_skin, art, and `build` with root/sources/cflags/libs.
Paths resolve relative to the descriptor directory; build root is configurable.
Generated outputs are `build/params.h`, `.so`, `skin/<vendor> - VST - <name>/`, and
`pluginlist-entry.xml`. Identity UID and `.so` name must remain stable across releases.

## MPC installation and limitations

Reference release format is one ZIP with `portable/<vendor> - VST - <name>/`,
containing `.so`, `version.xml`, `Plugin Skins/TUI.json`, `Q-Links.json`, resource
files and `plugin-meta.xml`. Package root also includes checksums and install/
uninstall scripts. `plugin-meta.xml` carries a `%payload-path%` template.

`docs/RELEASING.md` explicitly documents execution **as root**. Installer validates
root/ARMv7, discovers `/media/az01-internal/Settings/*/MPC.settings`, stops the MPC
application service (`acvs`, else `inmusic-mpc`), stages the folder under default
`/sdcard/Synths`, backs up settings, updates only matching `uid`/file entries within
`pluginList-arm`, checks the result, and restarts the application. A `-t` switch
supports another confirmed Synths folder. No firmware flash or OS reboot is required.
The application must be stopped during settings writes because it rewrites its own
in-memory settings. Check `SynthContentLocations` contains the target for skin discovery.

Reference installer preserves archive modes using `cp -a`, restores executable files
to 755 from its manifest, and does not issue a blanket chown. Files deployed by root
must remain readable by the MPC application. Actual permissions/ownership should be
verified on target rather than invented.

Native MPC pages are declarative skin resources, not plugin-drawn editors. VST2
MIDI output is reported ignored; generators use ALSA instead. Native menu pickers
are reported empty for VST2; skins implement popup lists. No native custom waveform/
envelope renderer or text entry is available through this wrapper. These are upstream
documented findings; device behavior for our build still requires verification.

## Vendored source references

Selected MPC integration files come from sd88me's `mpc-vst-plugins`; the pinned
commit and component references are recorded in `VENDORED.md`. Existing source
comments and the force-shadow/stb_truetype notices are preserved.

## Validation available from upstream

`tools/test_port.sh` generates params, compiles native sources with ASan+UBSan,
and links `tools/host_test.c` plus the wrapper. It checks two independent instances,
ABI magic/count, names, selected parameter round-trip, enum nudging, popup behavior,
legacy additive processing and available chunk restore. MIDI-to-audio silence is
only a warning, and missing chunks are also a warning. This smoke test alone is
insufficient to prove audible MIDI, complete state, realtime safety, DSP stability,
arbitrary-block behavior or hardware compatibility. Our tests must make those
requirements explicit and fail where the target requires them.
