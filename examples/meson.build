
examples = [
  [ '2geglbuffer',          [ ] ],
  [ 'frame-counter',        [ ] ],
  [ 'gegl-convert',         [ ] ],
  [ 'gegl-slicer',          [ ] ],
  [ 'geglbuffer-add-image', [ ] ],
  [ 'geglbuffer-clock',     [ ] ],
  [ 'hello-world',          [ ] ],
  [ 'video-invert',         [ ] ],
]

if get_option('with-sdl1')
  examples += [
    [ 'sdl-draw',           [ sdl1, math, ] ],
  ]
endif
if get_option('with-gexiv2')
  examples += [
    [ 'gegl-video',         [ gexiv2, ] ],
  ]
endif


foreach example : examples
  example_name = example[0]
  example_deps = example[1]

  exe = executable(example_name,
    example_name + '.c',
    include_directories: [ geglInclude, ],
    dependencies: [
      babl,
      glib,
      gobject,
      example_deps,
    ],
    link_with: [ gegl_lib, ],
    install: false,
  )

  if example_name == 'gegl-video'
    gegl_video_example = exe
  endif
  if example_name == 'frame-counter'
    frame_counter_example = exe
  endif
endforeach
