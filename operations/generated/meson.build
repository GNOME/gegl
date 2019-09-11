gegl_generated_sources = files(
  'add.c',
  'clear.c',
  'color-burn.c',
  'color-dodge.c',
  'darken.c',
  'difference.c',
  'divide.c',
  'dst-atop.c',
  'dst-in.c',
  'dst-out.c',
  'dst-over.c',
  'dst.c',
  'exclusion.c',
  'gamma.c',
  'hard-light.c',
  'lighten.c',
  'multiply.c',
  'overlay.c',
  'plus.c',
  'screen.c',
  'soft-light.c',
  'src-atop.c',
  'src-in.c',
  'src-out.c',
  'src.c',
  'subtract.c',
  'xor.c',
)

gegl_generated_sources += custom_target('module_generated_gpl3.c',
  input : gegl_generated_sources,
  output: 'module_generated_gpl3.c',
  command: [ gen_loader, '@INPUT@', ],
  capture: true
)

gegl_generated = shared_library('gegl-generated',
  gegl_generated_sources,
  include_directories: [ rootInclude, geglInclude, ],
  dependencies: [
    babl,
    glib,
    json_glib,
    math,
  ],
  link_with: [
    gegl_lib,
  ],
  c_args: [
    '-DGEGL_OP_BUNDLE',
  ],
  name_prefix: '',
  install: true,
  install_dir: get_option('libdir') / api_name,
)

gegl_operations += gegl_generated
