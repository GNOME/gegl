
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
_tgt = []
foreach _file: website_asciidoc_files
  _tgt += custom_target('@0@.html'.format(_file).underscorify(),
    output: '@BASENAME@.html',
    input: _file,
    command: [
      asciidoc,
      '--unsafe',
      '-o', '@OUTPUT@',
      '-a', 'gegl_usage=@0@'.format(gegl_usage.full_path()),
      '-a', 'stylesheet=@0@'.format(
        meson.current_build_dir() / 'gegl.css'
      ),
      '-a', 'quirks!',
      '@INPUT@',
    ],
    build_by_default : true,
    depends: gegl_usage,
  )
endforeach
index_html = _tgt[0]

# Source files
src_gegl = project_source_root / 'gegl'
src_gegl_operation =  src_gegl / 'operation'
source_files = {
  'gegl.h' : src_gegl,
  'gegl-plugin.h' : src_gegl,
  'gegl-operation.h' : src_gegl_operation,
  'gegl-operation-composer.h' : src_gegl_operation,
  'gegl-operation-area-filter.h' : src_gegl_operation,
  'gegl-operation-filter.h' : src_gegl_operation,
  'gegl-operation-meta.h' : src_gegl_operation,
  'gegl-operation-point-composer.h' : src_gegl_operation,
  'gegl-operation-temporal.h' : src_gegl_operation,
  'gegl-operation-point-filter.h' : src_gegl_operation,
  'gegl-operation-point-render.h' : src_gegl_operation,
  'gegl-operation-sink.h' : src_gegl_operation,
  'gegl-operation-source.h' : src_gegl_operation,
  'brightness-contrast.c' : 
    project_source_root / 'operations' / 'common',
}

# asciidoc source files
_input = source_highlight.found() ? 'src.txt' : 'src-no-highlight.txt' 
# _input = 'src.txt'
foreach _file, _path : source_files
  custom_target('@0@.html'.format(_file).underscorify(),
    input: _input,
    output: '@0@.html'.format(_file),
    command: [
      asciidoc,
      '--unsafe',
      '-o', '@OUTPUT@',
      '-a', 'stylesheet=@0@'.format(
        meson.current_build_dir() / 'gegl.css'
      ),
      '-a', 'name=@0@'.format(_file),
      '-a', 'full_path=@0@'.format(_path / _file),
      '-a', 'quirks!',
      '@INPUT@',
    ],
    build_by_default: true,
  )
endforeach


subdir('images')
subdir('operations')
