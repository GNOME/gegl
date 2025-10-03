#!/usr/bin/env python3
import sys
import subprocess

print('#include "config.h"')
print('#include <gegl-plugin.h>')

operation_names = []
for file_path in sys.argv[1:]:
  with open(file_path, 'r', encoding='utf-8') as file:
    for line in file:
      if 'GEGL_OP_NAME' in line:
        operation_names.append(line.split('NAME', 1)[1].strip())
for a in operation_names:
  print(f"void gegl_op_{a}_register_type(GTypeModule *module);")

print('''static const GeglModuleInfo modinfo = {
GEGL_MODULE_ABI_VERSION
};

G_MODULE_EXPORT const GeglModuleInfo * gegl_module_query (GTypeModule *module);
G_MODULE_EXPORT gboolean gegl_module_register (GTypeModule *module);

const GeglModuleInfo *
gegl_module_query (GTypeModule *module)
{
  return &modinfo;
}

gboolean
gegl_module_register (GTypeModule *module)
{''')


for a in operation_names:
  print(f"  gegl_op_{a}_register_type(module);")


print('''  return TRUE;
}''')
