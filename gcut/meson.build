
gcut_sources = files(
  'clip.c',
  'gcut-util.c',
  'gcut.c',
  'iconographer.c',
)

gcut_sources += custom_target('default.edl.inc',
  input : 'default.edl',
  output: 'default.edl.inc',
  command: [ find_program('make_includable.sh'), '@INPUT@', ],
  capture: true,
)

gcut_deps = [
  babl,
  glib,
  gio,
  math,
]

if libpng.found()
  gcut_deps += libpng
endif

if gexiv2.found()
  gcut_deps += gexiv2
endif

if mrg.found() and sdl1.found()
  gcut_sources += files(
    'gcut-ui.c',
    'renderer.c',
  )
  gcut_deps += [ mrg, sdl1 ]
endif


gcut_bin = executable('gcut',
  gcut_sources,
  include_directories: [ rootInclude, geglInclude, ],
  dependencies: gcut_deps,
  link_with: gegl_lib,
  install: true,
)
