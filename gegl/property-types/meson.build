gegl_property_types_sources = files(
  'gegl-audio-fragment.c',
  'gegl-color.c',
  'gegl-curve.c',
  'gegl-paramspecs.c',
  'gegl-path.c',
)

gegl_property_types_lib = static_library('gegl_property_types',
  gegl_property_types_sources,
  include_directories: [ rootInclude, geglInclude, ],
  dependencies: [
    babl,
    glib,
  ],
)
