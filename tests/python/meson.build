
testnames = [
  'gegl-buffer',
  'gegl-color',
  'gegl-format',
  'gegl-node',
  'gegl',
]

ld_library_path = run_command(python, '-c',
  '''import os; print(os.environ['LD_LIBRARY_PATH'])'''
).stdout().strip()
gi_typelib_path = run_command(python, '-c',
  '''import os; print(os.environ['GI_TYPELIB_PATH'])'''
).stdout().strip()

if python.found() and pygobject3.found()
  foreach testname : testnames
    testfile = 'test-' + testname + '.py'
    test(testname,
      find_program(testfile),
      env: [
        'LD_LIBRARY_PATH='+ meson.build_root() / 'gegl' + ':' + ld_library_path,
        'GI_TYPELIB_PATH='+ meson.build_root() / 'gegl' + ':' + gi_typelib_path,
        'GEGL_PATH='      + meson.build_root() / 'operations',
        'GEGL_SWAP=RAM',
      ],
      suite: 'python',
      is_parallel: false,
    )
  endforeach
endif
