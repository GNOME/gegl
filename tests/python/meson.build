python_tests = [
  'test-gegl.py',
  'test-gegl-node.py',
  'test-gegl-color.py',
  'test-gegl-buffer.py',
  'test-gegl-format.py',
]

foreach _test: python_tests
  test(_test, find_program(_test),
    env: test_env,
  )
endforeach
