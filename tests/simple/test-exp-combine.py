#!/usr/bin/env python3
import os
import sys
import subprocess

# Set by gegl_test_env in meson.build
abs_top_srcdir = os.getenv('ABS_TOP_SRCDIR')
abs_top_builddir = os.getenv('ABS_TOP_BUILDDIR')

exp_combine_path = os.path.join(abs_top_builddir, "tools", "exp_combine")
if not os.path.isfile(exp_combine_path):
  print("Skipping test-exp-combine due to lack of exp_combine executable")
  sys.exit(77)
try:
  exp_combine_command = [
  exp_combine_path,
  os.path.join(abs_top_builddir, "tests", "simple", "exp-combine.hdr"),
  os.path.join(abs_top_srcdir, "tests", "compositions", "data", "parliament_0.png"),
  os.path.join(abs_top_srcdir, "tests", "compositions", "data", "parliament_1.png"),
  os.path.join(abs_top_srcdir, "tests", "compositions", "data", "parliament_2.png"),
  ]
  subprocess.run(exp_combine_command, check=True)
  gegl_imgcmp_command = [
  os.path.join(abs_top_builddir, "tools", "gegl-imgcmp"),
  os.path.join(abs_top_srcdir, "tests", "simple", "reference", "exp-combine.hdr"),
  os.path.join(abs_top_builddir, "tests", "simple", "exp-combine.hdr"),
  ]
  result = subprocess.run(gegl_imgcmp_command, check=True)
  returncode = result.returncode
except subprocess.CalledProcessError as e:
  returncode = e.returncode
  # if returncode == 0:
  #   os.remove(os.path.join(abs_top_builddir, "tests", "simple", "test-exp-combine.hdr"))

sys.exit(returncode)
# print("WARNING: The result of this test is being ignored!")
# sys.exit(0)
