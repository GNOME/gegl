gegl_sources = [
  'gegl.c',
  'gegl-options.c',
  'gegl-path-smooth.c',
]

gegl_dependencies = [
  babl,
  libpng,
  mrg,
  gexiv2,
  libmath,
  libgegl_dep,
]

if mrg_support and gexiv2_support and sdl_support
  gegl_sources += 'ui.c'
  gegl_dependencies += sdl
endif

if spiro_support
  gegl_sources += 'gegl-path-spiro.c'
  gegl_dependencies += spiro
endif

gegl_bin = executable('gegl',
  gegl_sources,
  dependencies: gegl_dependencies,
  install: true,
)
