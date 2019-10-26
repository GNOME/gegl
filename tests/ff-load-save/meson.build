if avlibs_found and gexiv2.found()

  test('ff-load-save',
    find_program('tests_ff_load_save.sh'),
    env: [ 'MESON_BUILD_ROOT=' + meson.build_root() ],
    workdir: meson.current_build_dir(),
    suite: 'ff-load-save',
    protocol: 'tap',
    timeout: 120,
    is_parallel: false,
  )

endif
