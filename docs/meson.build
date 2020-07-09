
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
  'images' / 'gaussian-blur-graph.png',
)

# stylesheets
stylesheets = files(
  'gegl.css',
  'devhelp.css',
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
  'NEWS.txt',
  'abyss_policy.txt',
  'contribute.txt',
  'editor.txt',
  'environment.txt',
  'journal.txt',
  'source-overview.txt',
)
asciidoc_files += [
  configure_file(
    input : 'hello-world.txt.in',
    output: 'hello-world.txt',
    configuration: asciidoc_config,
  ),
]

# asciidoc html files
if asciidoc.found()
  _tgt = []
  foreach _file: asciidoc_files
    _tgt += custom_target('@0@.html'.format(_file).underscorify(),
      output: '@BASENAME@.html',
      input: _file,
      command: [
        asciidoc,
        '--unsafe',
        '-o', '@OUTPUT@',
        '-a', 'stylesheet=@0@'.format(
            project_source_root / 'docs' / 'gegl.css'
          ),
        '-a', 'quirks!',
        '@INPUT@',
      ],
      build_by_default : true,
    )
  endforeach
  news_html = _tgt[0]
else
  error('asciidoc is required to build docs')
endif

# website html files
website_asciidoc_files = [
  configure_file(
    input : 'index.txt.in',
    output: 'index.txt',
    configuration: asciidoc_config,
  ),
  configure_file(
    input : 'copyright.txt.in',
    output: 'copyright.txt',
    configuration: asciidoc_config,
  ),
]
website_asciidoc_files += files(
  'build.txt',
  'commandline.txt',
  'development.txt',
  'features.txt',
  'gegl-chain.txt',
  'glossary.txt',
  'operation-api.txt',
)

# Inheritance diagram
if ruby.found()
  gobj2dot = find_program(
    project_source_root / 'tools' / 'gobj2dot.rb',
    native: true,
    required: true,
  )
  inheritance_txt = custom_target(
    'inheritance_txt',
    output: 'inheritance.txt',
    command: [gobj2dot, project_source_root],
    capture: true,
  )
endif

subdir('website')
