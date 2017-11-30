project('gegl', ['c', 'cpp'],
  license: 'GPL3+',
  version: '0.3.23',
  meson_version: '>=0.40.0',
  default_options: [
    'sysconfdir=/etc',
    'localstatedir=/var',
    'sharedstatedir=/var/lib'
  ],
  # http://bugzilla.gnome.org/enter_bug.cgi?product=GEGL
)

# Making releases on the stable branch:
#   GEGL_MICRO_VERSION += 1;
#   GEGL_INTERFACE_AGE += 1;
#   GEGL_BINARY_AGE += 1;
# if any functions have been added,
#    set GEGL_INTERFACE_AGE to 0.
# if backwards compatibility has been broken,
#    set GEGL_BINARY_AGE _and_ GEGL_INTERFACE_AGE to 0.

pkgconfig = import('pkgconfig')
i18n      = import('i18n')
gnome     = import('gnome')
python    = import('python3').find_python()

cc        = meson.get_compiler('c')
prefix    = get_option('prefix')
buildtype = get_option('buildtype')
conf      = configuration_data()

################################################################################
# Projects infos

version = meson.project_version()
array_version = version.split('.')
major_version = array_version[0].to_int()
minor_version = array_version[1].to_int()
micro_version = array_version[2].to_int()
interface_age = 1

binary_age = 100 * minor_version + micro_version
lt_current = binary_age - interface_age

api_version = '@0@.@1@'.format(major_version, minor_version)
so_version  = '@0@.@1@.@2@'.format(0, lt_current, interface_age)
lib_name    = meson.project_name() + '-' + api_version
gettext_package = lib_name

stability_version_number = (major_version != 0 ? minor_version : micro_version)
stable = (stability_version_number % 2 == 0)

conf.set10('GEGL_UNSTABLE', not stable)

conf.set       ('GEGL_MAJOR_VERSION',     '@0@'.format(major_version))
conf.set       ('GEGL_MINOR_VERSION',     '@0@'.format(minor_version))
conf.set       ('GEGL_MICRO_VERSION',     '@0@'.format(micro_version))
conf.set_quoted('GEGL_LIBRARY',           '@0@'.format(lib_name))
conf.set_quoted('GETTEXT_PACKAGE',        '@0@'.format(gettext_package))

################################################################################
# Host system detection

host_os = host_machine.system()
message('Host os: ' + host_os)

os_win32  = host_os.contains('mingw')
os_android= host_os.contains('android')
os_osx    = host_os.contains('darwin')

if os_osx and cc.get_id() != 'clang'
  error('You should use Clang on OSx.')
endif


host_cpu = host_machine.cpu()
message('Architecture: ' + host_cpu)

if   host_cpu.startswith('i') and host_cpu.endswith('86')
  have_x86 = true
  conf.set10('ARCH_X86',    true)
elif host_cpu == 'x86_64'
  have_x86 = true
  conf.set10('ARCH_X86',    true)
  conf.set10('ARCH_X86_64', true)
elif host_cpu == 'ppc' or host_cpu == 'powerpc'
  have_ppc = true
  conf.set10('ARCH_PPC',    true)
elif host_cpu == 'ppc64' or host_cpu == 'powerpc64'
  have_ppc = true
  conf.set10('ARCH_PPC',    true)
  conf.set10('ARCH_PPC64',  true)
else
  error('Unknown host architecture')
endif


################################################################################
# Extra warnings

extra_warnings_list = [
  '-Wdeclaration-after-statement',
  '-Winit-self',
  '-Wmissing-declarations',
  '-Wmissing-prototypes',
  '-Wold-style-definition',
  '-Wpointer-arith',
]
extra_warnings = []
foreach warning : extra_warnings_list
  if cc.has_argument(warning)
    extra_warnings += warning
  endif
endforeach


################################################################################
# Check for compiler CPU extensions

if cc.has_argument     ('-mfpmath=sse')
  add_project_arguments('-mfpmath=sse', language: 'c')
endif

if cc.has_argument     ('-mmmx') and get_option('enable-mmx')
  add_project_arguments('-mmmx', language: 'c')
endif

if cc.has_argument     ('-msse') and get_option('enable-sse')
  add_project_arguments('-msse', language: 'c')
endif

if cc.has_argument     ('-msse2') and get_option('enable-sse2')
  add_project_arguments('-msse2', language: 'c')
endif

if cc.has_argument     ('-msse4.1') and get_option('enable-sse4_1')
  add_project_arguments('-msse4.1', language: 'c')
endif

if cc.has_argument     ('-mf16c') and get_option('enable-f16c')
  add_project_arguments('-mf16c', language: 'c')
endif

conf.set('HAVE_UNISTD_H', cc.has_header('unistd.h'))


################################################################################
# Utilities

env         = find_program('env')
bash        = find_program('bash')

asciidoc    = find_program('asciidoc',      required: false)
enscript    = find_program('enscript',      required: false)
perl        = find_program('perl5', 'perl', required: false)
python2     = find_program('python2',       required: false)
rsvg_convert= find_program('rsvg-convert',  required: false)
ruby        = find_program('ruby',          required: false)
w3m         = find_program('w3m',           required: false)

################################################################################
# Dependencies

math  = cc.find_library('m',       required: false)
thread= cc.find_library('pthread', required: false)
log   = cc.find_library('log',     required: false)
dl    = cc.find_library('dl',      required: false)

babl          = dependency('babl',            version: '>=0.1.38')

glib          = dependency('glib-2.0',        version: '>=2.44.0')
gobject       = dependency('gobject-2.0',     version: '>=2.44.0')
gmodule       = dependency('gmodule-2.0',     version: '>=2.44.0')
gthread       = dependency('gthread-2.0',     version: '>=2.44.0')
gio_specific  = os_win32 ? 'gio-windows-2.0' : 'gio-unix-2.0'
gio           = [
                dependency('gio-2.0',         version: '>=2.44.0'),
                dependency(gio_specific,      version: '>=2.44.0'),
              ]
json_glib     = dependency('json-glib-1.0',   version: '>=1.2.6')

if get_option('with-cairo')
  cairo       = dependency('cairo',           version: '>=1.12.2')
endif
if get_option('with-exiv2')
  exiv2       = dependency('exiv2',           version: '>=0.25')
endif
if get_option('with-gdk-pixbuf')
  gdk_pixbuf  = dependency('gdk-pixbuf-2.0',  version: '>=2.32.0')
endif
if get_option('with-gexiv2')
  gexiv2      = dependency('gexiv2',          version: '>=0.0.0')
endif
conf.set10('HAVE_GEXIV2', get_option('with-gexiv2'))
if get_option('with-graphviz')
  dot         = find_program('dot')
endif
if get_option('with-jasper')
  jasper      = dependency('jasper',          version: '>=1.900.1')
endif
if get_option('with-lcms')
  lcms        = dependency('lcms2',           version: '>=2.8')
endif
if get_option('with-lensfun')
  lensfun     = dependency('lensfun',         version: '>=0.2.5')
endif
if get_option('with-libav')
  avlibs = [
    dependency('libavcodec',                  version: '>=55.48.100'),
    dependency('libavformat',                 version: '>=1.32.0'),
    dependency('libavutil',                   version: '>=1.32.0'),
    dependency('libswscale',                  version: '>=2.8.0'),
  ]
endif
if get_option('with-libjpeg')
  libjpeg     = dependency('libjpeg',         version: '>=1.0.0')
endif
if get_option('with-libpng')
  libpng      = dependency('libpng',          version: '>=0.2.5')
endif
if get_option('with-libraw')
  libraw      = dependency('libraw',          version: '>=0.15.4')
endif
if get_option('with-librsvg')
  librsvg     = dependency('librsvg-2.0',     version: '>=2.40.6')
endif
if get_option('with-libspiro')
  libspiro    = dependency('libspiro',        version: '>=0.5.0')
endif
if get_option('with-libtiff')
  libtiff     = dependency('libtiff-4',       version: '>=4.0.0')
endif
if get_option('with-libv4l')
  libv4l1     = dependency('libv4l1',         version: '>=1.0.1')
endif
if get_option('with-libv4l2')
  libv4l2     = dependency('libv4l2',         version: '>=1.0.1')
endif
if get_option('with-lua')
  lua         = dependency('lua',             version: '>=5.1.0')
endif
if get_option('with-mrg')
  mrg         = dependency('mrg',             version: '>=0.0.0')
endif
conf.set10('HAVE_MRG', get_option('with-mrg'))
if get_option('with-openexr')
  openexr     = dependency('OpenEXR',         version: '>=1.6.1')
endif
if get_option('with-pango')
  pango       = dependency('pango',           version: '>=1.38.0')
endif
if get_option('with-pangocairo')
  pangocairo  = dependency('pangocairo',      version: '>=1.38.0')
endif
if get_option('with-pygobject')
  pygobject3  = dependency('pygobject-3.0',   version: '>=3.2.0')
  gobj_introsp= dependency('gobject-introspection-1.0', version: '>=1.32.0')
endif
if get_option('with-sdl1')
  sdl1        = dependency('sdl',             version: '>=1.2.0')
endif
have_umfpack = false
have_suitesparse_umfpack = false
if get_option('with-umfpack')
  if cc.has_header('umfpack.h')#, 'UMFPACK_H')
    have_umfpack = true
  elif cc.has_header_symbol('suitesparse/umfpack.h', 'umfpack_dl_solve')
    have_suitesparse_umfpack = true
  else
    error('UmfPack dependency not found.')
  endif
  libumfpack = cc.find_library('umfpack')
endif
conf.set('HAVE_UMFPACK_H', have_umfpack)
conf.set('HAVE_SUITESPARSE_UMFPACK_H', have_suitesparse_umfpack)
if get_option('with-vala')
  vapigen     = dependency('vapigen',         version: '>=0.20.0')
endif
if get_option('with-webp')
  libwebp     = dependency('libwebp',         version: '>=0.5.0')
endif

# Required library eventually provided in libs/ subdir
poly2tri_c  = dependency('poly2tri-c',        version: '>=0.0.0',
  fallback: [ 'poly2tri-c', 'poly2tri_c' ],
)
p2tc_status = 'true (internal)'

conf.set('HAVE_FSYNC',      cc.has_function('fsync'))
conf.set('HAVE_EXECINFO_H', cc.has_header('execinfo.h'))
# HAVE_ALTIVEC_SYSCTL
# HAVE_OPENMP


################################################################################
# Autogenerate files

configure_file(
  output: 'config.h',
  configuration: conf
)

pkgconfig.generate(filebase: 'gegl-' + api_version,
  name: 'GEGL',
  description: 'Generic Graphics Library',
  version: meson.project_version(),
  libraries: [
    '-L${libdir}',
    '-l' + lib_name,
    '-l' + meson.project_name() + '-npd-' + api_version,
  ],
  libraries_private: [
    '-lm', '-lpthread',
  ],
  requires: [
    'babl',
    'gio-2.0',
    'gio-unix-2.0',
    'gmodule-2.0',
    'gobject-2.0',
    'json-glib-1.0',
  ],
  subdirs: [
    lib_name,
  ],
  variables: [
    'pluginsdir=' + join_paths(get_option('libdir'), lib_name),
  ],
)

################################################################################
# Subdirs

rootInclude = include_directories('.')

# version_script = join_paths(meson.current_source_dir(), 'gegl.map')

subdir('libs/rgbe')
subdir('opencl')
subdir('gegl')
subdir('libs/npd')
subdir('seamless-clone')
subdir('bin')
subdir('gcut')
subdir('tools')
subdir('operations')
subdir('examples')
subdir('tests')
subdir('perf')
subdir('po')
if get_option('with-docs')
  subdir('docs')
endif

if w3m.found()
  custom_target('README',
    input : [ join_paths('docs', 'index.html'), ] ,
    output: [ 'README' ] ,
    command: [
      w3m,
      '-cols', '72',
      '-dump',
      '@INPUT@',
    ],
    capture: true,
  )
  custom_target('NEWS',
    input : [ join_paths('docs', 'NEWS.html'), ] ,
    output: [ 'NEWS' ] ,
    command: [
      w3m,
      '-cols', '72',
      '-dump',
      '@INPUT@',
    ],
    capture: true,
  )
endif


message('\n'.join(['',
'Building GEGL with prefix=@0@'.format(get_option('prefix')),
'',
'Optional features:',
'  GEGL docs:       @0@'.format(get_option('with-docs')),
'  Build workshop:  @0@'.format(get_option('with-workshop')),
'  Build website:   @0@'.format(asciidoc.found()),
'  SIMD:            sse:@0@ mmx:@1@ sse2:@2@'
.format(get_option('enable-sse'), get_option('enable-mmx'), get_option('enable-sse2')),
'  Vala support:    @0@'.format(get_option('with-vala')),
'',
'Optional dependencies:',
'  asciidoc:        @0@'.format(asciidoc.found()),
'  enscript:        @0@'.format(enscript.found()),
'  mrg:             @0@'.format(get_option('with-mrg')),
'  Ruby:            @0@'.format(ruby.found()),
'  Lua:             @0@'.format(get_option('with-lua')),
'  Cairo:           @0@'.format(get_option('with-cairo')),
'  Pango:           @0@'.format(get_option('with-pango')),
'  pangocairo:      @0@'.format(get_option('with-pangocairo')),
'  GDKPixbuf:       @0@'.format(get_option('with-gdk-pixbuf')),
'  JPEG:            @0@'.format(get_option('with-libjpeg')),
'  PNG:             @0@'.format(get_option('with-libpng')),
'  OpenEXR:         @0@'.format(get_option('with-openexr')),
'  rsvg:            @0@'.format(get_option('with-librsvg')),
'  SDL:             @0@'.format(get_option('with-sdl1')),
'  libraw:          @0@'.format(get_option('with-libraw')),
'  Jasper:          @0@'.format(get_option('with-jasper')),
'  graphviz:        @0@'.format(get_option('with-graphviz')),
'  av libs:         @0@'.format(get_option('with-libav')),
'  V4L:             @0@'.format(get_option('with-libv4l')),
'  V4L2:            @0@'.format(get_option('with-libv4l2')),
'  spiro:           @0@'.format(get_option('with-libspiro')),
'  EXIV:            @0@'.format(get_option('with-exiv2')),
'  gexiv2:          @0@'.format(get_option('with-gexiv2')),
'  umfpack:         @0@'.format(get_option('with-umfpack')),
'  TIFF             @0@'.format(get_option('with-libtiff')),
'  webp:            @0@'.format(get_option('with-webp')),
'  poly2tri-c:      @0@'.format(p2tc_status),

'']))
