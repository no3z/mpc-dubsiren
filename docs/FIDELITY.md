# Fidelity to the BARZINE Siren Deck page

Dub Force Siren 2.0.0 is a native reimplementation, so this compares what it *sounds like*
with the page it follows. The page (SHA-256 `f43e43a2ac55159be34672aebbfb550b7c890d5e0d7a07204a3fda1c65adf9cb`,
see [BARZINE_DSP_REFERENCE.md](BARZINE_DSP_REFERENCE.md)) was rendered in Chromium 153 with an
`OfflineAudioContext` at 44.1 kHz, driving its own controls, and the engine was rendered with the
same physical values. Both sides had a second of silence before the note, so the page's freshly
built graph and the plugin's running state are comparable. No code or asset of the page is
included in this repository. The comparison scripts are not part of the repository either.

For the common signal path the page was also rendered with its compressor and limiter
disconnected (and with its reverb, echo spread, ducking and pan base disabled in a private
copy), to separate the engine from the master chain.

## What matches

| Aspect | Result |
| --- | --- |
| ZAP sweep | Same exponential curve (1637 / 1120 / 775 / 517 / 345 Hz at 20 / 50 / 80 / 110 / 140 ms for 2100 Hz, −40 st) |
| Envelope | Attack 10→90 %: 6 → 145 ms on both; release to −20 dB: 501 ms on both |
| Resonant lowpass, loop filters | Frequency response identical to Chromium's `BiquadFilterNode` (0.00 dB) |
| Echo | Repeat levels, feedback decay, wet gain and loop filters equal to 0.0 dB (pure tone, 42 % and 70 % feedback); repeat timing equal (see below) |
| Master chain | Compressor and limiter static curve within 0.006 dB of Chromium over −60…0 dBFS; Air Raid RMS −5.5 dB on both |
| LFO triangle and sine | Same depth (ratio 1.00), ~2 ms apart |
| Waveform spectra | Harmonic levels within 0.7 dB (to the 15th), correlation ≥ 0.99 |

Echo timing: in Web Audio every pass through the feedback path takes one render quantum
(128 samples) more than the first repeat. The engine reproduces it (repeat k at k·T+(k−1)·128).
"Feedback Madness" (88 % at 0.1 s) differs by −0.14 dB RMS from the page.

## What differs

* **No look-ahead.** The page's two compressor nodes delay its output by 12 ms. The plugin adds
  no latency, so the limiter lets a few peaks through; OUTPUT ends in a soft bound (0.9 → 0.98).
* **Saw and square level.** Chromium scales its band-limited tables so the Gibbs peak is 1; the
  flat parts sit at 0.82–0.85 (0.848 at 30 Hz, 0.830 at 1.76 kHz for the square). The engine
  uses 0.84 for the audio oscillator.
* **Saw and square LFO, and PING PONG.** The same normalisation applies to the page's LFOs, which
  therefore sweep ~15–18 % less than ±depth, and the page's ping-pong at 90 % pans to about 76 %.
  The engine's LFOs and ping-pong reach their full depth, and its saw LFO starts at its minimum,
  half a cycle away from the page's. Left as is: Police's two tones depend on the full square swing.
* **20 kHz post filter** of the page: not present (only visible above 14 kHz).
* **Echo "clean" extras of the page:** reverb (16 %), the second stereo tap (spread), ducking
  while a note is held and the −0.28 pan base. All removed on purpose in 2.0.0.
* **Design changes:** LFO depth is a percentage of pitch; pitch reaches 10 Hz (the page stops
  at 60 Hz); ZAP lasts 180 ms; no ZAP repeat; and everything else listed in [DSP.md](DSP.md).
* **Presets:** only some of the twelve come from the page. Smoke uses resonance 2 (page
  "wobble": 4); Clash uses cutoff 4000 and resonance 2 (page "pyunpyun": 5200 and 3).

## History

Until this check the master used a textbook compressor with the page's threshold, knee and
ratio. Chromium's node has makeup gain and a knee above the threshold, so 1.0.x and the first
2.0.0 build were about 19 dB quieter than the page on a loud signal (Air Raid: −24.4 dB RMS
against −5.5 dB). Fixed before release and installed on the Force on 2026-10-05; CPU cost on
the device in [PERFORMANCE_2.0.0.md](PERFORMANCE_2.0.0.md).
