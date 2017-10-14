perf_tests = [
  'test-blur',
  'test-bcontrast',
  'test-bcontrast-minichunk',
  'test-unsharpmask',
  'test-bcontrast-4x',
  'test-init',
  'test-gegl-buffer-access',
  'test-samplers',
  'test-rotate',
  'test-saturation',
  'test-scale',
  'test-translate',
]

perf_tests_cflags = [
  '-DG_DISABLE_SINGLE_INCLUDES',
  '-DGLIB_DISABLE_DEPRECATION_WARNINGS',
  '-DCLUTTER_DISABLE_DEPRECATION_WARNINGS',
  '-DTESTS_DATA_DIR="@0@"'.format(join_paths(meson.source_root(), 'tests', 'data')),
]

foreach _test: perf_tests
  test_bin = executable(_test,
    '@0@.c'.format(_test),
    dependencies: libgegl_dep,
    c_args: perf_tests_cflags,
  )

  test(_test, test_bin)
endforeach
