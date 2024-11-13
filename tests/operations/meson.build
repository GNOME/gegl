
# Source operations
exclude = [
  'gegl:buffer-source',
  'gegl:exr-load',
  'gegl:ff-load',
  'gegl:gegl-buffer-load',
  'gegl:gif-load',
  'gegl:icc-load',
  'gegl:jp2-load',
  'gegl:jpg-load',
  'gegl:load',
  'gegl:magick-load',
  'gegl:open-buffer',
  'gegl:pdf-load',
  'gegl:pixbuf',
  'gegl:png-load',
  'gegl:ppm-load',
  'gegl:rawbayer-load',
  'gegl:raw-load',
  'gegl:rgbe-load',
  'gegl:svg-load',
  'gegl:tiff-load',
  'gegl:v4l',
  'gegl:v4l2',
  'gegl:webp-load',
]
# Programming operations
exclude += [
  'gegl:cache',
  'gegl:cast-format',
  'gegl:clone',
  'gegl:convert-format',
  'gegl:copy-buffer',
  'gegl:image-compare',
  'gegl:line-profile',
  'gegl:nop',
]
# Other operations
exclude += [
  'gegl:exp-combine',
  'gegl:gegl',
  'gegl:introspect',
  'gegl:lcms-from-profile',
  'gegl:mblur',
  'gegl:json:grey2',
]

# Crashing - workshop
exclude += [
  'gegl:alpha-inpaint',
  'gegl:integral-image',
]

gegl_test_ops_env = gegl_test_env
gegl_test_ops_env.set('BABL_TOLERANCE', '0.0')

test(
  'ops_stable',
  gegl_tester,
  args: [
    '--data-directory', project_source_root / 'docs' / 'images',
    '--output-directory', meson.current_build_dir(),
    '--all',
    '--exclusion-pattern', '|'.join(exclude),
  ],
  env: gegl_test_ops_env,
  protocol: 'tap',
  suite: 'operations',
  is_parallel: gegl_test_parallel,
  timeout: 300,
)
