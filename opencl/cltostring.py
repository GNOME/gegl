#!/usr/bin/env python
from __future__ import print_function
from __future__ import unicode_literals

import os
import sys
import io

# Search for lines that look like #include "blah.h" and replace them
# with the contents of blah.h.
def do_includes (source):
  result = list()
  for line in source.split("\n"):
    if line.lstrip().startswith("#include"):
      splitstr = line.split('"')
      if len(splitstr) != 3:
        raise RuntimeError("Invalid include: %s" % line)
      include_path = splitstr[1]
      if not os.path.isfile(include_path):
        raise RuntimeError("Could not find include: %s" % line)
      with open(include_path, "r") as inc_file:
        result += do_includes(inc_file.read()).split("\n")
    else:
      result.append(line)
  return "\n".join(result)

# From http://stackoverflow.com/questions/14945095/how-to-escape-string-for-generated-c
def escape_string(s):
  result = ''
  for c in s:
    if not (32 <= ord(c) < 127) or c in ('\\', '"'):
      result += '\\%03o' % ord(c)
    else:
      result += c
  return result


if len(sys.argv) == 2:
  infile  = io.open(sys.argv[1], "r", encoding="utf-8")
  outfile = io.open(sys.argv[1] + '.h',  "w", encoding="utf-8")

elif len(sys.argv) == 3:
  infile  = io.open(sys.argv[1], "r", encoding="utf-8")
  outfile = io.open(sys.argv[2], "w", encoding="utf-8")

else:
  print("Usage: %s input [output]" % sys.argv[0])
  sys.exit(1)


cl_source = infile.read()
cl_source = do_includes(cl_source)
infile.close()

string_var_name = os.path.basename(sys.argv[1]).replace("-", "_").replace(":", "_")
if string_var_name.endswith(".cl"):
  string_var_name = string_var_name[:-3]

outfile.write("static const char* %s_cl_source =\n" % string_var_name)
for line in cl_source.rstrip().split("\n"):
  line = line.rstrip()
  line = escape_string(line)
  line = '"%-78s\\n"\n' % line
  outfile.write(line)
outfile.write(";\n")
outfile.close()
