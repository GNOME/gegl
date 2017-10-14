xml_tests = [
  'test-save',
  'test-load',
  'test-roundtrip',
]

foreach _test: xml_tests
  test_bin = executable(_test,
    '@0@.c'.format(_test),
    dependencies: libgegl_dep,
  )

  test(_test, test_bin,
    env: test_env,
  )
endforeach
