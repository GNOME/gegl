
testnames = [
  'invert-crop',
  'invert',
  'rotate-crop',
  'rotate',
  'unsharp-crop',
  'unsharp',
]

foreach testname : testnames
  test(testname.underscorify(),
    find_program(testname + '.sh'),
    env: [
      'ABS_TOP_BUILDDIR=' + meson.build_root(),
      'ABS_TOP_SRCDIR='   + meson.source_root(),
      'GEGL_SWAP=RAM',
      'GEGL_PATH=' + meson.build_root() / 'operations',
    ],
    suite: 'mipmap',
  )
endforeach
