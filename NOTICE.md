# Source credits

Dub Force Siren is based on BARZINE Siren Deck's algorithms and control mappings.
Source: https://barzine.news/siren-deck/.

The MPC VST2 wrapper, parameter/skin tooling, host test/benchmark and installer
come from sd88me's mpc-vst-plugins:
https://github.com/sd88me/mpc-vst-plugins, commit
faae55acfbf3ccac88d15df8ab3c45f74002e91c. Original file comments are preserved.

The included force-shadow renderer is copyright (c) 2026 sd88me, MIT licensed.
The full notice is preserved under
vendor/mpc-vst-plugins/tools/vendor/force-shadow/LICENSE. stb_truetype includes
its own embedded MIT/public-domain alternatives. See VENDORED.md for sources.

The master compressor and limiter (`Compressor` in `src/dsp_core.h`) are a port of the
algorithm and constants of Chromium's Blink `DynamicsCompressor`
(third_party/blink/renderer/platform/audio/dynamics_compressor.cc), which the BARZINE
page uses through Web Audio. Its license notice follows.

    Copyright (C) 2011 Google Inc. All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions
    are met:

    1.  Redistributions of source code must retain the above copyright
        notice, this list of conditions and the following disclaimer.
    2.  Redistributions in binary form must reproduce the above copyright
        notice, this list of conditions and the following disclaimer in the
        documentation and/or other materials provided with the distribution.
    3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
        its contributors may be used to endorse or promote products derived
        from this software without specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
    EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
    DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
    THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
