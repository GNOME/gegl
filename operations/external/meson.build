
if pangocairo_support
  library('text',
    'text.c',
    dependencies: [ libgegl_dep, pangocairo ],
    install: true,
    install_dir: operations_ext_dir,
  )
endif

if cairo_support
  library('path',
    'path.c',
    dependencies: [ libgegl_dep, cairo ],
    install: true,
    install_dir: operations_ext_dir,
  )
  library('vector-fill',
    'vector-fill.c',
    dependencies: [ libgegl_dep, cairo ],
    install: true,
    install_dir: operations_ext_dir,
  )
  library('vector-stroke',
    'vector-stroke.c',
    dependencies: [ libgegl_dep, cairo ],
    install: true,
    install_dir: operations_ext_dir,
  )
endif

# XXX the rest
