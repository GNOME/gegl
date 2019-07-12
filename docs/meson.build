doc_config = configuration_data()
doc_config.set('top_srcdir', meson.source_root())

html_files = []
gtkdoc_files = []

#######################################
# Enscript files

if enscript.found()
  enscript_inputs = [
    [ 'gegl',              'gegl-plugin.h' ],
    [ 'gegl',              'gegl.h' ],
    [ 'gegl/operation',    'gegl-operation-area-filter.h' ],
    [ 'gegl/operation',    'gegl-operation-composer.h' ],
    [ 'gegl/operation',    'gegl-operation-filter.h' ],
    [ 'gegl/operation',    'gegl-operation-meta.h' ],
    [ 'gegl/operation',    'gegl-operation-point-composer.h' ],
    [ 'gegl/operation',    'gegl-operation-point-filter.h' ],
    [ 'gegl/operation',    'gegl-operation-point-render.h' ],
    [ 'gegl/operation',    'gegl-operation-sink.h' ],
    [ 'gegl/operation',    'gegl-operation-source.h' ],
    [ 'gegl/operation',    'gegl-operation-temporal.h' ],
    [ 'gegl/operation',    'gegl-operation.h' ],
    [ 'operations/common', 'brightness-contrast.c' ],
  ]

  enscript_files = []
  foreach input : enscript_inputs
    input_file = meson.source_root() / input[0] / input[1]
    output_file = input[1] + '.html'

    enscript_files += custom_target(output_file,
      input : input_file,
      output: output_file,
      command: [
        enscript,
        '--highlight',
        '--color',
        '--language', 'html',
        '--output', '@OUTPUT@',
        '@INPUT@',
      ],
      install: true,
      install_dir: gnome.gtkdoc_html_dir('gegl')
    )
  endforeach




  html_files += enscript_files
  # gtkdoc_files += enscript_files
endif

#######################################
# HTML files

html_files += custom_target('class-hierarchy.html',
  input : [ ],
  output: [ 'class-hierarchy.html' ],
  command: [ find_program('env'),
    'GEGL_SWAP=RAM',
    'GEGL_PATH='+ meson.build_root() / 'operations',
    introspect,
  ],
  capture: true,
)

if asciidoc.found()
  txt_files = files(
    'abyss_policy.txt',
    'build.txt',
    'commandline.txt',
    'contribute.txt',
    'development.txt',
    'editor.txt',
    'environment.txt',
    'features.txt',
    'gcut.txt',
    'gegl-chain.txt',
    'glossary.txt',
    'journal.txt',
    'NEWS.txt',
    'operation-api.txt',
    'source-overview.txt',
  )
  txt_files += [
    configure_file(
      input : 'copyright.txt.in',
      output: 'copyright.txt',
      configuration: doc_config,
    ),
    configure_file(
      input : 'hello-world.txt.in',
      output: 'hello-world.txt',
      configuration: doc_config,
    ),
    configure_file(
      input : 'index.txt.in',
      output: 'index.txt',
      configuration: doc_config,
    ),
  ]

  asciidoc_gen = generator(asciidoc,
    arguments: [
      '--unsafe',
      '-o', '@OUTPUT@',
      '-a', 'stylesheet=@0@'.format(meson.current_source_dir() / 'gegl.css'),
      '-a', 'quirks!',
      '@INPUT@'
    ],
    output: '@BASENAME@.html',
  )

  html_files += asciidoc_gen.process(txt_files)
endif


if ruby.found()
  inheritance_dot = custom_target('inheritance.dot',
    output: 'inheritance.dot',
    command: [ gobj2dot, meson.source_root() ],
    capture: true,
    build_always_stale: true,
  )
  html_files += custom_target('inheritance.png',
    input :  inheritance_dot,
    output: 'inheritance.png',
    command: [ graphviz, '-Tpng', '@INPUT@', '-o', '@OUTPUT@' ],
  )
endif


#######################################
# GTK Doc files

gtkdoc_files += files(
  'gegl.css',
)

gtkdoc_images = files(
  'images' / 'standard-input.png',
  'images' / 'standard-panorama.png',
  'images' / 'standard-aux.png',
)


install_data(gtkdoc_files,
  install_dir: gnome.gtkdoc_html_dir('gegl')
)
install_data(gtkdoc_images,
  install_dir: gnome.gtkdoc_html_dir('gegl') / 'images'
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
