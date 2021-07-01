
operations = [
  { 'name': 'ppm-load' },
  { 'name': 'ppm-save' },
  { 'name': 'npy-save' },
  { 'name': 'rgbe-load', 'deps': librgbe },
  { 'name': 'rgbe-save', 'deps': librgbe },
  { 'name': 'gif-load',  'deps': libnsgif },
]


if pangocairo.found()
  operations += { 'name': 'text', 'deps': pangocairo }
endif

if cairo.found()
  operations += [
    { 'name': 'path',           'deps': cairo },
    { 'name': 'vector-stroke',  'deps': cairo },
    { 'name': 'npd',            'deps': [ cairo, libnpd ] },
  ]
endif

if libpng.found()
  operations += [
    { 'name': 'png-load', 'deps': libpng },
    { 'name': 'png-save', 'deps': libpng },
  ]
endif

if poppler.found()
  operations += [
    { 'name': 'pdf-load', 'deps': poppler }
    ]
endif

if libjpeg.found()
  operations += [
    { 'name': 'jpg-load', 'deps': libjpeg },
    { 'name': 'jpg-save', 'deps': libjpeg },
  ]
endif

if cairo.found() and librsvg.found()
  operations += [
    { 'name': 'svg-load', 'deps': [ librsvg, cairo ], },
  ]
endif

if gdk_pixbuf.found()
  operations += [
    { 'name': 'pixbuf-load',  'srcs': 'pixbuf.c',      'deps': gdk_pixbuf },
    { 'name': 'pixbuf-save',  'srcs': 'save-pixbuf.c', 'deps': gdk_pixbuf },
  ]
endif

if openexr.found()
  operations += [
    { 'name': 'exr-load',   'srcs': 'exr-load.cpp', 'deps': openexr },
    { 'name': 'exr-save',   'srcs': 'exr-save.cc',  'deps': openexr },
  ]
endif

if sdl1.found()
  operations += { 'name': 'sdl-display', 'deps': sdl1 }
endif

if sdl2.found()
  operations += { 'name': 'sdl2-display', 'deps': sdl2 }
endif

if libraw.found()
  operations += { 'name': 'raw-load', 'deps': libraw }
endif

if libv4l1.found()
  operations += { 'name': 'v4l', 'deps': [ libv4l1, thread ], }
endif

if jasper.found()
  operations += { 'name': 'jp2-load', 'deps': jasper }
endif

if avlibs_found
  operations += [
    { 'name': 'ff-load', 'deps': avlibs },
    { 'name': 'ff-save', 'deps': avlibs },
  ]
endif

if libumfpack.found()
  operations += {
    'name': 'matting-levin',
    'srcs': [ 'matting-levin.c', 'matting-levin-cblas.c', ],
    'deps': libumfpack
  }
endif

if lcms.found()
  operations += { 'name': 'lcms-from-profile', 'deps': lcms }
endif

if libtiff.found()
  operations += [
    { 'name': 'tiff-load', 'deps': libtiff },
    { 'name': 'tiff-save', 'deps': libtiff },
  ]
endif

if libwebp.found()
  operations += [
    { 'name': 'webp-load', 'deps': libwebp },
    { 'name': 'webp-save', 'deps': libwebp },
  ]
endif


foreach operation : operations
  libname = operation.get('name')
  libsrcs = operation.get('srcs', libname + '.c')
  libdeps = operation.get('deps', [])

  gegl_operations += shared_library(libname,
    libsrcs,
    dependencies: [ babl, gio, glib, gobject, math, libdeps, ],
    include_directories: [ rootInclude, geglInclude, ],
    link_with: [ gegl_lib, ],
    name_prefix: '',
    install: true,
    install_dir: get_option('libdir') / api_name,
  )
endforeach
