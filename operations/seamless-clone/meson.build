
seamless_clone_libs = [
  'seamless-clone',
  'seamless-clone-compose',
]

foreach lib : seamless_clone_libs
  gegl_operations += shared_library(lib,
    files(lib + '.c'),
    include_directories: [ rootInclude, geglInclude, seamlessInclude, ],
    dependencies: [
      babl,
      glib,
      json_glib,
      poly2tri_c,
      math,
    ],
    link_with: [
      gegl_lib,
      seamlessclone_lib,
    ],
    name_prefix: '',
    install: true,
    install_dir: get_option('libdir') / api_name,
  )
endforeach
