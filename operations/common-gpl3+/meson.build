gegl_common_sources = [
  'antialias.c',
  'apply-lens.c',
  'bump-map.c',
  'cartoon.c',
  'channel-mixer.c',
  'color-exchange.c',
  'color-to-alpha.c',
  'cubism.c',
  'deinterlace.c',
  'diffraction-patterns.c',
  'displace.c',
  'edge-laplace.c',
  'edge.c',
  'emboss.c',
  'engrave.c',
  'fractal-explorer.c',
  'fractal-trace.c',
  'gaussian-blur-selective.c',
  'illusion.c',
  'lens-distortion.c',
  'lens-flare.c',
  'maze.c',
  'mosaic.c',
  'motion-blur-circular.c',
  'motion-blur-zoom.c',
  'noise-slur.c',
  'noise-solid.c',
  'oilify.c',
  'photocopy.c',
  'plasma.c',
  'polar-coordinates.c',
  'red-eye-removal.c',
  'ripple.c',
  'shift.c',
  'sinus.c',
  'softglow.c',
  'spiral.c',
  'supernova.c',
  'texturize-canvas.c',
  'tile-glass.c',
  'tile-paper.c',
  'value-propagate.c',
  'video-degradation.c',
  'waves.c',
  'whirl-pinch.c',
  'wind.c',
]

module_c = custom_target('module_common_gpl3.c',
  input : [ gegl_common_sources ],
  output: [ 'module_common_gpl3.c' ],
  command: [
    gen_loader,
    '@INPUT@',
  ],
  capture: true
)

gegl_common_sources += module_c

gegl_common = shared_library('gegl-common-gpl3',
  [ gegl_common_sources, opencl_headers, ],
  include_directories: [ rootInclude, geglInclude, ],
  dependencies: [
    babl,
    glib,
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
  install_dir: join_paths(get_option('libdir'), lib_name),
)