if get_option('with-gexiv2')

  tests_args = [
    [ '--video-codec', 'mpeg4', '--video-bit-rate', '128',                 'mpeg4-128kb.avi', ],
    [ '--video-codec', 'mpeg4', '--video-bit-rate', '512',                 'mpeg4-512kb.avi', ],
    [ '--video-codec', 'mpeg4', '--video-bit-rate', '512',                 '512kb.mp4', ],
    [ '--video-codec', 'mpeg4', '--video-bit-rate', '128',                 '128kb.mp4', ],
    [ '--video-codec', 'mpeg4', '--video-bit-rate', '128', '--fps', '12',  '128kb-12fps.mp4', ],
    [ '--video-codec', 'mpeg4', '--video-bit-rate', '128', '--fps', '100', '128kb-100fps.mp4', ],
    [                           '--video-bit-rate', '512', '--fps', '28',  '512kb-28fps.ogv', ],
  ]
  foreach args : tests_args
    filename = args[-1]

    # Regroup the two tests as a single test unit
    test('ff-load-save_' + filename.underscorify(),
      bash,
      args: [ '-c', ' '.join(
          [ frame_counter_example.full_path() ]
        + args
        + [ '&&' ]
        + [ gegl_video_example.full_path() ]
        + [ filename, '-s', '74', '-e', '74', '-of', filename + '-' ])
      ],
      env: [ 'GEGL_PATH=' + join_paths(meson.build_root(), 'operations'), ],
      workdir: meson.current_build_dir(),
      suite: 'ff-load-save',
    )
  endforeach

endif
