# List of [ name, sources, dependencies, ]
operations = [
  [   'ppm-load',     [ 'ppm-load.c', ],    [ ], ],
  [   'ppm-save',     [ 'ppm-save.c', ],    [ ], ],

  [   'npy-save',     [ 'npy-save.c', ],    [ ], ],

  [   'rgbe-load',    [ 'rgbe-load.c', ],   [ librgbe ], ],
  [   'rgbe-save',    [ 'rgbe-save.c', ],   [ librgbe ], ],
]


if get_option('with-pangocairo')
  operations += [
    [ 'text',         [ 'text.c', ],        [ pangocairo ], ],
  ]
endif

if get_option('with-cairo')
  operations += [
    [ 'path',         [ 'path.c', ],        [ cairo ], ],
    [ 'vector-fill',  [ 'vector-fill.c', ], [ cairo ], ],
    [ 'vector-stroke',[ 'vector-stroke.c',],[ cairo ], ],
    [ 'npd',          [ 'npd.c', ],         [ cairo, libnpd ], ],
  ]
endif

if get_option('with-libpng')
  operations += [
    [ 'png-load',     [ 'png-load.c', ],    [ libpng ], ],
    [ 'png-save',     [ 'png-save.c', ],    [ libpng ], ],
  ]
endif

if get_option('with-libjpeg')
  operations += [
    [ 'jpg-load',     [ 'jpg-load.c', ],    [ libjpeg ], ],
    [ 'jpg-save',     [ 'jpg-save.c', ],    [ libjpeg ], ],
  ]
endif

if get_option('with-cairo') and get_option('with-librsvg')
  operations += [
    [ 'svg-load',     [ 'svg-load.c', ],    [ librsvg, cairo ], ],
  ]
endif

if get_option('with-gdk-pixbuf')
  operations += [
    [ 'pixbuf-load',  [ 'pixbuf.c',      ], [ gdk_pixbuf ], ],
    [ 'pixbuf-save',  [ 'save-pixbuf.c', ], [ gdk_pixbuf ], ],
  ]
endif

if get_option('with-openexr')
  operations += [
    [ 'exr-load',     [ 'exr-load.cpp', ],  [ openexr ], ],
    [ 'exr-save',     [ 'exr-save.cc',  ],  [ openexr ], ],
  ]
endif

if get_option('with-sdl1')
  operations += [
    [ 'sdl-display',  [ 'sdl-display.c', ], [ sdl1 ], ],
  ]
endif

if get_option('with-libraw')
  operations += [
    [ 'raw-load',     [ 'raw-load.c', ],    [ libraw ], ],
  ]
endif

if get_option('with-libv4l')
  operations += [
    [ 'v4l',          [ 'v4l.c', ],         [ libv4l1, thread ], ],
  ]
endif

if get_option('with-jasper')
  operations += [
    [ 'jp2-load',     [ 'jp2-load.c', ],    [ jasper ], ],
  ]
endif

if get_option('with-libav')
  operations += [
    [ 'ff-load',      [ 'ff-load.c', ],     [ avlibs ], ],
    [ 'ff-save',      [ 'ff-save.c', ],     [ avlibs ], ],
  ]
endif

if get_option('with-umfpack')
  operations += [
    [ 'matting-levin',
      [ 'matting-levin.c', 'matting-levin-cblas.c', ],
      [ libumfpack ],
    ],
  ]
endif

if get_option('with-lcms')
  operations += [
    [ 'lcms-from-profile',
      [ 'lcms-from-profile.c', ],
      [ lcms ],
    ],
  ]
endif

if get_option('with-libtiff')
  operations += [
    [ 'tiff-load',    [ 'tiff-load.c', ],   [ libtiff ], ],
    [ 'tiff-save',    [ 'tiff-save.c', ],   [ libtiff ], ],
  ]
endif

if get_option('with-webp')
  operations += [
    [ 'webp-load',    [ 'webp-load.c', ],   [ libwebp ], ],
    [ 'webp-save',    [ 'webp-save.c', ],   [ libwebp ], ],
  ]
endif


foreach operation : operations
  libname = operation[0]
  libsrcs = operation[1]
  libdeps = operation[2]

  shared_library(libname,
    libsrcs,
    dependencies: [ babl, gio, glib, gobject, math, libdeps, ],
    include_directories: [ rootInclude, geglInclude, ],
    link_with: [ gegl_lib, ],
    name_prefix: '',
    install: true,
    install_dir: join_paths(get_option('libdir'), lib_name)
  )
endforeach
