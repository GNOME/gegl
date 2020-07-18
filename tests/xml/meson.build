
xml_tests = [
  'test-load',
  'test-roundtrip',
  'test-save',
]

foreach _test : xml_tests
  test(_test.underscorify(),
    executable(_test,
      _test + '.c',
      include_directories: gegl_test_includes,
      dependencies: gegl_test_deps,
      link_with: gegl_lib,
    ),
    env: gegl_test_env,
    protocol: 'tap',
    suite: 'xml',
    is_parallel: gegl_test_parallel,
  )
endforeach
