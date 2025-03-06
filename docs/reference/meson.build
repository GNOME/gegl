
glib_prefix = dependency('glib-2.0').get_variable(pkgconfig: 'prefix')
glib_gtk_docdir = glib_prefix / 'share' / 'gtk-doc' / 'html'

# GTK doc
if get_option('gtk-doc')
  gnome.gtkdoc(
    'gegl',
    dependencies: gegl_lib,
    main_xml: meson.project_name() + '-docs.xml',
    src_dir: [
      project_source_root / 'gegl',
      project_build_root / 'gegl',
    ],
    fixxref_args: [
      '--html-dir=@0@'.format(gegl_gtk_docdir),
      '--extra-dir=@0@'.format(glib_gtk_docdir),
    ],
    install : true,
  )
endif

# GI-docgen
if build_gi_docgen
  # Extra markdown files
  gegl_doc_content_files = [
  ]

  gegl_doc_toml = configure_file(
    input: 'gegl.toml.in',
    output: '@BASENAME@',
    configuration: {
      'GEGL_VERSION':   version,
      'GEGL_LOGO_PATH': 'https://gegl.org/images/GEGL.png',
    },
  )

  gegl_docs = custom_target('gegl-docs',
    input: gegl_gir[0],
    output: api_name,
    command: [
      gi_docgen,
      'generate',
      '--quiet',
      '--fatal-warnings',
      '--config', gegl_doc_toml,
      '--output-dir=@OUTPUT@',
      '--no-namespace-dir',
      '--content-dir=@0@'.format(meson.current_source_dir()),
      '--add-include-path=@0@'.format(get_option('prefix') / 'share' / 'gir-1.0'),
      '@INPUT@',
    ],
    depend_files: [
      gegl_doc_toml,
      gegl_doc_content_files,
    ],
    build_by_default: true,
    install: true,
    install_dir: get_option('datadir') / 'doc',
  )
endif
