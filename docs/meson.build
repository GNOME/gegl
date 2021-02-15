
if build_reference
  subdir('reference')
endif
if not build_docs
  subdir_done()
endif

# website
doc_env = [
  'GEGL_SWAP=' + meson.current_build_dir() / 'RAM',
  'GEGL_PATH=' + gegl_operations_build_dir,
  'BABL_TOLERANCE=0.0',
]
if is_variable('babl_path')
  doc_env += 'BABL_PATH=' + babl_path
endif

# images
images = files(
  'images' / 'GEGL.png',
  'images' / 'GEGL-250.png',
  'images' / 'gaussian-blur-graph.png',
)

# stylesheets
stylesheets = files(
  'gegl.css',
  'devhelp.css'
)
configure_file(
  input : files('gegl.css'),
  output : '@PLAINNAME@',
  copy: true,
)

# asciidoc html files
asciidoc_config = configuration_data()
asciidoc_config.set('top_srcdir', project_source_root)

asciidoc_files = files(
  'NEWS.adoc',
  'abyss_policy.adoc',
  'contribute.adoc',
  'editor.adoc',
  'environment.adoc',
  'journal.adoc',
)
asciidoc_files += [
  configure_file(
    input : 'hello-world.adoc.in',
    output: '@BASENAME@',
    configuration: asciidoc_config,
  ),
]

# asciidoc html files
_tgt = []
foreach _file: asciidoc_files
  _tgt += custom_target(
    '@0@.html'.format(_file).underscorify(),
    output: '@BASENAME@.html',
    input: _file,
    command: [
      asciidoc,
      '--unsafe',
      '-o', '@OUTPUT@',
      '-a', 'stylesheet=@0@'.format(
        meson.current_build_dir() / 'gegl.css'
      ),
      '-a', 'highlight=@0@'.format(source_highlight.found()),
      '-a', 'quirks!',
      '@INPUT@',
    ],
    build_by_default : true,
  )
endforeach
news_html = _tgt[0]

# website html files
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
endif

website_asciidoc_files = files(
  'index.adoc',
  'commandline.adoc',
  'development.adoc',
  'features.adoc',
  'gegl-chain.adoc',
  'glossary.adoc',
  'operation-api.adoc',
  'source-overview.adoc',
)
website_asciidoc_files += [
  configure_file(
    input : 'build.adoc.in',
    output: '@BASENAME@',
    configuration: dep_ver,
  ),
]

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


subdir('website')
