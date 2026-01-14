
seamlessInclude = include_directories('.')

seamlessclone_sources = [
  'sc-context.c',
  'sc-outline.c',
  'sc-sample.c',
]

seamlessclone_headers = [
  'sc-common.h',
  'sc-context.h',
  'sc-outline.h',
  'sc-sample.h',
]

gegl_sc_def_conf = {}
if poly2tri_c.type_name() == 'internal'
  gegl_sc_def_conf += { 'p2t_exports' : fs.read('gegl-p2t.def') }
else
  gegl_sc_def_conf += { 'p2t_exports' : '' }
endif

#Needed otherwise defcheck.py fails
gegl_sc_def = configure_file(
  input: 'gegl-sc.def',
  output: 'gegl-sc.def',
  configuration: gegl_sc_def_conf
)

seamlessclone_lib = library('gegl-sc-' + api_version,
  seamlessclone_sources,
  include_directories: [ rootInclude, geglInclude, ],
  dependencies: [
    gegl_dep,
    math,
    poly2tri_c,
  ],
  vs_module_defs: gegl_sc_def,
  install: true,
)

install_headers(
  seamlessclone_headers,
  subdir: api_name / 'sc'
)

pkgconfig.generate(seamlessclone_lib,
  filebase: 'gegl-sc-' + api_version,
  name: 'GEGL Seamless Cloning',
  description: 'Seamless Cloning Library Based on GEGL',
  version: meson.project_version(),
  libraries: gegl_dep,
  subdirs: api_name / 'sc',
)

gegl_sc_dep = declare_dependency(link_with: seamlessclone_lib,
  include_directories: seamlessInclude,
  dependencies: [gegl_dep],
)

if meson.version().version_compare('>=0.54.0')
  meson.override_dependency('gegl-sc-' + api_version, gegl_sc_dep)
endif
