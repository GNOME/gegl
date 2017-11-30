
subdir('common')
subdir('sweep')

libp2tc = static_library('p2tc',
  libp2tc_common_sources,
  libp2tc_sweep_sources,
  dependencies: [ glib, math, ],
  include_directories: [ rootInclude, ],
)
