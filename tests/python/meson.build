
testnames = [
  'gegl-buffer',
  'gegl-color',
  'gegl-format',
  'gegl-node',
  'gegl',
]

if python.found() and pygobject_found
  foreach testname : testnames
    testfile = 'test-' + testname + '.py'
    test(testname,
      bash,
      args: [
        '-c',
        'LD_LIBRARY_PATH='+ meson.build_root() / 'gegl' + ':$LD_LIBRARY_PATH',
        'GI_TYPELIB_PATH='+ meson.build_root() / 'gegl' + ':$GI_TYPELIB_PATH',
        'GEGL_PATH='      + meson.build_root() / 'operations',
        'GEGL_SWAP=RAM',
        python.path(), files(testfile),
      ],
      suite: 'python',
      is_parallel: false,
    )
  endforeach
endif
