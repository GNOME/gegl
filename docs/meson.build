doc_config = configuration_data()
doc_config.set('top_srcdir', meson.source_root())

html_files = []
gtkdoc_files = []

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
  depends: gegl_operations,
  build_by_default : true,
  capture: true,
)

if asciidoc.found()
asciidoc_files = files(
    'abyss_policy.txt',
    'build.txt',
    'commandline.txt',
    'contribute.txt',
    'development.txt',
    'editor.txt',
    'environment.txt',
    'features.txt',
    'gegl-chain.txt',
    'glossary.txt',
    'journal.txt',
    'NEWS.txt',
    'operation-api.txt',
    'source-overview.txt',
  )
asciidoc_files += [
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


foreach file: asciidoc_files
  rendered_file = custom_target('@0@.html'.format(file).underscorify(),
    output: '@BASENAME@.html',
    input: file,
    build_by_default : true,
    command: [ asciidoc,
      '--unsafe',
      '-o', '@OUTPUT@',
      '-a', 'stylesheet=@0@'.format(join_paths(meson.source_root(), 'docs', 'gegl.css')),
      '-a', 'quirks!',
      '@INPUT@',
    ],
  )
endforeach


endif


#######################################
# GTK Doc files

gtkdoc_files += files(
  'gegl.css',
)

gtkdoc_images = files(
  'images/standard-input.png',
  'images/standard-panorama.png',
  'images/standard-aux.png',
  'images/GEGL.png',
)


install_data(gtkdoc_files,
  install_dir: gnome.gtkdoc_html_dir('gegl')
)
install_data(gtkdoc_images,
  install_dir: join_paths (gnome.gtkdoc_html_dir('gegl') , 'images')
)



configure_file(input: 'images/GEGL.png',
               output: 'GEGL.png',
               copy: true,
)


docpath = join_paths(get_option('datadir'), 'gtk-doc', 'html')

glib_prefix = dependency('glib-2.0').get_pkgconfig_variable('prefix')
glib_docpath = join_paths(glib_prefix, 'share', 'gtk-doc', 'html')


gnome.gtkdoc('gegl',
             dependencies : gegl_lib,
             main_xml : meson.project_name() + '-docs.xml',
  src_dir: [
    join_paths(meson.source_root(), 'gegl'),
    join_paths(meson.build_root(), 'gegl'),
  ],

  fixxref_args: [
    '--html-dir=@0@'.format(docpath),
    '--extra-dir=@0@'.format(glib_docpath),
  ],
  install : true,
)


 doc_operations_examples_dir = join_paths(
   meson.current_build_dir(), 'operations',
 )

# Source operations
exclude = [
  'gegl:buffer-source',
  'gegl:exr-load',
  'gegl:ff-load',
  'gegl:gegl-buffer-load',
  'gegl:gif-load',
  'gegl:icc-load',
  'gegl:jp2-load',
  'gegl:jpg-load',
  'gegl:load',
  'gegl:magick-load',
  'gegl:open-buffer',
  'gegl:pdf-load',
  'gegl:pixbuf',
  'gegl:png-load',
  'gegl:ppm-load',
  'gegl:rawbayer-load',
  'gegl:raw-load',
  'gegl:rgbe-load',
  'gegl:svg-load',
  'gegl:tiff-load',
  'gegl:v4l',
  'gegl:v4l2',
  'gegl:webp-load',
]
# Sink operations
exclude += [
  'gegl:buffer-sink',
  'gegl:display',
  'gegl:exr-save',
  'gegl:ff-save',
  'gegl:gegl-buffer-save',
  'gegl:icc-save',
  'gegl:jpg-save',
  'gegl:npy-save',
  'gegl:png-save',
  'gegl:ppm-save',
  'gegl:rgbe-save',
  'gegl:save',
  'gegl:save-pixbuf',
  'gegl:sdl2-display',
  'gegl:tiff-save',
  'gegl:webp-save',
  'gegl:write-buffer',
]
# Programming operations
exclude += [
  'gegl:cache',
  'gegl:cast-format',
  'gegl:clone',
  'gegl:convert-format',
  'gegl:copy-buffer',
  'gegl:image-compare',
  'gegl:line-profile',
  'gegl:nop',
]
# Other operations
exclude += [
  'gegl:exp-combine',
  'gegl:gegl',
  'gegl:introspect',
  'gegl:lcms-from-profile',
  'gegl:mblur',
]

# Crashing - workshop
exclude += [
  'gegl:alpha-inpaint',
  'gegl:integral-image',
  'gegl:lens-correct',
]
exclusion_pattern = '|'.join(exclude)


doc_operations_examples = custom_target('gallery-stamp',
   install:true,
   install_dir: gnome.gtkdoc_html_dir('gegl'),
   output: ['gallery-stamp'],
   command: [ bash, '-c',
     ' '.join([
     'mkdir', '-p', doc_operations_examples_dir,
     '&&',
     'cd', join_paths(meson.current_build_dir(), 'operations'),
     '&&',
     'GEGL_PATH=' + join_paths(meson.build_root(), 'operations'),
     operations_html.full_path(),
     '&&',
     'echo', 'Generating example images',
     '&&',
     'BABL_TOLERANCE=0.0',
     'GEGL_SWAP=RAM',
     'GEGL_PATH=' + join_paths(meson.build_root(), 'operations'),
     gegl_tester.full_path(),
     '--all', '-o',  doc_operations_examples_dir,
     '-d', join_paths(meson.current_source_dir(), 'images'),
     '-e', '"' + exclusion_pattern + '"', '||', 'true', '&&', 'touch', '../gallery-stamp'
     ])
   ],
   depends: gegl_operations,
 )

meson.add_install_script('install-docs.sh')
