
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
# Sink operations
exclude += [
  'gegl:buffer-sink',
  'gegl:display',
  'gegl:exr-save',
  'gegl:ff-save',
  'gegl:gegl-buffer-save',
  'gegl:icc-save',
  'gegl:jpg-save',
  'gegl:npy-save',
  'gegl:png-save',
  'gegl:ppm-save',
  'gegl:rgbe-save',
  'gegl:save',
  'gegl:save-pixbuf',
  'gegl:sdl2-display',
  'gegl:tiff-save',
  'gegl:webp-save',
  'gegl:write-buffer',
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
]
# Crashing - workshop
exclude += [
  'gegl:alpha-inpaint',
  'gegl:integral-image',
]
# Leaking buffers - Workshop
exclude += [
  'gegl:aces-rrt',
]

doc_ops_img_cmd = [
  gegl_tester,
  '--data-directory', project_source_root / 'docs' / 'images',
  '--output-directory', '@OUTDIR@',
  '--all',
  '--exclusion-pattern', '|'.join(exclude),
]

# generate operation image examples
doc_operations_img = custom_target(
  'doc-operations-img',
  output: 'gegl-oilify.png',
  command: [
    env,
    doc_env,
    doc_ops_img_cmd,
    ],
  console: true,
  depends: gegl_operations,
  build_by_default: true,
)
