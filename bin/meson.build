gegl_sources = [
  'gegl-options.c',
  'gegl-path-smooth.c',
  'gegl.c',
]
gegl_deps = [
  babl,
  glib,
  gobject,
  math,
]

if get_option('with-libpng')
  gegl_deps += [ libpng, ]
endif

if get_option('with-mrg') and get_option('with-gexiv2') and get_option('with-sdl')
  gegl_sources += [ 'mrg-gegl.c', 'ui.c', ]
  gegl_deps += [ mrg, gexiv2, sdl1, ]
endif

if get_option('with-libspiro')
  gegl_sources += [ 'gegl-path-spiro.c', ]
  gegl_deps += [ libspiro, ]
endif

gegl_bin = executable('gegl',
  gegl_sources,
  include_directories: [ rootInclude, geglInclude, ],
  dependencies: gegl_deps,
  link_with: gegl_lib,
  install: true,
)
