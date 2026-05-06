
libraries = [
  'average',
  'blend-reflect',
  'negation',
  'soft-burn',
  'soft-dodge',
  'subtractive',
]

foreach lib : libraries
  gegl_operations += shared_module(lib,
    lib + '.c',
    include_directories: geglWorkshopInclude,
    dependencies: gegl_operations_common_deps,
    link_with: [
      gegl_lib,
    ],
    name_prefix: '',
    install: true,
    install_dir: get_option('libdir') / api_name,
  )
endforeach
