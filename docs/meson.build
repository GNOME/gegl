
if build_reference
  subdir('reference')
endif
if not build_docs
  subdir_done()
endif

# images
images = files(
  'images' / 'GEGL.png',
  'images' / 'GEGL-250.png',
  'images' / 'gaussian-blur-graph.png',
)

# stylesheets
gegl_css = files('gegl.css')
stylesheets = files(
  'devhelp.css'
)

# website main page - one file only
website_index = files(
  'index.adoc'
)

# website content
website_content = files(
  'abyss_policy.adoc',
  'commandline.adoc',
  'copyright.adoc',
  'development.adoc',
  'environment.adoc',
  'features.adoc',
  'gegl-chain.adoc',
  'glossary.adoc',
  'hello-world.adoc',
  'operation-api.adoc',
  'release-notes.adoc',
  'source-overview.adoc',
)
website_content += [
  configure_file(
    input : 'build.adoc.in',
    output: '@BASENAME@',
    configuration: dep_ver,
  ),
]

# Website source listings
src_gegl_operation = project_source_root / 'gegl' / 'operation'
website_listing_files = files(
  src_gegl_operation / 'gegl-operation.h',
  src_gegl_operation / 'gegl-operation-composer.h',
  src_gegl_operation / 'gegl-operation-area-filter.h',
  src_gegl_operation / 'gegl-operation-filter.h',
  src_gegl_operation / 'gegl-operation-meta.h',
  src_gegl_operation / 'gegl-operation-point-composer.h',
  src_gegl_operation / 'gegl-operation-temporal.h',
  src_gegl_operation / 'gegl-operation-point-filter.h',
  src_gegl_operation / 'gegl-operation-point-render.h',
  src_gegl_operation / 'gegl-operation-sink.h',
  src_gegl_operation / 'gegl-operation-source.h',
  project_source_root / 'operations' / 'common' /
   'brightness-contrast.c',
)

# Standalone content
standalone_content = files(
  'NEWS.adoc',
  'contribute.adoc',
  'editor.adoc',
  'journal.adoc',
)

website_index_deps = []
website_content_deps = []
website_listing_deps = []
standalone_content_deps = []

# Documentation build environment
doc_env = [
  'GEGL_SWAP=' + project_build_root / 'RAM',
  'GEGL_PATH=' + gegl_operations_build_dir,
  'BABL_TOLERANCE=0.0',
]
if is_variable('babl_path')
  doc_env += 'BABL_PATH=' + babl_path
endif

# website include files
if can_run_host_binaries
  gegl_usage = custom_target(
    'gegl_usage_txt',
    output: 'gegl-usage.txt',
    command: [
      find_program('gegl-usage.py'),
      '--output', '@OUTPUT@',
      '--gegl-exe', gegl_bin,
    ]
  )
  website_content_deps += gegl_usage
endif

# Inheritance diagram
inheritance_txt = custom_target(
  'inheritance_txt',
  output: 'inheritance.txt',
  command: [
    find_program(
      project_source_root / 'tools' / 'gobj2dot.py', native: true
    ),
    '--output', '@OUTPUT@',
    project_source_root
  ],
)

# build stand-alone asciidoc files - not part of website
_tgt = []
foreach _file: standalone_content
  _tgt += custom_target(
    '@0@.html'.format(_file).underscorify(),
    output: '@BASENAME@.html',
    input: _file,
    command: [
      asciidoc,
      '--unsafe',
      '-o', '@OUTPUT@',
      '-a', 'stylesheet=@0@'.format(
        meson.current_source_dir() / 'gegl.css'
      ),
      '-a', 'quirks!',
      '@INPUT@',
    ],
    depends: standalone_content_deps,
    build_by_default : true,
  )
endforeach
news_html = _tgt[0]


subdir('website')
