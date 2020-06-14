
python_tests = [
  'gegl-buffer',
  'gegl-color',
  'gegl-format',
  'gegl-node',
  'gegl',
]

python_test_env = gegl_test_env
python_test_env.prepend('LD_LIBRARY_PATH', gegl_library_build_dir)
python_test_env.prepend('GI_TYPELIB_PATH', gegl_library_build_dir)

foreach _test : python_tests
  test(_test.underscorify(),
    find_program('test-' + _test + '.py'),
    env: python_test_env,
    suite: 'python',
    is_parallel: gegl_test_parallel,
  )
endforeach
