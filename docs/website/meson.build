
# stylesheets
foreach _file : stylesheets
  configure_file(
    input : _file,
    output : '@PLAINNAME@',
    copy: true,
  )
endforeach

# HTML files
if env.found() and can_run_host_binaries
  custom_target('class_hierarchy_html',
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

# asciidoc html files
if asciidoc.found()
  _tgt = []
  foreach _file: website_asciidoc_files
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
  index_html = _tgt[0]
else
  error('asciidoc is required to build website')
endif

# enscript html files
enscript_files = {
  'gegl.h' :
    project_source_root / 'gegl',
  'gegl-plugin.h' :
    project_source_root / 'gegl',
  'gegl-operation.h' :
    project_source_root / 'gegl' / 'operation',
  'gegl-operation-composer.h' :
    project_source_root / 'gegl' / 'operation',
  'gegl-operation-area-filter.h' :
    project_source_root / 'gegl' / 'operation',
  'gegl-operation-filter.h' :
    project_source_root / 'gegl' / 'operation',
  'gegl-operation-meta.h' :
    project_source_root / 'gegl' / 'operation',
  'gegl-operation-point-composer.h' :
    project_source_root / 'gegl' / 'operation',
  'gegl-operation-temporal.h' :
    project_source_root / 'gegl' / 'operation',
  'gegl-operation-point-filter.h' :
    project_source_root / 'gegl' / 'operation',
  'gegl-operation-point-render.h' :
    project_source_root / 'gegl' / 'operation',
  'gegl-operation-sink.h' :
    project_source_root / 'gegl' / 'operation',
  'gegl-operation-source.h' :
    project_source_root / 'gegl' / 'operation',
  'brightness-contrast.c' :
    project_source_root / 'operations' / 'common',
}

if enscript.found()
  foreach _file, _path : enscript_files
    custom_target('@0@.html'.format(_file).underscorify(),
      input: files(_path / _file),
      output: '@PLAINNAME@.html',
      command: [
        enscript,
        '-E', '--color', '--language=html', '-p', '@OUTPUT@', '@INPUT@'
        ],
      build_by_default: true,
    )
  endforeach
else
  warning('enscript required to generate website code examples')
endif

subdir('images')
subdir('operations')
