testnames = [
  'backend-file',
  'buffer-cast',
  'buffer-changes',
  'buffer-extract',
  'buffer-hot-tile',
  'buffer-sharing',
  'buffer-tile-voiding',
  'change-processor-rect',
  'color-op',
  'convert-format',
  'empty-tile',
  'format-sensing',
  'gegl-color',
  'gegl-rectangle',
  'gegl-tile',
  'image-compare',
  'license-check',
  'misc',
  'node-connections',
  'node-exponential',
  'node-passthrough',
  'node-properties',
  'object-forked',
  'opencl-colors',
  'path',
  'proxynop-processing',
  'scaled-blit',
  'serialize',
  'svg-abyss',
]

foreach testname : testnames

  test_exe = executable(testname,
    'test-' + testname + '.c',
    include_directories: [ rootInclude, geglInclude, ],
    dependencies: [
      babl,
      glib,
      gobject,
    ],
    link_with: [
      gegl_lib,
    ],
    install: false,
  )
  test(testname,
    test_exe,
    env: [
      'ABS_TOP_BUILDDIR=' + meson.build_root(),
      'ABS_TOP_SRCDIR='   + meson.source_root(),
      'GEGL_SWAP=RAM',
      'GEGL_PATH='+ meson.build_root() / 'operations',
    ],
    suite: 'simple',
    is_parallel: false,
  )

endforeach
