examples = [
  '2geglbuffer',
  'gegl-convert',
  'gegl-slicer',
  'video-invert',
  'geglbuffer-add-image',
  'geglbuffer-clock',
  'frame-counter',
  'hello-world',
]

example_dependencies = [
  common_deps,
  babl,
  sdl,
  libmath,
  libgegl_dep,
]

foreach example: examples
  executable(example,
    '@0@.c'.format(example),
    dependencies: example_dependencies,
  )
endforeach

# sdl and gegl examples
