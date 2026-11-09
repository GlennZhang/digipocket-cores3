"""Stage a full-resource build using the current tracked source files."""
from pathlib import Path
import shutil
import subprocess

root = Path(__file__).resolve().parents[1]
original = root / 'local/original'
target = root / 'local/full-build'
if not (original / 'sdcard/digipet/catalog.tsv').is_file():
    raise SystemExit('Missing local/original resource tree')
if target.exists():
    raise SystemExit(f'{target} already exists. Rename it before preparing a fresh build.')

for name in subprocess.check_output(['git', '-C', str(root), 'ls-files', '-z']).decode().split('\0'):
    if name:
        destination = target / name
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(root / name, destination)
for name in ('pet_assets.h', 'audio_assets.h', 'background_assets.h'):
    shutil.copy2(original / 'firmware/include' / name, target / 'firmware/include' / name)
for name in ('web/database', 'web/data', 'sdcard/digipet'):
    shutil.copytree(original / name, target / name, dirs_exist_ok=True)
print(target)
