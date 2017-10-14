project('gegl', 'c',
  version: '0.3.21',
  license: 'GPL3+',
  meson_version: '>= 0.43',
)

i18n = import('i18n')
gnome = import('gnome')

# Versioning
gegl_version_split = meson.project_version().split('.')
gegl_major_version = gegl_version_split[0]
gegl_minor_version = gegl_version_split[1]
gegl_micro_version = gegl_version_split[2]
gegl_api_version = '.'.join([gegl_major_version, gegl_minor_version])

# Options (for more info, see meson_options.txt)
docs_enabled = get_option('docs')
workshop_enabled = get_option('workshop')
gexiv2_support = get_option('gexiv2')
mrg_support = get_option('mrg')
cairo_support = get_option('cairo')
pango_support = get_option('pango')
pangocairo_support = get_option('pangocairo')
gdk_pixbuf_support = get_option('gdk-pixbuf')
lensfun_support = get_option('lensfun')
libpng_support = get_option('libpng')
librsvg_support = get_option('librsvg')
libv4l2_support = get_option('libv4l2')
openexr_support = get_option('openexr')
sdl_support = get_option('sdl')
spiro_support = get_option('spiro')
lua_support = get_option('lua')
libraw_support = get_option('libraw')
webp_support = get_option('webp')

# Some variables
prefix = get_option('prefix')
config_h_dir = include_directories('.')
locale_dir = join_paths(prefix, get_option('localedir'))
pkgdatadir = join_paths(get_option('datadir'), meson.project_name())
libdir = get_option('libdir')
po_dir = join_paths(meson.source_root(), 'po')

# Dependencies
cc = meson.get_compiler('c')

libmath = cc.find_library('m')
babl = dependency('babl', version: '>= 0.1.34')
# glib packages
glib = dependency('glib-2.0', version: '>= 2.44')
gobject = dependency('gobject-2.0')
gmodule = dependency('gmodule-2.0')
json_glib = dependency('json-glib-1.0')
gthread = dependency('gthread-2.0')
gio = dependency('gio-2.0')
if target_machine.system() == 'windows'
  gio_os = dependency('gio-windows-2.0')
else
  gio_os = dependency('gio-unix-2.0')
endif

pango = dependency('pango', version: '>= 1.38.0')
perl = find_program('perl5', 'perl', 'perl5.005', 'perl5.004')
ruby = find_program('ruby', required: false)
if not ruby.found()
  warning('Could not find Ruby interpreter. The HTML API reference will not be updated.')
endif
enscript = find_program('enscript', required: false)
asciidoc = find_program('asciidoc', required: false)
dot = find_program('dot', required: false)
w3m = find_program('w3m', required: false)
python = find_program('python2.5', required: false) # used for introspection unittests
# XXX pygobject
poly2tri_c = dependency('poly2tri-c', version: '>= 0.0.0', required: false)
if not poly2tri_c.found()
  message('Couldn\'t find poly2tri-c, falling back to internal version')
endif

if gexiv2_support
  gexiv2 = dependency('gexiv2')
endif
if mrg_support
  mrg = dependency('mrg', required: false) # XXX normally required: true I think?
endif
if cairo_support
  cairo = dependency('cairo', version: '>= 1.1.12')
endif
if pango_support
  pango = dependency('pango', version: '>= 1.38.0')
endif
if pangocairo_support
  pangocairo = dependency('pangocairo', version: '>= 1.38.0')
endif
if gdk_pixbuf_support
  gdk_pixbuf = dependency('gdk-pixbuf-2.0', version: '>= 2.32.0')
endif
if lensfun_support
  lensfun = dependency('lensfun', version: '>= 0.2.5')
endif
# XXX jpeg
if libpng_support
  libpng = dependency('libpng')
endif
if librsvg_support
  librsvg = dependency('librsvg-2.0', version: '>= 2.40.6')
endif
if libv4l2_support
  if not cc.has_header('linux/videodev2.h')
    error('Couln\'t find header file <linux/videodev2.h>, needed for libv4l2 support.')
  endif
  libv4l2 = dependency('libv4l2', version: '>= 1.0.1')
endif
if openexr_support
  openexr = dependency('OpenEXR', version: '>= 1.6.1')
endif
if sdl_support
  sdl = dependency('sdl') # XXX correct? autoconf uses sdl-config
  # XXX something with running sdl-config to get the right flags?
endif
if libraw_support
  libraw = dependency('libraw', version: '>= 0.15.4')
endif
# XXX jasper
# XXX graphviz
if lua_support
  lua_min_version = '5.1.0'
  lua = dependency('lua', version: '>= '+lua_min_version, required: false)
  if not lua.found()
    lua = dependency('lua5.1', version: '>= '+lua_min_version, required: false)
    if not lua.found()
      lua = dependency('luajit', version: '>= 2.0.4')
    endif
  endif
endif
# XXX libavformat
# XXX lib4l
# XXX lcms
if spiro_support
  spiro = cc.find_library('spiro')
endif
# XXX exiv2
# XXX umfpack
if webp_support
  webp = dependency('libwebp', version: '>= 0.5.0')
endif
# XXX mmx / sse

# Configuration
conf = configuration_data()
conf.set_quoted('GETTEXT_PACKAGE', meson.project_name())
conf.set_quoted('GEGL_LOCALEDIR', locale_dir)
conf.set_quoted('GEGL_LIBRARY', '@0@-@1@'.format(meson.project_name(), gegl_api_version))
conf.set_quoted('LIBDIR', libdir)
conf.set_quoted('top_srcdir', meson.source_root())
conf.set_quoted('GEGL_VERSION', meson.project_version())
configure_file(output: 'config.h', configuration: conf)

# Custom stuff
# custom_target('NEWS',
#   build_by_default: true,
#   output: join_paths('docs', 'NEWS.html'),
#   command: [ w3m,
#     '-cols', '72',
#     '-dump', '@OUTPUT@', # XXX isn't this @OUTPUT@ ?
#   ],
# )

# Common dependencies
common_deps = [
  glib,
  gobject,
  gmodule,
  gio,
  gio_os,
  json_glib,
  gthread,
]

# Pkg-conf
pkg_conf = configuration_data()
pkg_conf.set_quoted('prefix', prefix)
# pkg_conf.set_quoted('exec_prefix', )
pkg_conf.set_quoted('libdir', libdir)
# pkg_conf.set_quoted('includedir', includedir)
pkg_conf.set_quoted('PACKAGE_NAME', meson.project_name())
pkg_conf.set_quoted('GEGL_API_VERSION', gegl_api_version)
pkg_conf.set_quoted('GEGL_REAL_VERSION', meson.project_version())
pkg_conf.set_quoted('GLIB_PACKAGES', 'gobject-2.0 gmodule-2.0 gio-2.0 json-glib-1.0') # XXX add gio-unix or gio-windows
pkg_conf.set_quoted('MATH_LIB', '-lm')
configure_file(
  input: 'gegl.pc.in',
  output: 'gegl.pc',
  configuration: pkg_conf,
)

# Subfolders
subdir('libs')
subdir('opencl')
subdir('gegl')
subdir('libs/npd') # depends on gegl
subdir('seamless-clone')
subdir('bin')
subdir('gcut')
subdir('tools')
subdir('operations')
subdir('examples')
subdir('tests')
subdir('perf')
subdir('po')
if docs_enabled
  subdir('docs')
endif
