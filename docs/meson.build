
html_files = []

if asciidoc.found()
  html_files += [
    'abyss_policy.html',
    'bugzilla.html',
    'build.html',
    'commandline.html',
    'contribute.html',
    'copyright.html',
    'development.html',
    'editor.html',
    'environment.html',
    'features.html',
    'gcut.html',
    'gegl-chain.html',
    'glossary.html',
    'hello-world.html',
    'index.html',
    'journal.html',
    'NEWS.html',
    'operation-api.html',
    'source-overview.html',
  ]
endif

class_hierarchy = custom_target('class-hierarchy.html',
  input : [ ],
  output: [ 'class-hierarchy.html' ],
  command: [ env,
    'GEGL_SWAP=RAM',
    'GEGL_PATH='+ join_paths(meson.build_root(), 'operations'),
    introspect,
  ],
  capture: true,
)

if ruby.found()
  inheritance_dot = custom_target('inheritance.dot',
    input : [ ],
    output: [ 'inheritance.dot' ],
    command: [ gobj2dot, meson.source_root() ],
    capture: true,
  )
  inheritance_png = custom_target('inheritance.png',
    input : [  inheritance_dot  ],
    output: [ 'inheritance.png' ],
    command: [ dot, '-Tpng', '@INPUT@', '-o', '@OUTPUT@' ],
  )
  html_files += inheritance_png
endif

if enscript.found()
  enscript_files = [
    [ join_paths('gegl'),                 'gegl-plugin.h' ],
    [ join_paths('gegl'),                 'gegl.h' ],
    [ join_paths('operations', 'common'), 'brightness-contrast.c' ],
    [ join_paths('operations'),           'gegl-operation-area-filter.h' ],
    [ join_paths('operations'),           'gegl-operation-composer.h' ],
    [ join_paths('operations'),           'gegl-operation-filter.h' ],
    [ join_paths('operations'),           'gegl-operation-meta.h' ],
    [ join_paths('operations'),           'gegl-operation-point-composer.h' ],
    [ join_paths('operations'),           'gegl-operation-point-filter.h' ],
    [ join_paths('operations'),           'gegl-operation-point-render.h' ],
    [ join_paths('operations'),           'gegl-operation-sink.h' ],
    [ join_paths('operations'),           'gegl-operation-source.h' ],
    [ join_paths('operations'),           'gegl-operation-temporal.h' ],
    [ join_paths('operations'),           'gegl-operation.h' ],
  ]
  foreach file: enscript_files
    enscript_html = custom_target(file[1] + '.html',
      input: join_paths(meson.source_root(), file[0], file[1]),
      output: file[1] + '.html',
      command: [
        enscript,
        '-E', '--color', '--language=html',
        '-p', '@OUTPUT@',
        '@INPUT@',
      ],
    )
    html_files += enscript_html
  endforeach
endif

install_data([
    'gegl.css',
  ],
  install_dir: join_paths(gnome.gtkdoc_html_dir('gegl'))
)
install_data([
    join_paths('images', 'standard-input.png'),
    join_paths('images', 'standard-panorama.png'),
    join_paths('images', 'standard-aux.png'),
  ],
  install_dir: join_paths(gnome.gtkdoc_html_dir('gegl'), 'images')
)


# doc_operations_examples_dir = join_paths(
#   meson.current_build_dir(), 'operations', 'images'
# )
# exclusion_pattern = '|'.join([
#   'box-blur', 'box-percentile', 'buffer-cache', 'buffer-source',
#   'clone', 'convert-format',
#   'disc-percentile', 'dropshadow',
#   'exp-combine', 'exr-load',
#   'hstack',
#   'image-compare', 'integral-image', 'introspect',
#   'jpg-load',
#   'kuwahara',
#   'layer', 'line-profile', 'load',
#   'magick-load', 'mandelbrot', 'matting-global',
#   'nop',
#   'open-buffer',
#   'pixbuf', 'png-load',
#   'remap',
#   'snn-percentile', 'stretch-contrast', 'svg-load',
#   'v4l2',
#   'warp',
# ])

# doc_operations_examples = run_target('doc_examples',
#   command: [ bash, '-c',
#     ' '.join([
#     'echo', 'Generating example images',
#     '&&',
#     'mkdir', '-p', doc_operations_examples_dir,
#     '&&',
#     'BABL_TOLERANCE=0.0',
#     'GEGL_SWAP=RAM',
#     'GEGL_PATH=' + join_paths(meson.source_root(), 'operations'),
#     gegl_tester.full_path(),
#     '--all', '-o',  doc_operations_examples_dir,
#     '-d', join_paths(meson.current_source_dir(), 'images'),
#     '-e', '"' + exclusion_pattern + '"',
#     '&&',
#     'cd', join_paths(meson.current_build_dir(), 'operations'),
#     '&&',
#     'GEGL_PATH=' + join_paths(meson.build_root(), 'operations'),
#     operations_html.full_path(),
#     ])
#   ],
# )
