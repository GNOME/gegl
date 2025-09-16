#!/usr/bin/env python3
import os
import shutil
import re
import sys
import json

if not os.path.isfile("build.ninja"):
  print("\033[31m(ERROR)\033[0m: Script called standalone. This script should be only called from build systems.")
  sys.exit(1)


# This .py script should not even exist
# Ideally meson should take care of it automatically.
# See: https://github.com/mesonbuild/meson/issues/12977
with open("meson-info/intro-installed.json", "r") as f:
  build_installed = json.load(f)
for build_bin, installed_bin in build_installed.items():
  if build_bin.endswith((".dll", ".exe")):
    pdb_debug = os.path.splitext(build_bin)[0] + ".pdb"
    install_dir = os.path.dirname(installed_bin)
    if os.path.isfile(pdb_debug):
      if not os.getenv("MESON_INSTALL_DRY_RUN"):
        print(f"Installing {pdb_debug} to {install_dir}")
        shutil.copy2(pdb_debug, install_dir)
