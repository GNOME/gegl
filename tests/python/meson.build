
testnames = [
  'gegl-buffer',
  'gegl-color',
  'gegl-format',
  'gegl-node',
  'gegl',
]

if python.found() and get_option('with-pygobject')
  foreach testname : testnames
    testfile = 'test-' + testname + '.py'
    test('python_' + testname,
      find_program(testfile),
      env: [
        'LD_LIBRARY_PATH=' + join_paths(meson.build_root(), 'gegl:$LD_LIBRARY_PATH'),
        'GI_TYPELIB_PATH=' + join_paths(meson.build_root(), 'gegl:$GI_TYPELIB_PATH'),
        'GEGL_PATH='+ join_paths(meson.build_root(), 'operations'),
        'GEGL_SWAP=RAM',
      ],
      suite: 'python',
    )
  endforeach
endif
