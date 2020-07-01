
simple_tests = [
  'backend-file',
  'buffer-cast',
  'buffer-extract',
  'buffer-hot-tile',
  'buffer-iterator-aliasing',
  'buffer-sharing',
  'buffer-tile-voiding',
  'buffer-unaligned-access',
  'change-processor-rect',
  'color-op',
  'compression',
  'convert-format',
  'empty-tile',
  'format-sensing',
  'gegl-rectangle',
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
simple_tests_tap = [
  'buffer-changes',
  'gegl-color',
  'gegl-tile',
]
# Tests that are expected to fail - must also appear in main lists
simple_tests_fail = []
if os_win32
  simple_tests_fail += 'buffer-sharing'
endif
# Tests that must not run in parallel - must also appear in main lists
simple_tests_not_parallel = [
  'buffer-sharing'
]

# Run tests
foreach _test : simple_tests
    if simple_tests_not_parallel.contains('_test')
      _parallel = false
    else
      _parallel = gegl_test_parallel
    endif

    test(_test.underscorify(),
    executable(_test,
      'test-' + _test + '.c',
      include_directories: gegl_test_includes,
      dependencies: gegl_test_deps,
      link_with: gegl_lib,
    ),
    env: gegl_test_env,
    should_fail: simple_tests_fail.contains(_test),
    workdir: meson.current_build_dir(),
    suite: 'simple',
    is_parallel: _parallel,
  )
endforeach

foreach _test : simple_tests_tap
  test(_test.underscorify(),
    executable(_test,
      'test-' + _test + '.c',
      include_directories: gegl_test_includes,
      dependencies: gegl_test_deps,
      link_with: gegl_lib,
    ),
    env: gegl_test_env,
    should_fail: simple_tests_fail.contains(_test),
    workdir: meson.current_build_dir(),
    protocol: 'tap',
    suite: 'simple',
    is_parallel: _parallel,
  )
endforeach

# exp combine test
if gexiv2.found()
  test('exp_combine',
    find_program('test-exp-combine.sh'),
    env: gegl_test_env,
    depends: [exp_combine, gegl_imgcmp],
    suite: 'simple',
    timeout: 60,
    is_parallel: gegl_test_parallel,
  )
endif
