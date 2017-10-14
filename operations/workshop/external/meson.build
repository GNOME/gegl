
if cairo_support
  line_profile_dependencies = [
    common_deps,
    libgegl_dep,
    cairo,
    pango,
    babl,
  ]

  line_profile_op = library('line-profile',
    'line-profile.c',
    dependencies: line_profile_dependencies,
    include_directories: config_h_dir,
  )
endif

if lua_support
  gluas_dependencies = [
    common_deps,
    libgegl_dep,
    lua,
    babl,
  ]

  gluas_op = library('gluas',
    'gluas.c',
    dependencies: gluas_dependencies,
    include_directories: config_h_dir,
  )
endif

if lensfun_support
  lens_correct_op_dependencies = [
    common_deps,
    libgegl_dep,
    lensfun,
  ]

  lens_correct_op = library('lens-correct',
    'lens-correct.c',
    dependencies: lens_correct_op_dependencies,
    include_directories: config_h_dir,
  )
endif

if libv4l2_support
  v4l2_op_dependencies = [
    common_deps,
    libgegl_dep,
    libv4l2,
  ]

  v4l2_op = library('v4l2',
    'v4l2.c',
    dependencies: v4l2_op_dependencies,
    include_directories: config_h_dir,
  )
endif
