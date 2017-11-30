gegl_module_sources = files(
  'gegldatafiles.c',
  'geglmodule.c',
  'geglmoduledb.c',
)

gegl_module_lib = static_library('gegl_module',
  gegl_module_sources,
  include_directories: [ rootInclude, geglInclude, ],
  dependencies: [
    babl,
    glib,
  ],
)
