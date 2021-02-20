
subdir('images')
subdir('operations')

# stylesheets
gegl_css = project_source_root / 'docs' / 'gegl.css'

foreach _file : stylesheets
  configure_file(
    input : _file,
    output : '@PLAINNAME@',
    copy: true,
  )
endforeach

# generate website
# website - class hierarchy reference
if env.found() and can_run_host_binaries
  custom_target(
    'class_hierarchy_html',
    output: 'class-hierarchy.html',
    command: [
      env,
      doc_env,
      introspect,
    ],
    depends: gegl_operations,
    capture: true,
    build_by_default : true,
  )
else
  warning('Cannot create class reference in this environment')
endif

# website content
foreach _file: website_content
  custom_target(
    '@0@.html'.format(_file).underscorify(),
    output: '@BASENAME@.html',
    input: _file,
    command: [
      asciidoc,
      '--unsafe',
      '-o', '@OUTPUT@',
      '-a', 'stylesheet=@0@'.format(gegl_css),
      '-a', 'highlight=@0@'.format(source_highlight.found()),
      '-a', 'build_root=@0@'.format(project_build_root),
      '-a', 'source_root=@0@'.format(project_source_root),
      '-a', 'content=@0@'.format('@INPUT@'),
      '-a', 'name=@0@'.format('@BASENAME@'),
      '-a', 'quirks!',
      files('website-content.adoc'),
    ],
    depends: website_content_deps,
    build_by_default : true,
  )
endforeach

# website source listings
foreach _file : website_listing_files
  custom_target(
    '@0@.html'.format(_file).underscorify(),
    input: _file,
    output: '@PLAINNAME@.html',
    command: [
      asciidoc,
      '--unsafe',
      '-o', '@OUTPUT@',
      '-a', 'stylesheet=@0@'.format(gegl_css),
      '-a', 'highlight=@0@'.format(source_highlight.found()),
      '-a', 'build_root=@0@'.format(project_build_root),
      '-a', 'source_root=@0@'.format(project_source_root),
      '-a', 'content=@0@'.format('@INPUT@'),
      '-a', 'name=@0@'.format('@PLAINNAME@'),
      '-a', 'parent_link=operation-api.html',
      '-a', 'parent=GEGL Operation API',
      '-a', 'quirks!',
      files('website-listing.adoc'),
    ],
    depends: website_listing_deps,
    build_by_default: true,
  )
endforeach

# website index
index_html = custom_target(
  'index_html',
  output: 'index.html',
  input: website_index,
  command: [
    asciidoc,
    '--unsafe',
    '-o', '@OUTPUT@',
    '-a', 'stylesheet=@0@'.format(gegl_css),
    '-a', 'quirks!',
    '@INPUT@',
  ],
  depends: website_index_deps,
  build_by_default : true,
)
