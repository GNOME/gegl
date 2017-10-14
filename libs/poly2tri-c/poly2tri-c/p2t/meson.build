subdir('sweep')
subdir('common')

libp2tc_linkedlibs = [
  libp2tc_sweep,
  libp2tc_common,
]

libp2tc = library('pt2c',
  'poly2tri.h',
  link_with: libp2tc_linkedlibs,
  include_directories: libpoly2tri_c_common_includes,
)
