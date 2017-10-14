workshop_generated_ops = [
  'average',
  'blend-reflect',
  'negation',
  'soft-burn',
  'soft-dodge',
  'subtractive',
]

workshop_generated_ops_deps = [
  common_deps,
  libgegl_dep,
]

foreach op: workshop_generated_ops
  library(op,
    '@0@.c'.format(op),
    dependencies: workshop_generated_ops_deps ,
    include_directories: config_h_dir,
    install: true,
    install_dir: operations_ext_dir,
  )
endforeach
