
composition_tests = [
  'affine-with-negative',
  'alien-map',
  'apply-lens',
  'apply-lens3',
  'bump-map',
  'checkerboard',
  'clones',
  'color-enhance',
  'color-exchange',
  'color-to-alpha',
  'colors',
  'composite-transform',
  'contrast-curve',
  'convolve1',
  'convolve2',
  'dropshadow-json',
  'edge',
  'exposure',
  'fattal02',
  'gamma',
  'grey-json',
  'hdr-color',
  'illusion',
  'image-compare',
  'lens-flare',
  'mantiuk06',
  'matting-global',
  'noise-cell',
  'noise-hurl',
  'noise-simplex',
  'noise-solid',
  'posterize',
  'rectangles',
  'red-eye-removal',
  'reflect',
  'reflect2',
  'reinhard05',
  'rgb-params',
  'rotate',
  'rotate-on-center',
  'saturation',
  'scale-size-keepaspect',
  'simple-scale',
  'sinus',
  'stretch-contrast',
  'supernova',
  'transform',
  'weighted-blend',
]
if cairo.found()
  composition_tests += 'gegl'
endif
if libumfpack.found()
  composition_tests += 'matting-levin'
endif

composition_tests_without_opencl = [
  'color-reduction',
  'pnm-ascii-load',
  'pnm-raw-load',
  'rgbe-load',
  'rgbe-save',
  'shift',
  'stretch-contrast-hsv',
  'tile',
]
if jasper.found()
  composition_tests_without_opencl += 'jp2-load'
endif
if libjpeg.found()
  composition_tests_without_opencl += 'jpg-load-datauri'
endif
if libtiff.found()
  composition_tests_without_opencl += 'tiff-load'
endif

# Timeout for tests that require extra time to run
#   test-name : timeout <int>
composition_tests_set_timeout = {
  'pnm-ascii-load' : 120
}

# Tests that are expected to fail - must also appear in the main lists
composition_tests_fail = [
  'matting-global',
]
if not os_win32
  composition_tests_fail += 'matting-levin'
endif

# composition tests
tests = composition_tests + composition_tests_without_opencl
foreach _test : tests
  # without opencl
  _test_name = _test.underscorify()
  _xml_file = files('@0@.xml'.format(_test))
  _ref_dir = meson.current_source_dir() / 'reference'
  test(_test_name,
    test_runner,
    args: [
      '--test-name', _test,
      '--output-dir', meson.current_build_dir() / 'output',
      '--reference-path', _ref_dir,
      '--endian', host_machine.endian(),
      '--input-file', _xml_file,
      '--gegl-exe', gegl_bin,
      '--imgcmp-exe', gegl_imgcmp,
    ],
    env: gegl_test_env_no_ocl,
    should_fail: composition_tests_fail.contains(_test),
    suite: 'composition',
    is_parallel: gegl_test_parallel,
    timeout: composition_tests_set_timeout.get(_test, 30),
  )

  # with opencl
  if not composition_tests_without_opencl.contains(_test)
    test('@0@ (OpenCL)'.format(_test_name),
      test_runner,
      args: [
        '--test-name', _test,
        '--output-dir', meson.current_build_dir() / 'output-opencl',
        '--reference-path', _ref_dir,
        '--endian', host_machine.endian(),
        '--input-file', _xml_file,
        '--gegl-exe', gegl_bin,
        '--imgcmp-exe', gegl_imgcmp,
        '--with-opencl',
      ],
      env: gegl_test_env,
      should_fail: composition_tests_fail.contains(_test),
      suite: 'composition',
      is_parallel: gegl_test_parallel,
      timeout: composition_tests_set_timeout.get(_test, 30),
    )
  endif
endforeach
