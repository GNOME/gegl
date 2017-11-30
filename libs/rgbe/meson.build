rgbe_sources = [
  'rgbe.c',
]

rgbe_lib = static_library('rgbe',
  rgbe_sources,
  dependencies: [ glib, ],
  include_directories: [ rootInclude, ],
)

librgbe = declare_dependency(
  # dependencies:, other dependencies needed to use this dependency
  # version:, the version of this dependency, such as 1.2.3
  include_directories: include_directories('..'),
  link_with: rgbe_lib,
)
