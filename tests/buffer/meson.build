
buffer_tests_list = run_command(env,
  'find', join_paths(meson.current_source_dir(), 'tests'), '-name', '*.c',
  '-printf', '%f\n'
).stdout().strip().split('\n')


buffer_test_run_script = find_program('buffer-tests-run.sh')

foreach test_file : buffer_tests_list
  test_name = test_file.split('.c')[0]

  test_conf = configuration_data()
  test_conf.set_quoted('TEST_NAME', test_name)
  test_conf.set('TESTNAME', test_name)
  test_conf.set('TEST_INCLUDE', '#include "' + join_paths('tests', test_file) + '"')
  test_source = configure_file(
    input : 'buffer-test.c.in',
    output: test_name + '_test_run.c',
    configuration: test_conf,
  )

  test_exe = executable(test_name,
    test_source,
    include_directories: [ rootInclude, geglInclude, ],
    link_with: [
      gegl_lib,
    ],
    dependencies: [
      babl,
      glib,
      gobject,
      math,
    ],
  )

  test(test_name,
  buffer_test_run_script,
    args: [ test_name, test_exe.full_path(), ],
    env: [
      'GEGL_PATH=' + join_paths(meson.build_root(), 'operations'),
      'GEGL_SWAP=RAM',
      'REFERENCE_DIR='+join_paths(meson.source_root(),'tests','buffer','reference'),
    ],
    workdir: meson.current_build_dir(),
    suite: 'buffers',
  )

endforeach
