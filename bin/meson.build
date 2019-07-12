subdir('lua')

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
  gegl_deps += [ libpng, ]
endif

if mrg.found() and gexiv2.found() and sdl1.found()
  gegl_sources += files('mrg-gegl.c', 'ui.c')
  gegl_deps += [ mrg, gexiv2, sdl1, ]
endif

if libspiro.found()
  gegl_sources += files('gegl-path-spiro.c')
  gegl_deps += [ libspiro, ]
endif

gegl_bin = executable('gegl',
  gegl_sources,
  include_directories: [ rootInclude, geglInclude, ],
  dependencies: gegl_deps,
  link_with: gegl_lib,
  install: true,
)
