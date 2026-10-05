#!/usr/bin/env python3
"""Compare two 16-bit PCM WAV renders sample by sample: compare_wav.py a.wav b.wav max_lsb"""
import array
import sys
import wave


def samples(path):
    with wave.open(path) as w:
        data = array.array("h", w.readframes(w.getnframes()))
    if sys.byteorder != "little":
        data.byteswap()
    return data


a, b = samples(sys.argv[1]), samples(sys.argv[2])
limit = int(sys.argv[3])
worst = max(abs(x - y) for x, y in zip(a, b)) if len(a) == len(b) else None
print("SIMD/scalar render: %d samples, max difference %s LSB" % (len(a), worst))
if worst is None or worst > limit:
    sys.exit("renders differ")
