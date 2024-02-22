
tools_deps = [
  babl,
  glib,
  gobject,
]

tools_c_args = [
  '-DTOP_SRCDIR="' + project_source_root + '"',
]

detect_opencl = executable(
  'detect_opencl',
  'detect_opencl.c',
  include_directories: [ rootInclude, geglInclude, ],
  dependencies: [ tools_deps, ],
  link_with: [ gegl_lib, ],
  c_args: [ tools_c_args ],
  install: false,
)
gegl_imgcmp = executable(
  'gegl-imgcmp',
  'gegl-imgcmp.c',
  include_directories: [ rootInclude, geglInclude, ],
  dependencies: [ tools_deps, ],
  link_with: [ gegl_lib, ],
  c_args: [ tools_c_args ],
  install: true,
)
gegl_tester = executable(
  'gegl-tester',
  'gegl-tester.c',
  include_directories: [ rootInclude, geglInclude, ],
  dependencies: [ tools_deps, ],
  link_with: [ gegl_lib, ],
  c_args: [ tools_c_args ],
  install: false,
)
introspect = executable(
  'introspect',
  'introspect.c',
  include_directories: [ rootInclude, geglInclude, ],
  dependencies: [ tools_deps, ],
  link_with: [ gegl_lib, ],
  c_args: [ tools_c_args ],
  install: false,
)
operation_reference = executable(
  'operation_reference',
  'operation_reference.c',
  include_directories: [ rootInclude, geglInclude, ],
  dependencies: [ tools_deps, ],
  link_with: [ gegl_lib, ],
  c_args: [ tools_c_args ],
  install: false,
)
operations_html = executable(
  'operations_html',
  'operations_html.c',
  include_directories: [ rootInclude, geglInclude, ],
  dependencies: [ tools_deps, ],
  link_with: [ gegl_lib, ],
  c_args: [ tools_c_args ],
  install: false,
)

if gexiv2.found()
  exp_combine = executable(
    'exp_combine',
    'exp_combine.cpp',
    include_directories: [ rootInclude, geglInclude, ],
    dependencies: [ tools_deps, gexiv2, ],
    link_with: [ gegl_lib, ],
    c_args: [ tools_c_args ],
    install: false,
  )
endif

gen_loader = find_program('gen-loader.sh')
gobj2dot   = find_program('gobj2dot.rb')


meson.add_dist_script('dist-script.sh')
