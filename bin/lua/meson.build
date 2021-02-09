
lua_files = files(
  'cairo_h.lua',
  'cairo.lua',
  'gegl_box-blur.lua',
  'gegl_c2g.lua',
  'gegl_edge-neon.lua',
  'gegl_gaussian-blur.lua',
  'gegl_linear-gradient.lua',
  'gegl_median-blur.lua',
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

lua_symlink_files = {
  'gegl_crop.lua' : 'gegl_rectangle.lua',
  'gegl_fill-path.lua' : 'gegl_vector-stroke.lua',
  'gegl_radial-gradient.lua' : 'gegl_linear-gradient.lua',
}
# Windows doesn't like *nix symlinks so we check if the symlinked lua
# files are valid and if not copy the original to the build dir in place
# of the symlink 
fs=import('fs')
foreach _link, _file : lua_symlink_files
  if fs.is_symlink(_link)
    lua_files += _link
  else
    lua_files += configure_file(
      input: _file,
      output: _link,
      copy: true,
    )
  endif
endforeach

if lua.found()
  install_data(
    lua_files,
    install_dir: get_option('datadir') / api_name / 'lua'
  )
endif
