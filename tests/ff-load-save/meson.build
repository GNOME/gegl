
ff_video_tests = {
  'mpeg4-128kb.avi': [
      '--video-codec mpeg4',
      '--video-bit-rate', '128',
    ],
  'mpeg4-512kb.avi': [
      '--video-codec mpeg4',
      '--video-bit-rate', '512',
    ],
  '512kb.mp4': [
      '--video-codec mpeg4',
      '--video-bit-rate', '512',
    ],
  '128kb.mp4': [
      '--video-codec mpeg4',
      '--video-bit-rate', '128',
    ],
  '128kb-12fps.mp4': [
      '--video-codec mpeg4',
      '--video-bit-rate', '128',
      '--fps', '12',
    ],
  '128kb-100fps.mp4': [
      '--video-codec mpeg4',
      '--video-bit-rate', '128',
      '--fps', '100',
    ],
  '512kb-28fps.ogv': [
      '--video-bit-rate', '128',
      '--fps', '28',
    ],
}

foreach _test, _args : ff_video_tests
  test(_test.underscorify(),
    frame_counter,
    args: [
      _args,
      _test,
    ],
    env: gegl_test_env,
    workdir: meson.current_build_dir(),
    suite: 'ff-load-save',
    is_parallel: false,
  )
  test(_test.underscorify(),
    gegl_video,
    args: [
      _test,
      '-s', '74',
      '-e', '74',
      '-of', '@0@-'.format(_test)
    ],
    env: gegl_test_env,
    workdir: meson.current_build_dir(),
    suite: 'ff-load-save',
    is_parallel: false,
  )
endforeach
