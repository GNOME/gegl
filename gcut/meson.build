gcut_sources = [
  'clip.c',
  'gcut-util.c',
  'gcut.c',
  'iconographer.c',
]

gcut_dependencies = [
  common_deps,
  babl,
  libpng,
  spiro,
  libmath,
  mrg,
  gexiv2,
  libgegl_dep,
]

# XXX
# if mrg_support and sdl_support
#   gcut_sources += 'gcut-ui.c'
#   gcut_dependencies += libsdl
# endif

gcut = executable('gcut',
  gcut_sources,
  dependencies: gcut_dependencies,
  include_directories: config_h_dir,
  install: true,
)
