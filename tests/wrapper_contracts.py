#!/usr/bin/env python3
"""Check index compatibility and refresh opt-ins using an isolated port fixture."""
import json
from pathlib import Path
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
TOOLS = ROOT / 'vendor/mpc-vst-plugins/tools'
sys.path.insert(0, str(TOOLS))
import gen_vst

with tempfile.TemporaryDirectory(prefix='dub-wrapper-') as directory:
    here = Path(directory)
    params = json.loads((ROOT / 'params.json').read_text())
    gen_vst.check_lock(directory, params)
    locked = (here / 'params.lock').read_bytes()
    gen_vst.check_lock(directory, params)
    assert (here / 'params.lock').read_bytes() == locked
    swapped = params.copy()
    swapped[0], swapped[1] = swapped[1], swapped[0]
    try:
        gen_vst.check_lock(directory, swapped)
    except SystemExit as error:
        assert 'parameter 0' in str(error)
    else:
        raise AssertionError('reordered parameters accepted')
    gen_vst.check_lock(directory, params + [dict(key='appended')])
    assert (here / 'params.lock').read_text().splitlines()[-1] == 'appended'
    (here / 'params.lock').write_bytes(locked)
    # This fixture alone opts pitch into a display refresh and turns STOP into
    # a stepper. Production parameter identities and behavior remain unchanged.
    params[0]['refresh_display'] = True
    params[-1].update(step_of='output', step_delta=-1)
    (here / 'params.json').write_text(json.dumps(params))
    cfg = json.loads((ROOT / 'vst.json').read_text())
    cfg.pop('layout')
    cfg['build']['root'] = str(ROOT)
    (here / 'vst.json').write_text(json.dumps(cfg))
    subprocess.run([str(TOOLS / 'test_port.sh'), str(here / 'vst.json')], cwd=ROOT, check=True)
print('PASSED: parameter lock, explicit refresh and stepper contracts')
