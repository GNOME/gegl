simple_tests = [
  'test-backend-file',
  'test-buffer-cast',
  'test-buffer-changes',
  'test-buffer-extract',
  'test-buffer-hot-tile',
  'test-buffer-sharing',
  'test-buffer-tile-voiding',
  'test-change-processor-rect',
  'test-convert-format',
  'test-color-op',
  'test-empty-tile',
  'test-format-sensing',
  'test-gegl-rectangle',
  'test-gegl-color',
  'test-gegl-tile',
  'test-image-compare',
  'test-license-check',
  'test-misc',
  'test-node-connections',
  'test-node-passthrough',
  'test-node-properties',
  'test-object-forked',
  'test-opencl-colors',
  'test-serialize',
  'test-path',
  'test-proxynop-processing',
  'test-scaled-blit',
  'test-svg-abyss',
]

foreach _test: simple_tests
  test_bin = executable(_test,
    '@0@.c'.format(_test),
    dependencies: libgegl_dep,
  )

  test(_test, test_bin,
    env: test_env,
  )
endforeach
