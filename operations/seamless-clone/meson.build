
seamless_clone_libs = [
  'seamless-clone',
  'seamless-clone-compose',
]

foreach lib : seamless_clone_libs
  gegl_operations += shared_module(lib,
    files(lib + '.c'),
    include_directories: [ rootInclude, geglInclude, seamlessInclude, ],
    dependencies: gegl_operations_common_deps + poly2tri_c,
    link_with: [
      gegl_lib,
      seamlessclone_lib,
    ],
    install_rpath: gegl_prefix / gegl_libdir,
    name_prefix: '',
    install: true,
    install_dir: get_option('libdir') / api_name,
  )
endforeach
