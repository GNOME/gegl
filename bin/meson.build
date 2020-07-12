
subdir('lua', if_found: lua)

gegl_sources = files(
  'gegl-options.c',
  'gegl-path-smooth.c',
  'gegl.c',
)

gegl_deps = [
  babl,
  glib,
  gobject,
  math,
]

if libpng.found()
  gegl_deps += libpng
endif

if mrg.found() and gexiv2.found() and lua.found()
  gegl_sources += files(
    'argvs.c',
    'mrg-gegl.c',
    'ui-collection.c',
    'ui-core.c',
    'ui-viewer.c',
  )

  gegl_sources += custom_target('argvs-commands.inc',
    input : gegl_sources,
    output: 'argvs-commands.inc',
    command: [argvs_extract, '@INPUT@'],
    capture: true,
  )

  gegl_deps += [mrg, gexiv2, lua, gio]
endif

if libspiro.found()
  gegl_sources += files('gegl-path-spiro.c')
  gegl_deps += libspiro
endif

gegl_bin = executable('gegl',
  gegl_sources,
  include_directories: [rootInclude, geglInclude],
  dependencies: gegl_deps,
  export_dynamic: true,
  link_with: gegl_lib,
  install: true,
)
