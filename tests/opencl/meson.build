
opencl_tests = [
  'bilateral-filter',
  'box-blur',
  'brightness-contrast',
  'color-temperature',
  'edge-sobel',
  'focus-blur',
  'gaussian-blur',
  'invert-linear',
  'levels',
  'mono-mixer',
  'motion-blur',
  'noise-reduction',
  'oilify',
  'opacity',
  'pixelize',
  'snn-mean',
  'svg-src-over',
  'threshold',
  'value-invert',
  'vignette',
]

foreach _test : opencl_tests
  test(_test.underscorify(),
    test_runner,
    args: [
      '--test-name', _test,
      '--output-dir', meson.current_build_dir() / 'output',
      '--reference-path', meson.current_build_dir() / 'reference',
      '--endian', host_machine.endian(),
      '--generate-reference',
      '--input-file', files('@0@.xml'.format(_test)),
      '--gegl-exe', gegl_bin,
      '--imgcmp-exe', gegl_imgcmp,
      '--with-opencl',
    ],
    env: gegl_test_env,
    suite: 'opencl',
    timeout: 60,
    is_parallel: gegl_test_parallel,
    )
endforeach
