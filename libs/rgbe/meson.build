
rgbe_lib = static_library('rgbe',
  'rgbe.c',
  dependencies: glib,
  include_directories: rootInclude,
)

librgbe = declare_dependency(
  include_directories: include_directories('..'),
  link_with: rgbe_lib,
)
