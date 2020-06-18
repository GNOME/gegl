
examples = [
  { 'name': '2geglbuffer', },
  { 'name': 'frame-counter', },
  { 'name': 'gegl-convert', },
  { 'name': 'gegl-slicer', },
  { 'name': 'geglbuffer-add-image', },
  { 'name': 'geglbuffer-clock', },
  { 'name': 'hello-world', },
  { 'name': 'video-invert', },
]

if sdl2.found()
  examples += {
    'name': 'sdl-draw',
    'deps': [ sdl2, math, ]
  }
endif
if gexiv2.found()
  examples += {
    'name': 'gegl-video',
    'deps': [ gexiv2, ]
  }
endif


foreach example : examples
  example_name = example.get('name')
  example_srcs = example.get('srcs', example_name + '.c')
  example_deps = example.get('deps', [])

  exe = executable(example_name,
    example_srcs,
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

  # Fore use as tests for ff-load-save
  if example_name == 'gegl-video'
    gegl_video = exe
  endif
  if example_name == 'frame-counter'
    frame_counter = exe
  endif
endforeach
