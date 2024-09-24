subdir('external')
subdir('generated')

gegl_workshop_sources = files(
  'aces-rrt.c',
  'alpha-inpaint.c',
  'bilateral-filter-fast.c',
  'boxblur-1d.c',
  'boxblur.c',
  'connected-components.c',
  'demosaic-bimedian.c',
  'demosaic-simple.c',
  'ditto.c',
  'band-tune.c',
  'gcr.c',
  'gradient-map.c',
  'integral-image.c',
  'rawbayer-load.c',
  'segment-kmeans.c',
  'shuffle-search.c',
  'selective-hue-saturation.c'
)

gegl_workshop_sources += custom_target('module_workshop.c',
  input : gegl_workshop_sources,
  output: 'module_workshop.c',
  command: [ gen_loader, '@INPUT@', ],
  capture: true
)

gegl_workshop = shared_library('gegl-workshop',
  gegl_workshop_sources,
  include_directories: [ rootInclude, geglInclude],
  dependencies: [
    babl,
    glib,
    gmodule,
    json_glib,
    math,
  ],
  link_with: [
    gegl_lib,
  ],
  c_args: [
    '-DGEGL_OP_BUNDLE',
  ],
  name_prefix: '',
  install: true,
  install_dir: get_option('libdir') / api_name,
)
