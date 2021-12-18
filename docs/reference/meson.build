
glib_prefix = dependency('glib-2.0').get_pkgconfig_variable('prefix')
glib_gtk_docdir = glib_prefix / 'share' / 'gtk-doc' / 'html'

# GTK doc
if gtkdoc_scan.found()
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
