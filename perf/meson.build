
perf_tests = [
  'bcontrast-4x',
  'bcontrast-minichunk',
  'bcontrast',
  'blur',
  'gegl-buffer-access',
  'init',
  'rotate',
  'samplers',
  'saturation',
  'scale',
  'translate',
  'unsharpmask',
]

foreach testname : perf_tests
  perf_test_exe = executable(testname,
    'test-' + testname + '.c',
    include_directories: [ rootInclude, geglInclude, ],
    dependencies: [
      babl,
      glib,
      gobject,
    ],
    link_with: [ gegl_lib, ],
    c_args: [
      '-DG_DISABLE_SINGLE_INCLUDES',
      '-DGLIB_DISABLE_DEPRECATION_WARNINGS',
      '-DCLUTTER_DISABLE_DEPRECATION_WARNINGS',
      '-DTESTS_DATA_DIR="@0@"'.format( project_source_root / 'tests' / 'data'),
    ],
    install: false,
  )

  benchmark('Perf Test ' + testname, perf_test_exe,
    env: [
      'GEGL_PATH=' + project_build_root / 'operations',
      'GEGL_USE_OPENCL=no',
    ],
  )
endforeach
