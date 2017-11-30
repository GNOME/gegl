default_edl = custom_target('default.edl.inc',
  input : 'default.edl',
  output: 'default.edl.inc',
  command: [
    'sed', '', # 's_\\_\\\\_g;s_\r_a_;s_"_\\"_g;s_^_"_;s_$$_\\n"_',
    '@INPUT@',
  ],
  capture: true
)

gcut_sources = [
  'clip.c',
  'gcut-util.c',
  'gcut.c',
  'iconographer.c',
  default_edl,
]

gcut_deps = [
  babl,
  glib,
  gio,
  math,
]

if get_option('with-libpng')
  gcut_deps += [ libpng, ]
endif

if get_option('with-gexiv2')
  gcut_deps += [ gexiv2, ]
endif

if get_option('with-mrg') and get_option('with-sdl')
  gcut_sources += [
    'gcut-ui.c',
    'renderer.c',
  ]
  gcut_deps += [ sdl1, ]
endif


gcut_bin = executable('gcut',
  gcut_sources,
  include_directories: [ rootInclude, geglInclude, ],
  dependencies: gcut_deps,
  link_with: gegl_lib,
  install: true,
)
