# 1.0.1 state audit

The actual VST entry advertises `effFlagsProgramChunks`, version 1001. Its
`effGetChunk`/`effSetChunk` calls reach the engine's `serialize`/`restore` functions.
Each instance has an 8192-byte chunk buffer; payload is `DFS1 ` followed by all
51 ordered physical parameter values with `%.9g` precision. All 51 values must
parse as finite before restore mutates the engine. Wrong schema, incomplete or
nonfinite chunks leave prior state intact. Trailing tokens are accepted; the
dispatcher return confirms delivery, not semantic validation.

Restore writes validated targets directly, avoiding preset/character selection
side effects. Thus an edited factory patch restores its custom values rather
than silently recalling defaults. It includes waveform, mode, filter type,
character, both nested LFO rates/amounts, all echo/reverb and EQ settings, and
performance toggle targets. The preset label remains the last selected factory
name after editing; it does not replace the custom snapshot.

`tests/rc_audit.cpp` verified 256 randomized snapshots, each restored into a
fresh instance and an existing instance reset through preset selection, plus
all 12 factory states: **26,724 normalized parameter comparisons passed** at
2e−6 tolerance. Randomization selects preset/character before custom settings.
Transient exclusions and audible latch restoration are separate assertions.
Existing malformed-chunk and ABI tests also pass under ASan/UBSan.

| State | Persistence / restoration |
| --- | --- |
| All 51 public parameter targets | Serialized in fixed ID order; FIRE/STOP read back as zero |
| LATCH | Restored; ON starts the gate on render, OFF stays silent without a new trigger |
| Freeze, Bend, Fast/Slow, octaves, Kill, Filter FX | Restored toggle settings; STOP clears performance holds |
| FIRE/STOP pending events, tap countdown | Excluded; a saved tap cannot refire on load |
| MIDI held notes/gates | Excluded; loading cannot recreate held external keys |
| Popup open state / hidden UI flags | Excluded; editor modal state is transient |
| Oscillator/LFO phase, envelope/ZAP progress, random generator | Excluded; processing starts with fresh history |
| Echo/reverb buffers and filter histories | Excluded; old tails do not replay |
| Smoothed current parameter values | Excluded; saved targets initialize the fresh engine |

This verifies the plugin chunk path programmatically. Whether the Force host
actually saves and delivers that chunk on project reload remains **physical test
15** in [the test card](MANUAL_FORCE_TEST_CARD.md). Do not mark host save/reload
verified until that test passes.
