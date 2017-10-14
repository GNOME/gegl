subdir('generated')
subdir('external')

workshop_ops = [
  'bayer-matrix',
  'bilateral-filter-fast',
  'demosaic-bimedian',
  'demosaic-simple',
  'ditto',
  'gradient-map',
  'hstack',
  'integral-image',
  'kuwahara',
  'linear-sinusoid',
  'median-blur',
  'rawbayer-load',
  'segment-kmeans',
  'shadows-highlights',
  'shadows-highlights-correction',
  'spherize',
]

workshop_ops_dependencies = [
  common_deps,
  libgegl_dep,
]

foreach op: workshop_ops
  library(op,
    '@0@.c'.format(op),
    dependencies: workshop_ops_dependencies,
    include_directories: config_h_dir,
    install: true,
    install_dir: operations_ext_dir,
  )
endforeach
