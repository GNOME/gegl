subdir('external')
subdir('generated')

libraries = [
  'bayer-matrix',
  'bilateral-filter-fast',
  'color-to-alpha-plus',
  'demosaic-bimedian',
  'demosaic-simple',
  'ditto',
  'gradient-map',
  'hstack',
  'integral-image',
  'linear-sinusoid',
  'rawbayer-load',
  'segment-kmeans',
  'shadows-highlights-correction',
  'shadows-highlights',
  'spherize',
]

foreach lib : libraries
  shared_library(lib,
    [ lib + '.c', opencl_headers, ],
    include_directories: [ rootInclude, geglInclude, seamlessInclude, ],
    dependencies: [
      babl,
      glib,
      gobject,
      math,
    ],
    link_with: [
      gegl_lib,
      seamlessclone_lib,
    ],
    name_prefix: '',
    install: true,
    install_dir: join_paths(get_option('libdir'), lib_name),
  )
endforeach
