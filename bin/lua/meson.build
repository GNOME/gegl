lua_files = files(
  'cairo_h.lua',
  'cairo.lua',
  'gegl_box-blur.lua',
  'gegl_c2g.lua',
  'gegl_crop.lua',
  'gegl_edge-neon.lua',
  'gegl_fill-path.lua',
  'gegl_gaussian-blur.lua',
  'gegl_linear-gradient.lua',
  'gegl_median-blur.lua',
  'gegl_radial-gradient.lua',
  'gegl_rectangle.lua',
  'gegl_rotate.lua',
  'gegl_snn-mean.lua',
  'gegl_stress.lua',
  'gegl_translate.lua',
  'gegl_unsharp-mask.lua',
  'gegl_vector-stroke.lua',
  'init.lua',
  'mime.lua',
  'mrg.lua',
  'mrl.lua',
  'preferences.lua',
  'viewer.lua',
)

if lua.found()
  install_data(
    lua_files,
    install_dir: get_option('datadir') /  api_name / 'lua'
  )
endif
