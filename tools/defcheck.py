#!/usr/bin/env python3

"""
defcheck.py -- Consistency check for the .def files.
Copyright (C) 2006  Simon Budig <simon@gimp.org>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.


This is a hack to check the consistency of the .def files compared to
the respective libraries.

Invoke in the build directory and pass the name
of the built .def files on the command-line.

Needs the tool "objdump" to work

"""

import os, sys, subprocess

from os import path

def_files = sys.argv[1:]

#gegl_glXG* symbols are Linux-specific
exclude_symbols = [
    "gegl_glXGetCurrentContext",
    "gegl_glXGetCurrentDisplay",
]

# Some .def files are concatenated, which can result in an unsorted error.
# We allow those cases to be skipped by defining them here.
ignore_sorting_errors = [
   # in gegl.def
   "gegl_downscale_2x2_get_fun_x86_64_v2",
   "gegl_downscale_2x2_arm_neon",
   # in gegl-sc.def:
   "AdvancingFront_set_head",
]

have_errors = 0

libextension   = ".so"
command        = "nm --defined-only --extern-only "
platform_linux = True

if sys.platform in ['win32', 'cygwin']:
   libextension   = ".dll"
   command        = "objdump -p "
   platform_linux = False

for df in def_files:
   directory, name = path.split (df)
   basename, extension = name.split (".")

   libname = path.join(directory, "lib" + basename + "-*" + libextension)
   #FIXME: This leaks to ninja stdout, which should not happen
   #print ("platform: " + sys.platform + " - extracting symbols from " + libname)

   filename = df
   try:
      defsymbols = open (filename).read ().split ()[1:]
   except IOError as message:
      print(message)
      sys.exit (-1)

   doublesymbols = []
   for i in range (len (defsymbols)-1, 0, -1):
      if defsymbols[i] in defsymbols[:i]:
         doublesymbols.append ((defsymbols[i], i+2))

   sorterrors = ""
   sortok = True
   for i in range (len (defsymbols)-1):
      if defsymbols[i].lower() > defsymbols[i+1].lower():
         if not defsymbols[i+1] in ignore_sorting_errors:
            sorterrors += f"{defsymbols[i]} > {defsymbols[i+1]}\n"
            sortok = False
   sorterrors = sorterrors.split(sep='\n')

   status, nm = subprocess.getstatusoutput (command + libname)
   print(nm)
   if status != 0:
      print("trouble reading {} - has it been compiled?".format(libname))
      have_errors = -1
      continue

   nmsymbols = ""
   if platform_linux:
      nmsymbols = nm

   else: # Windows
      # remove parts of objdump output we don't need: anything up to a few lines
      # after Export Table: ' Ordinal      RVA  Name'

      objnm = nm.split(sep='\n')

      found = False
      nmsymbols = ""
      for s in objnm:
         if s == " Ordinal      RVA  Name":
            found = True
         elif found:
            nmsymbols += s
         # else: skip this line

   nmsymbols = nmsymbols.split()[2::3]
   nmsymbols = [s for s in nmsymbols if s[0] != '_']

   missing_defs = [s for s in nmsymbols  if s not in defsymbols and s not in exclude_symbols]
   missing_nms  = [s for s in defsymbols if s not in nmsymbols  and s not in exclude_symbols]

   if missing_defs or missing_nms or doublesymbols or not sortok:
      print()
      print("Problem found in", filename)

      if missing_defs:
         print("  the following symbols are in the library,")
         print("  but are not listed in the .def-file:")
         for s in missing_defs:
            print("     +", s)
         print()

      if missing_nms:
         print("  the following symbols are listed in the .def-file,")
         print("  but are not exported by the library.")
         for s in missing_nms:
            print("     -", s)
         print()

      if doublesymbols:
         print("  the following symbols are listed multiple times in the .def-file,")
         for s in doublesymbols:
            print("     : %s (line %d)" % s)
         print()

      if not sortok:
         print("  the .def-file is not properly sorted in the following cases")
         for s in sorterrors:
            if s != "":
               print("     * ", s)

      have_errors = -1

sys.exit (have_errors)
