#!/usr/bin/env python3
"""Exercise the real packaged rename installer against isolated MPC settings."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import xml.etree.ElementTree as E
import zipfile

ROOT = Path(__file__).resolve().parents[1]
ZIP = ROOT / 'dist/Dub-Sirens-2.1.0-mpc-armv7.zip'
OLD = 'Dub Force - VST - Dub Force Siren'
NEW = 'Dub Force - VST - Dub Sirens'

for wrong_uid in (False, True):
    with tempfile.TemporaryDirectory(prefix='dub-rename-') as directory:
        work = Path(directory)
        with zipfile.ZipFile(ZIP) as archive:
            archive.extractall(work / 'package')
        package = work / 'package/Dub-Sirens-2.1.0'
        synths = work / 'Synths'
        old = synths / OLD
        shutil.copytree(package / 'portable' / NEW, old)
        meta = old / 'plugin-meta.xml'
        meta.write_text(meta.read_text().replace('Dub Sirens', 'Dub Force Siren').replace('44625372', '12345678' if wrong_uid else '44625372'))
        (old / 'user-note.txt').write_text('preserve this backup')
        settings = work / 'MPC.settings'
        settings.write_text(f'''<PROPERTIES>
<VALUE name="SynthContentLocations" val="{synths}"/>
<VALUE name="pluginList-arm">
<KNOWNPLUGINS>
<PLUGIN name="Dub Force Siren" uid="44625372" file="{old}/dub_force_siren.so"/>
<PLUGIN name="Other" uid="deadbeef" file="/other.so"/>
</KNOWNPLUGINS>
</VALUE>
</PROPERTIES>
''')
        before = settings.read_bytes()
        env = dict(os.environ, MPC_INSTALL_TEST='1', MPC_SETTINGS=str(settings), MPC_LEGACY_ROOT=str(work / 'legacy'))
        command = ['sh', str(package / 'install.sh'), '-y', '-t', str(synths)]
        result = subprocess.run(command, env=env, capture_output=True, text=True)
        if wrong_uid:
            assert result.returncode != 0 and 'different UID' in result.stderr
            assert settings.read_bytes() == before and old.exists() and not (synths / NEW).exists()
            continue
        assert result.returncode == 0, result.stdout + result.stderr
        assert not old.exists() and (synths / NEW).is_dir()
        backups = list((work / 'plugin-rename-backups').iterdir())
        assert len(backups) == 1 and (backups[0] / 'user-note.txt').read_text() == 'preserve this backup'
        entries = list(E.parse(settings).getroot().iter('PLUGIN'))
        matches = [e for e in entries if e.get('uid') == '44625372']
        assert len(matches) == 1 and matches[0].get('name') == 'Dub Sirens'
        assert matches[0].get('file') == str(synths / NEW / 'dub_force_siren.so')
        assert [e.attrib for e in entries if e.get('uid') == 'deadbeef'] == [dict(name='Other', uid='deadbeef', file='/other.so')]
        result = subprocess.run(command, env=env, capture_output=True, text=True)
        assert result.returncode == 0, result.stdout + result.stderr
        assert len(list((work / 'plugin-rename-backups').iterdir())) == 1
print('PASSED: rename archives old folder, preserves user files, registers once, is idempotent and rejects wrong UID')
