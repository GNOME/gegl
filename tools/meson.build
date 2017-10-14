# gen-loader.sh
gen_loader_sh = find_program('gen-loader.sh')
gobj2dot = find_program('gobj2dot.rb')

# Compiled tools
tools = [
  'introspect',
  'operation_reference',
  'detect_opencl',
  'gegl-tester',
  'operations_html',
]

tools_cflags = [
  '-DTOP_SRCDIR="@0@"'.format(meson.source_root()),
]

# Special case: gegl-imgcmp needs to be installed
gegl_imgcmp = executable('gegl-imgcmp',
  'gegl-imgcmp.c',
  c_args: tools_cflags,
  dependencies: libgegl_dep,
  install: true,
)

foreach tool: tools
  executable(tool,
    '@0@.c'.format(tool),
    c_args: tools_cflags,
    dependencies: libgegl_dep,
  )
endforeach


# TODO exp_combine
