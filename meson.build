project('gegl',
  'c', 'cpp',
  license: 'GPL3+',
  version: '0.4.65',
  meson_version: '>=0.59.0',
  default_options: [
    'c_std=gnu11',
    'cpp_std=c++14',
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

fs        = import('fs')
pkgconfig = import('pkgconfig')
i18n      = import('i18n')
gnome     = import('gnome')
python    = import('python').find_installation()

cc        = meson.get_compiler('c')
cpp       = meson.get_compiler('cpp')
buildtype = get_option('buildtype')

gegl_prefix     = get_option('prefix')
gegl_gtk_docdir = gnome.gtkdoc_html_dir('gegl')
gegl_libdir     = get_option('libdir')

project_build_root = meson.current_build_dir()
project_source_root = meson.current_source_dir()


################################################################################
# Dependency versions
#
# Core - required
dep_ver = {
  'babl'            : '>=0.1.116',
  'glib'            : '>=2.44.0',
  'json-glib'       : '>=1.0.0',
  'poly2tri-c'      : '>=0.0.0',
  'libnsgif'        : '>=1.0.0',
}

# Core - optional
dep_ver += {
  'g-ir'            : '>=1.32.0',
  'vapigen'         : '>=0.20.0',
}

# GEGL binary - required
dep_ver += {
  'libpng'          : '>=1.6.0',
}

# GEGL binary - optional
dep_ver += {
  'gexiv2'          : '>=0.14.0',
  'luajit'          : '>=2.0.4',
  'mrg'             : '>=0.0.0',
}

# Operations - required
dep_ver += {
  'libjpeg'         : '>=1.0.0',
}

# Operations - optional
dep_ver += {
  'gdk-pixbuf'      : '>=2.32.0',
  'cairo'           : '>=1.12.2',
  'pango'           : '>=1.38.0',
  'pangocairo'      : '>=1.38.0',
  'jasper'          : '>=1.900.1',
  'lcms'            : '>=2.8',
  'lensfun'         : '>=0.2.5',
  'libraw'          : '>=0.15.4',
  'librsvg'         : '>=2.40.6',
  'libspiro'        : '>=0.5.0',
  'libtiff'         : '>=4.0.0',
  'libv4l1'         : '>=1.0.1',
  'libv4l2'         : '>=1.0.1',
  'libwebp'         : '>=0.5.0',
  'maxflow'         : '>=3.0.4',
  'openexr'         : '>=1.6.1',
  'poppler'         : '>=0.71.0',
  'sdl1'            : '>=1.2.0',
  'sdl2'            : '>=2.0.5',
  'libavcodec'      : '>=55.69.100',
  'libavformat'     : '>=55.48.100',
  'libavutil'       : '>=55.92.100',
  'libswscale'      : '>=2.6.100',
}

# Tests (runtime) - optional
dep_ver += {
  'pygobject'       : '>=3.2.0',
}




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

host_cpu_family = host_machine.cpu_family()
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
elif host_cpu_family == 'arm'
  have_arm = true
  config.set10('ARCH_ARM',    true)
elif host_cpu_family == 'aarch64'
  have_aarch64 = true
  config.set10('ARCH_AARCH64',    true)
endif

# Only try to run compiled programs if native compile or cross-compile
# and have exe wrapper. If we don't need a wrapper (e.g. 32 bit build in
# 64-bit environment) then set proprty has_exe_wrapper=true in cross
# file
can_run_host_binaries = meson.can_run_host_binaries()


################################################################################
# Compiler arguments

cflags_common = []
cflags_c      = []
cflags_cpp    = []
lflags_common = []

cflags_common += '-DHAVE_CONFIG_H'


cflags_common += [
  '-Winit-self',
  '-Wmissing-declarations',
  '-Wpointer-arith',
  '-Wno-unused-parameter',
  '-Wno-cast-function-type',
  '-Wno-implicit-function-declaration'
]
cflags_c = [
  '-Wmissing-prototypes',
  '-Wold-style-definition',
]


# DEBUG SYMBOLS
debugging_format = 'Disabled'
if buildtype == 'debug' or buildtype == 'debugoptimized'
  cflags_common += '-DGEGL_ENABLE_DEBUG'

  if not os_win32
    # DWARF symbols for GCC and LLDB on Unix
    debugging_format = 'Native'
  elif cc.get_argument_syntax() == 'msvc'
    # CodeView symbols for DIA or DbgHelp debuggers and LLDB on Windows
    debugging_format = 'Native'
  elif os_win32 and get_option('win-debugging') == 'native' and cc.has_argument('-gcodeview') and cc.has_link_argument('-Wl,--pdb=') and cc.get_id() == 'clang'
    # CodeView symbols for DIA or DbgHelp debuggers and LLDB on Windows
    debugging_format = 'Native'
    cflags_common += '-gcodeview'
    lflags_common += '-Wl,--pdb='
  else
    # DWARF symbols for GCC and LLDB on Windows (fallback)
    debugging_format = 'DWARF'
    if cc.get_id() == 'clang'
      # Optimize DWARF symbols to Dr. Mingw
      # https://github.com/jrfonseca/drmingw/issues/42
      cflags_common += '-gdwarf-aranges'
    endif
  endif
endif


if os_win32
  cflags_common += '-D_FILE_OFFSET_BITS=64'

  if cc.get_id() == 'clang'
    # Workaround to get colored output
    # https://github.com/msys2/MINGW-packages/issues/2988
    cflags_common += '-fansi-escape-codes'
  endif

  if cc.get_argument_syntax() == 'msvc'
    #Needed otherwise MSVC get angry with the headers provided by UCRT
    cflags_common += ['/D_USE_MATH_DEFINES']
  endif
endif #if os_win32

if buildtype == 'debugoptimized' or buildtype == 'release'
  cflags_common += cc.get_supported_arguments(['-ftree-vectorize'])
endif

if host_cpu_family == 'x86_64'
  x86_64_v2_flags = cc.get_supported_arguments(['-march=x86-64','-msse2', '-msse2','-msse4.1','-msse4.2','-mpopcnt','-mssse3'])
  x86_64_v3_flags = x86_64_v2_flags + cc.get_supported_arguments(['-mavx','-mavx2','-mf16c','-mfma','-mmovbe', '-mbmi', '-mbmi2'])

  x86_64_v2_flags += ['-DSIMD_X86_64_V2']
  x86_64_v3_flags += ['-DSIMD_X86_64_V3']

elif host_cpu_family == 'arm'
  arm_neon_flags = cc.get_supported_arguments(['-mfpu=neon-vfpv4'])
  arm_neon_flags += ['-DSIMD_ARM_NEON']
elif host_cpu_family == 'aarch64'
  cflags_common += cc.get_supported_arguments(['-mfpu=neon-fp-armv8', '-ftree-vectorize'])
endif

cflags_c   = cflags_common + cflags_c
cflags_cpp = cflags_common + cflags_cpp

add_project_arguments(cc.get_supported_arguments(cflags_c), language: 'c')
add_project_arguments(cpp.get_supported_arguments(cflags_cpp), language: 'cpp')
add_project_link_arguments(lflags_common, language: [ 'c', 'cpp' ])

# on OSX ObjC and C sources are mixed so adding objc to the linkflags

if os_osx
  add_languages('objc')
  osx_ldflags = ['-Wl,-framework,Foundation', '-Wl,-framework,AppKit']
  add_project_link_arguments(osx_ldflags, language : ['objc', 'c'])
  add_project_arguments('-xobjective-c', language : 'c')
endif

################################################################################
# Build Utilities

asciidoc          = find_program('asciidoc',
                     required: false, native: true)
dot               = find_program('dot',
                     required: false, native: true)
gtkdoc_scan       = find_program('gtkdoc-scan',
                     required: get_option('gtk-doc'), native: true)
source_highlight  = find_program('source-highlight',
                     required: false, native: true)
w3m               = find_program('w3m',
                     required: false, native: true)

################################################################################
# Required Dependencies

config.set('HAVE_UNISTD_H',    cc.has_header('unistd.h'))
config.set('HAVE_EXECINFO_H',  cc.has_header('execinfo.h'))
config.set('HAVE_FSYNC',       cc.has_function('fsync'))
config.set('HAVE_MALLOC_TRIM', cc.has_function('malloc_trim'))
config.set('HAVE_STRPTIME',    cc.has_function('strptime'))

math    = cc.find_library('m',  required: false)
libdl   = cc.find_library('dl', required : false)
thread  = dependency('threads')

babl      = dependency('babl-0.1',    version: dep_ver.get('babl'), required: false)
if not babl.found()
  # babl changed its pkg-config name from 'babl' to 'babl-0.1' in version
  # 0.1.100 (0.1.99 dev cycle more exactly). 'babl-0.1' is checked in priority
  # because it would be a newer version.
  babl    = dependency('babl',        version: dep_ver.get('babl'), required: false)

  if not babl.found()
    # OK this is ugly as hell and you might wonder why we need to do this
    # tri-test, once with the new name, then with the old name, and finally
    # again with the new name. This is because when using babl as a subproject,
    # it seems that meson is only looking for the required dep. So without this
    # third redundant test, it searches for 'babl' which it doesn't find since
    # babl's meson code ran meson.override_dependency('babl-0.1', ...).
    # This is why we need to make the latest name search first AND last.
    #
    # Note that when we will bump meson requirement to >= 0.60, we will be able
    # to simply write both names as a list in dependency(), which should
    # simplify the whole tri-test meson code into one single call.
    babl  = dependency('babl-0.1',    version: dep_ver.get('babl'))
  endif
endif
glib      = dependency('glib-2.0',    version: dep_ver.get('glib'))
gobject   = dependency('gobject-2.0', version: dep_ver.get('glib'))
gmodule   = dependency('gmodule-2.0', version: dep_ver.get('glib'))
gthread   = dependency('gthread-2.0', version: dep_ver.get('glib'))
gio_os    = os_win32 ? 'gio-windows-2.0' : 'gio-unix-2.0'
gio       = [
            dependency('gio-2.0',     version: dep_ver.get('glib')),
            dependency(gio_os,        version: dep_ver.get('glib')),
]
json_glib = dependency('json-glib-1.0',
  version: dep_ver.get('json-glib')
)
libjpeg   = dependency('libjpeg',     version: dep_ver.get('libjpeg'))
libpng    = dependency('libpng',      version: dep_ver.get('libpng'))

# Required libraries eventually provided in subprojects/ subdir
poly2tri_c = dependency('poly2tri-c',
  version:  dep_ver.get('poly2tri-c'),
  fallback: ['poly2tri-c', 'poly2tri_c'],
  required: false,
)

libnsgif = dependency('libnsgif',     version: dep_ver.get('libnsgif'),
  fallback: ['libnsgif', 'libnsgif'],
)

################################################################################
# Optionnal Dependencies

# GEGL library
if get_option('introspection') != 'false'
  g_ir = dependency('gobject-introspection-1.0',
    version:  dep_ver.get('g-ir'),
    required: get_option('introspection') == 'true' ? true : false,
  )
else
  g_ir = disabler()
endif
if g_ir.found()
  vapigen   = dependency('vapigen',
    version: dep_ver.get('vapigen'),
    required: get_option('vapigen')
  )
else
  vapigen = disabler()
endif

# GEGL binary
gexiv2    = dependency('gexiv2',
  version: dep_ver.get('gexiv2'),
  required: get_option('gexiv2')
)
config.set('HAVE_GEXIV2', gexiv2.found())
lua       = dependency('luajit',
  version: dep_ver.get('luajit'),
  required: get_option('lua')
)
config.set('HAVE_LUA', lua.found())
mrg       = dependency('mrg',
  version: dep_ver.get('mrg'),
  required: get_option('mrg')
)
config.set('HAVE_MRG', mrg.found())

# Operations
gdk_pixbuf= dependency('gdk-pixbuf-2.0',
  version: dep_ver.get('gdk-pixbuf'),
  required: get_option('gdk-pixbuf')
)
cairo     = dependency('cairo',
  version: dep_ver.get('cairo'),
  required: get_option('cairo')
)
pango     = dependency('pango',
  version: dep_ver.get('pango'),
  required: get_option('pango')
)
pangocairo = dependency('pangocairo',
  version: dep_ver.get('pangocairo'),
  required: get_option('pangocairo')
)

jasper    = dependency('jasper',
  version: dep_ver.get('jasper'),
  required: get_option('jasper')
)
lcms      = dependency('lcms2',
  version: dep_ver.get('lcms'),
  required: get_option('lcms')
)
lensfun   = dependency('lensfun',
  version: dep_ver.get('lensfun'),
  required: get_option('lensfun')
)
libraw    = dependency('libraw',
  version: dep_ver.get('libraw'),
  required: get_option('libraw')
)
librsvg   = dependency('librsvg-2.0',
  version: dep_ver.get('librsvg'),
  required: get_option('librsvg')
)
libspiro  = dependency('libspiro',
  version: dep_ver.get('libspiro'),
  required: get_option('libspiro')
)
libtiff   = dependency('libtiff-4',
  version: dep_ver.get('libtiff'),
  required: get_option('libtiff')
)
libv4l1   = dependency('libv4l1',
  version: dep_ver.get('libv4l1'),
  required: get_option('libv4l')
)
libv4l2   = dependency('libv4l2',
  version: dep_ver.get('libv4l2'),
  required: get_option('libv4l2')
)
libwebp   = dependency('libwebp',
  version: dep_ver.get('libwebp'),
  required: get_option('webp')
)
maxflow   = dependency('maxflow',
  version: dep_ver.get('maxflow'),
  required: get_option('maxflow')
)
openexr   = dependency('OpenEXR',
  version: dep_ver.get('openexr'),
  required: get_option('openexr')
)
openmp    = dependency('openmp',
  required: get_option('openmp')
)
config.set('HAVE_OPENMP', openmp.found())
poppler = dependency('poppler-glib',
  version: dep_ver.get('poppler'),
  required: get_option('poppler')
)
sdl1      = dependency('sdl',
  version: dep_ver.get('sdl1'),
  required: get_option('sdl1')
)
sdl2      = dependency('sdl2',
  version: dep_ver.get('sdl2'),
  required: get_option('sdl2')
)

# AV libs
libavcodec = dependency('libavcodec',
  version: dep_ver.get('libavcodec'),
  required: get_option('libav')
)
libavformat = dependency('libavformat',
  version: dep_ver.get('libavformat'),
  required: get_option('libav')
)
libavutil = dependency('libavutil',
  version: dep_ver.get('libavutil'),
  required: get_option('libav')
)
libswscale = dependency('libswscale',
  version: dep_ver.get('libswscale'),
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
  have_umfpack = cc.has_header_symbol(
    'umfpack.h','umfpack_dl_solve')
  have_ss_umfpack = cc.has_header_symbol(
    'suitesparse/umfpack.h', 'umfpack_dl_solve')

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
  pygobject3 = dependency('pygobject-3.0',
    version: dep_ver.get('pygobject'),
    required: get_option('pygobject')
  )
else
  pygobject3 = disabler()
endif

################################################################################
# Build flags

# Docs
build_docs = true
if get_option('docs') == 'auto'
  if meson.is_cross_build()
    build_docs = false
    message('configure with -Ddocs=true to cross-build docs')
  elif not asciidoc.found()
    build_docs = false
  endif
elif get_option('docs') == 'false'
  build_docs = false
elif not asciidoc.found()
  build_docs = false
  warning('asciidoc is required to build website')
endif

gi_docgen = find_program('gi-docgen', required: get_option('gi-docgen'))
if get_option('gi-docgen').auto()
 build_gi_docgen = g_ir.found() and gi_docgen.found() and not meson.is_cross_build()
else
 build_gi_docgen = get_option('gi-docgen').enabled()

 if build_gi_docgen and not g_ir.found()
   warning('-Ddocs=true and -Dintrospection=true are required to build gi-docgen documentation')
   build_gi_docgen = false
 endif
endif

build_reference = get_option('gtk-doc') or build_gi_docgen

#########################################################################
# Install native debug data (.pdb) on Windows
# Ideally meson should take care of it automatically.
# See: https://github.com/mesonbuild/meson/issues/12977
if os_win32 and debugging_format == 'Native' and cc.get_argument_syntax() != 'msvc'
  install_win_debug_script = find_program('./meson_install_win_debug.py')
  meson.add_install_script(install_win_debug_script)
endif

#########################################################################
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
subdir('libs/ctx')

# pkg-config file
gegl_pub_deps = [
  glib,
  gobject,
  gmodule,
  gio,
  json_glib,
  babl,
]

pkgconfig.generate(gegl_lib,
  filebase: 'gegl-' + api_version,
  name: 'GEGL',
  description: 'Generic Graphics Library',
  version: meson.project_version(),
  variables: 'pluginsdir=' + '${prefix}' / get_option('libdir') / api_name,
  # gegl_npd_lib API is actually used by software, such as GIMP. It must
  # not be removed, even though adding it like this is not so proper. If
  # one wants to clean up this dependency, a proper port must be done
  # first so that using application still build and don't lose features.
  libraries: gegl_pub_deps + [ gegl_npd_lib ],
  subdirs: api_name,
)

gegl_dep = declare_dependency(link_with: [gegl_lib, gegl_ctx_lib],
  include_directories: geglInclude,
  dependencies: [gegl_pub_deps, libnpd],
  sources: g_ir.found() ? gegl_gir : [],
)

if meson.version().version_compare('>=0.54.0')
  meson.override_dependency('gegl-' + api_version, gegl_dep)
endif

subdir('seamless-clone')
subdir('bin')
subdir('tools')
subdir('operations')
subdir('examples')
subdir('tests')
subdir('perf')
subdir('po')
subdir('docs')


if w3m.found()
  # Create NEWS file from html file
  if is_variable('news_html')
    custom_target('NEWS',
      input: news_html,
      output: 'NEWS',
      command: [
        w3m,
        '-cols', '72',
        '-dump',
        '@INPUT@',
      ],
      capture: true,
      build_by_default: true
    )
  endif

  if is_variable('index_html')
    # Create README file from html file
    custom_target('README',
      input: index_html,
      output: 'README',
      command: [
        w3m,
        '-cols', '72',
        '-dump',
        '@INPUT@',
      ],
      capture: true,
      build_by_default: true
    )
  endif
endif

################################################################################
# Build summary

summary(
  {
    'prefix'        : gegl_prefix,
    'libdir'        : gegl_libdir,
  }, section: 'Directories'
)
summary(
  {
    'GTK-docs reference'  : get_option('gtk-doc'),
    'GI-docgen reference' : build_gi_docgen,
    'Docs (GEGL website)' : build_docs,
  }, section: 'GEGL docs'
)
summary(
  {
    'Build workshop'       : get_option('workshop'),
    'Introspection'        : g_ir.found(),
    'Vala support'         : vapigen.found(),
    'Debug symbols format' : debugging_format,
  }, section: 'Optional features'
)
summary(
  {
    'asciidoc'          : asciidoc.found(),
    'dot'               : dot.found(),
    'pygobject'         : pygobject3.found(),
    'source-highlight'  : source_highlight.found(),
    'w3m'               : w3m.found(),
  }, section: 'Optional build utilities'
)
summary(
  {
    'avlibs'            : avlibs_found,
    'Cairo'             : cairo.found(),
    'GDKPixbuf'         : gdk_pixbuf.found(),
    'gexiv2'            : gexiv2.found(),
    'Jasper'            : jasper.found(),
    'lcms'              : lcms.found(),
    'libnsgif'          : libnsgif.found(),
    'libraw'            : libraw.found(),
    'Luajit'            : lua.found(),
    'maxflow'           : maxflow.found(),
    'mrg'               : mrg.found(),
    'OpenEXR'           : openexr.found(),
    'openMP'            : openmp.found(),
    'Pango'             : pango.found(),
    'pangocairo'        : pangocairo.found(),
    'poly2tri-c'        : poly2tri_c.found(),
    'poppler'           : poppler.found(),
    'rsvg'              : librsvg.found(),
    'SDL1'              : sdl1.found(),
    'SDL2'              : sdl2.found(),
    'spiro'             : libspiro.found(),
    'TIFF'              : libtiff.found(),
    'umfpack'           : libumfpack.found(),
    'V4L'               : libv4l1.found(),
    'V4L2'              : libv4l2.found(),
    'webp'              : libwebp.found(),
  }, section: 'Optional dependencies'
)
