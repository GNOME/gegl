project('poly2tri-c', 'c',
  license: 'GPL3+',
  version: '0.1.0',
  meson_version: '>=0.40.0',
  default_options: [
    'sysconfdir=/etc',
    'localstatedir=/var',
    'sharedstatedir=/var/lib'
  ],
)

glib = dependency('glib-2.0')
math = meson.get_compiler('c').find_library('m')

rootInclude = include_directories('.')

subdir('poly2tri-c/p2t')
subdir('poly2tri-c/refine')
subdir('poly2tri-c/render')


libpoly2tri_c = static_library('poly2tri-c',
  link_with: [ libp2tc, libp2tc_refine, libp2tc_render, ],
)


poly2tri_c = declare_dependency(
  include_directories: [ rootInclude, ],
  link_with: [ libpoly2tri_c, ],
  version: meson.project_version(),
)
