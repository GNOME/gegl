project('gegl',
  'c', 'cpp',
  license: 'GPL3+',
  version: '0.4.29',
  meson_version: '>=0.54.0',
  default_options: [
    'c_std=gnu11',
    'cpp_std=gnu++14',
    'buildtype=debugoptimized',
  ],
)

# Making releases on the stable branch:
#   micro_version += 1;
#   interface_age += 1;
#   binary_age += 1;
# if any functions have been added,
#    set interface_age to 0.
# if backwards compatibility has been broken,
#    set binary_age _and_ interface_age to 0.

config    = configuration_data()

pkgconfig = import('pkgconfig')
i18n      = import('i18n')
gnome     = import('gnome')
python    = import('python').find_installation()

cc        = meson.get_compiler('c')
cpp       = meson.get_compiler('cpp')
buildtype = get_option('buildtype')

gegl_prefix = get_option('prefix')
gegl_libdir = join_paths(gegl_prefix, get_option('libdir'))


project_build_root = meson.current_build_dir()
project_source_root = meson.current_source_dir()

################################################################################
# Project infos

version = meson.project_version()
array_version = version.split('.')
major_version = array_version[0].to_int()
minor_version = array_version[1].to_int()
micro_version = array_version[2].to_int()

api_version = '@0@.@1@'.format(major_version, minor_version)
api_name    = meson.project_name() + '-' + api_version
gettext_package = api_name

stability_version_number = (major_version != 0 ? minor_version : micro_version)
stable = (stability_version_number % 2 == 0)

config.set        ('GEGL_MAJOR_VERSION',  '@0@'.format(major_version))
config.set        ('GEGL_MINOR_VERSION',  '@0@'.format(minor_version))
config.set        ('GEGL_MICRO_VERSION',  '@0@'.format(micro_version))
config.set        ('GEGL_UNSTABLE',       not stable)

config.set_quoted ('GEGL_LIBRARY',        '@0@'.format(api_name))
config.set_quoted ('GETTEXT_PACKAGE',     '@0@'.format(gettext_package))

# Libtool versionning
interface_age = 1

binary_age = 100 * minor_version + micro_version
lt_current = binary_age - interface_age
so_version  = '@0@.@1@.@2@'.format(0, lt_current, interface_age)

################################################################################
# Host system detection

host_os = host_machine.system()
os_win32  = host_os.contains('mingw') or host_os.contains('windows')
os_android= host_os.contains('android')
os_osx    = host_os.contains('darwin')

if os_osx and cc.get_id() != 'clang'
  error('You should use Clang on OSx.')
endif


host_cpu_family = host_machine.cpu_family()
message('Host machine cpu family: ' + host_cpu_family)

if   host_cpu_family == 'x86'
  have_x86 = true
  config.set10('ARCH_X86',    true)
elif host_cpu_family == 'x86_64'
  have_x86 = true
  config.set10('ARCH_X86',    true)
  config.set10('ARCH_X86_64', true)
elif host_cpu_family == 'ppc'
  have_ppc = true
  config.set10('ARCH_PPC',    true)
elif host_cpu_family == 'ppc64'
  have_ppc = true
  config.set10('ARCH_PPC',    true)
  config.set10('ARCH_PPC64',  true)
endif

################################################################################
# Compiler arguments

cflags_common = []
cflags_c      = []
cflags_cpp    = []

cflags_common += '-DHAVE_CONFIG_H'

if buildtype == 'debugoptimized' or buildtype == 'release'
  cflags_common += '-Ofast'
endif

if buildtype == 'debugoptimized' or buildtype == 'debug'
  cflags_common += '-DGEGL_ENABLE_DEBUG'
endif

cflags_common += [
  '-Winit-self',
  '-Wmissing-declarations',
  '-Wpointer-arith',
  '-Wno-deprecated-declarations',
]
cflags_c = [
  '-Wmissing-prototypes',
  '-Wold-style-definition',
]

if os_win32
  cflags_common += '-D_FILE_OFFSET_BITS=64'
endif

cflags_c   = cflags_common + cflags_c
cflags_cpp = cflags_common + cflags_cpp

add_project_arguments(cc.get_supported_arguments(cflags_c), language: 'c')
add_project_arguments(cpp.get_supported_arguments(cflags_cpp), language: 'cpp')

################################################################################
# Build Utilities

bash         = find_program('bash')

asciidoc     = find_program('asciidoc',      required: false, native: true)
dot          = find_program('dot',           required: false, native: true)
perl         = find_program('perl5', 'perl', required: false, native: true)
rsvg_convert = find_program('rsvg-convert',  required: false, native: true)
ruby         = find_program('ruby',          required: false, native: true)

################################################################################
# Required Dependencies

config.set('HAVE_UNISTD_H',    cc.has_header('unistd.h'))
config.set('HAVE_EXECINFO_H',  cc.has_header('execinfo.h'))
config.set('HAVE_FSYNC',       cc.has_function('fsync'))
config.set('HAVE_MALLOC_TRIM', cc.has_function('malloc_trim'))
config.set('HAVE_STRPTIME',    cc.has_function('strptime'))

math    = cc.find_library('m', required: false)
libdl   = cc.find_library('dl', required : false)
thread  = dependency('threads')

babl      = dependency('babl',        version: '>=0.1.84')
glib      = dependency('glib-2.0',    version: '>=2.44.0')
gobject   = dependency('gobject-2.0', version: '>=2.44.0')
gmodule   = dependency('gmodule-2.0', version: '>=2.44.0')
gthread   = dependency('gthread-2.0', version: '>=2.44.0')
gio_os    = os_win32 ? 'gio-windows-2.0' : 'gio-unix-2.0'
gio       = [
            dependency('gio-2.0',     version: '>=2.44.0'),
            dependency(gio_os,        version: '>=2.44.0'),
]
json_glib = dependency('json-glib-1.0', version: '>=1.2.6')

# Required libraries eventually provided in subprojects/ subdir

poly2tri_c= dependency('poly2tri-c',  version: '>=0.0.0',
  fallback: ['poly2tri-c', 'poly2tri_c'],
  required: false,
)

libnsgif = dependency('libnsgif',
  fallback: ['libnsgif', 'libnsgif'],
)

################################################################################
# Optionnal Dependencies

# GEGL library
if get_option('introspection') != 'false'
  g_ir = dependency('gobject-introspection-1.0', version: '>=1.32.0',
    required: get_option('introspection') == 'true' ? true : false,
  )
else
  g_ir = disabler()
endif
if g_ir.found()
  vapigen = dependency('vapigen',     version:'>=0.20.0',
    required: get_option('vapigen')
  )
else
  vapigen = disabler()
endif

# GEGL binary
gexiv2    = dependency('gexiv2',      version: '>=0.0.0',
  required: get_option('gexiv2')
)
config.set('HAVE_GEXIV2', gexiv2.found())
libpng    = dependency('libpng',      version: '>=1.6.0',
  required: get_option('libpng')
)
lua       = dependency('luajit',      version: '>=2.0.4',
  required: get_option('lua')
)
config.set('HAVE_LUA', lua.found())
mrg       = dependency('mrg',         version: '>=0.0.0',
  required: get_option('mrg')
)
config.set('HAVE_MRG', mrg.found())

# Operations
gdk_pixbuf= dependency('gdk-pixbuf-2.0', version:'>=2.32.0',
  required: get_option('gdk-pixbuf')
)
cairo     = dependency('cairo',       version: '>=1.12.2',
  required: get_option('cairo')
)
pango     = dependency('pango',       version: '>=1.38.0',
  required: get_option('pango')
)
pangocairo= dependency('pangocairo',  version: '>=1.38.0',
  required: get_option('pangocairo')
)

jasper    = dependency('jasper',      version: '>=1.900.1',
  required: get_option('jasper')
)
lcms      = dependency('lcms2',       version: '>=2.8',
  required: get_option('lcms')
)
lensfun   = dependency('lensfun',     version: '>=0.2.5',
  required: get_option('lensfun')
)
libjpeg   = dependency('libjpeg',     version: '>=1.0.0',
  required: get_option('libjpeg')
)
libraw    = dependency('libraw',      version: '>=0.15.4',
  required: get_option('libraw')
)
librsvg   = dependency('librsvg-2.0', version: '>=2.40.6',
  required: get_option('librsvg')
)
libspiro  = dependency('libspiro',    version: '>=0.5.0',
  required: get_option('libspiro')
)
libtiff   = dependency('libtiff-4',   version: '>=4.0.0',
  required: get_option('libtiff')
)
libv4l1   = dependency('libv4l1',     version: '>=1.0.1',
  required: get_option('libv4l')
)
libv4l2   = dependency('libv4l2',     version: '>=1.0.1',
  required: get_option('libv4l2')
)
libwebp   = dependency('libwebp',     version:'>=0.5.0',
  required: get_option('webp')
)
maxflow   = dependency('maxflow',     version: '>=3.0.4',
  required: get_option('maxflow')
)
openexr   = dependency('OpenEXR',     version: '>=1.6.1',
  required: get_option('openexr')
)
poppler = dependency('poppler-glib', version: '>=0.71.0',
  required: get_option('poppler')
)
sdl1      = dependency('sdl',         version: '>=1.2.0',
  required: get_option('sdl1')
)
sdl2      = dependency('sdl2',        version: '>=2.0.5',
  required: get_option('sdl2')
)

# AV libs
libavcodec  = dependency('libavcodec',   version: '>=55.69.100',
  required: get_option('libav')
)
libavformat = dependency('libavformat',  version: '>=55.48.100',
  required: get_option('libav')
)
libavutil   = dependency('libavutil',    version: '>=55.92.100',
  required: get_option('libav')
)
libswscale  = dependency('libswscale',   version: '>=2.6.100',
  required: get_option('libav')
)
avlibs_found = (
  libavcodec.found() and
  libavformat.found() and
  libavutil.found() and
  libswscale.found()
)
avlibs = avlibs_found ? [libavcodec, libavformat, libavutil, libswscale] : []

# libumfpack
libumfpack = cc.find_library('umfpack', required: get_option('umfpack'))
if libumfpack.found()
  have_umfpack = cc.has_header('umfpack.h')
  have_ss_umfpack = cc.has_header_symbol(
    'suitesparse/umfpack.h',
    'umfpack_dl_solve'
  )

  if not (have_umfpack or have_ss_umfpack)
    if get_option('umfpack').auto()
      libumfpack = dependency('', required: false)
    else
      error('UmfPack library found but not headers.')
    endif
  endif
  config.set('HAVE_UMFPACK_H', have_umfpack)
  config.set('HAVE_SUITESPARSE_UMFPACK_H', have_ss_umfpack)
endif

# Tests
if g_ir.found()
  pygobject3 = dependency('pygobject-3.0', version: '>=3.2.0',
    required: get_option('pygobject')
  )
else
  pygobject3 = disabler()
endif

################################################################################
# Subdirs

configure_file(
  output: 'config.h',
  configuration: config
)

rootInclude = include_directories('.')

argvs_extract = find_program('tools/argvs_extract.sh')

subdir('libs/rgbe')
subdir('opencl')
subdir('gegl')
subdir('libs/npd')
subdir('seamless-clone')
subdir('bin')
subdir('tools')
subdir('operations')
subdir('examples')
subdir('tests')
subdir('perf')
subdir('po')
if get_option('docs')
  subdir('docs')
endif


pkgconfig.generate(filebase: 'gegl-' + api_version,
  name: 'GEGL',
  description: 'Generic Graphics Library',
  version: meson.project_version(),
  variables: 'pluginsdir=' + '${prefix}' / get_option('libdir') / api_name,
  requires: [
    gobject,
    gmodule,
    gio,
    json_glib,
    babl,
  ],
  libraries: [
    gegl_lib,
    gegl_npd_lib,
    math,
  ],
  subdirs: api_name,
)


################################################################################
# Build summary

summary(
  {
    'prefix': gegl_prefix,
    'libdir': get_option('libdir'),
  }, section: 'Directories'
)
summary(
  {
    'GEGL docs'       : get_option('docs'),
    'Build workshop'  : get_option('workshop'),
    'Introspection'   : g_ir.found(),
    'Vala support'    : vapigen.found(),
  }, section: 'Optional features'
)
summary(
  {
    'asciidoc'        : asciidoc.found(),
    'dot'             : dot.found(),
    'pygobject'       : pygobject3.found(),
    'rsvg-convert'    : rsvg_convert.found(),
    'Ruby'            : ruby.found(),
  }, section: 'Optional build utilities'
)
summary(
  {
    'avlibs'          : avlibs_found,
    'Cairo'           : cairo.found(),
    'GDKPixbuf'       : gdk_pixbuf.found(),
    'gexiv2'          : gexiv2.found(),
    'Jasper'          : jasper.found(),
    'JPEG'            : libjpeg.found(),
    'lcms'            : lcms.found(),
    'libnsgif'        : libnsgif.found(),
    'libraw'          : libraw.found(),
    'Luajit'          : lua.found(),
    'maxflow'         : maxflow.found(),
    'mrg'             : mrg.found(),
    'Pango'           : pango.found(),
    'pangocairo'      : pangocairo.found(),
    'PNG'             : libpng.found(),
    'poly2tri-c'      : poly2tri_c.found(),
    'poppler'         : poppler.found(),
    'OpenEXR'         : openexr.found(),
    'rsvg'            : librsvg.found(),
    'SDL1'            : sdl1.found(),
    'SDL2'            : sdl2.found(),
    'spiro'           : libspiro.found(),
    'TIFF'            : libtiff.found(),
    'umfpack'         : libumfpack.found(),
    'V4L'             : libv4l1.found(),
    'V4L2'            : libv4l2.found(),
    'webp'            : libwebp.found(),
  }, section: 'Optional dependencies'
)
