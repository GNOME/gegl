gegl_common_gpl3_sources = files(
  'antialias.c',
  'apply-lens.c',
  'bayer-matrix.c',
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
  'linear-sinusoid.c',
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
  'shadows-highlights-correction.c',
  'shadows-highlights.c',
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
)

gegl_common_gpl3_sources += custom_target('module_common_gpl3.c',
  input : [ gegl_common_gpl3_sources ],
  output: [ 'module_common_gpl3.c' ],
  command: [
    gen_loader,
    '@INPUT@',
  ],
  capture: true
)

gegl_common_gpl3 = shared_library('gegl-common-gpl3',
  [ gegl_common_gpl3_sources, opencl_headers, ],
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
  install_dir: get_option('libdir') / api_name,
)

gegl_operations += gegl_common_gpl3

if host_cpu_family == 'x86_64'

  gegl_common_gpl3_x86_64_v2 = shared_library('gegl-common-gpl3-x86_64-v2',
    gegl_common_gpl3_sources,
    opencl_headers,
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
    c_args: [ '-DGEGL_OP_BUNDLE' ] + x86_64_v2_flags,
    name_prefix: '',
    install: true,
    install_dir: get_option('libdir') / api_name,
  )
  gegl_operations += gegl_common_gpl3_x86_64_v2
  
  gegl_common_gpl3_x86_64_v3 = shared_library('gegl-common-gpl3-x86_64-v3',
    gegl_common_gpl3_sources,
    opencl_headers,
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
    c_args: [ '-DGEGL_OP_BUNDLE' ] + x86_64_v3_flags,
    name_prefix: '',
    install: true,
    install_dir: get_option('libdir') / api_name,
  )
  gegl_operations += gegl_common_gpl3_x86_64_v3

elif host_cpu_family == 'arm'

  gegl_common_gpl3_arm_neon = shared_library('gegl-common-gpl3-arm-neon',
    gegl_common_gpl3_sources,
    opencl_headers,
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
    c_args: [ '-DGEGL_OP_BUNDLE' ] + arm_neon_flags,
    name_prefix: '',
    install: true,
    install_dir: get_option('libdir') / api_name,
  )
  gegl_operations += gegl_common_gpl3_arm_neon

endif
