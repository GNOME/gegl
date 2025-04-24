#!/usr/bin/env python3
import os
import shutil
from pathlib import Path

print("hoit hoi ", os.getcwd())
os.chdir('meson-dist')
print('Removing reference image to shrink size of tarball')
for png_file in Path('.').rglob('*.png'):
  if 'reference' in str(png_file):
    print(f'Removing: {png_file}')
    png_file.unlink()
print(os.getenv('MESON_SOURCE_ROOT', ''), os.getenv('MESON_BUILD_ROOT', ''))

# copy docs into distro
print('Copying README and NEWS to distribution')
for filename in ['README', 'NEWS']:
  source_path = Path(os.getenv('MESON_BUILD_ROOT', '')) / filename 
  if source_path.exists():
    shutil.copy(source_path, Path(os.getenv('MESON_DIST_ROOT', '')))
