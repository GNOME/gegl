# Common dependencies
libpoly2tri_c_common_dependencies = [
  glib,
  gobject,
  gmodule,
  json_glib,
  gthread,
  gio,
  gio_os,
  libmath,
]

# Common includes
libpoly2tri_c_common_includes = [
  include_directories('..'),
]


subdir('p2t')
subdir('refine')
subdir('render')

libpoly2tri_c_linkedlibs = [
  libp2tc,
  libp2tc_refine,
  libp2tc_render,
]

libpoly2tri_c = static_library('poly2tri-c',
  [],
  link_with: libpoly2tri_c_linkedlibs,
  include_directories: libpoly2tri_c_common_includes,
)

# Use the compiled internal library as the poly2tri_c dependency
poly2tri_c = declare_dependency(
  link_with: [ libpoly2tri_c, libp2tc_linkedlibs ],
  include_directories: libpoly2tri_c_common_includes,
)
