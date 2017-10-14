index_txt = configure_file(
  input: 'index.txt.in',
  output: 'index.txt',
  configuration: conf,
)
helloworld_txt = configure_file(
  input: 'hello-world.txt.in',
  output: 'hello-world.txt',
  configuration: conf,
)

asciidoc_files = [
  index_txt,
  'abyss_policy.txt',
  'bugzilla.txt',
  'build.txt',
  'commandline.txt',
  'contribute.txt',
  'copyright.txt',
  'development.txt',
  'editor.txt',
  'environment.txt',
  'features.txt',
  'gcut.txt',
  'gegl-chain.txt',
  'glossary.txt',
  helloworld_txt,
  'journal.txt',
  'NEWS.txt',
  'operation-api.txt',
  'source-overview.txt',
]

foreach file: asciidoc_files
  rendered_file = custom_target('@0@.html'.format(file).underscorify(),
    output: '@BASENAME@.html',
    input: file,
    command: [ asciidoc,
      '--unsafe',
      '-o', '@OUTPUT@',
      '-a', 'stylesheet="@0@"'.format(join_paths(meson.source_root(), 'gegl.css')),
      '-a', 'quirks!',
      '@INPUT@',
    ],
  )
endforeach

if ruby.found() and dot.found()
  inheritance_temp = custom_target('inheritance.dot',
    input: meson.source_root(),
    output: 'inheritance.dot',
    command: [ gobj2dot,
      '@INPUT@',
    ],
    capture: true,
    build_by_default: true,
  )

  inheritance_img = custom_target('inheritance.png',
    input: inheritance_temp,
    output: 'inheritance.png',
    command: [ dot,
      '-Tpng',
    ],
    capture: true,
    build_by_default: true,
  )
endif

enscript_files = [
  join_paths(meson.source_root(), 'gegl', 'gegl.h'),
  join_paths(meson.source_root(), 'gegl', 'gegl-plugin.h'),
  join_paths(meson.source_root(), 'operations', 'common', 'brightness-contrast.c'),
  join_paths(meson.source_root(), 'gegl', 'operation', 'gegl-operation.h'),
  join_paths(meson.source_root(), 'gegl', 'operation', 'gegl-operation-composer.h'),
  join_paths(meson.source_root(), 'gegl', 'operation', 'gegl-operation-area-filter.h'),
  join_paths(meson.source_root(), 'gegl', 'operation', 'gegl-operation-filter.h'),
  join_paths(meson.source_root(), 'gegl', 'operation', 'gegl-operation-meta.h'),
  join_paths(meson.source_root(), 'gegl', 'operation', 'gegl-operation-point-composer.h'),
  join_paths(meson.source_root(), 'gegl', 'operation', 'gegl-operation-temporal.h'),
  join_paths(meson.source_root(), 'gegl', 'operation', 'gegl-operation-point-filter.h'),
  join_paths(meson.source_root(), 'gegl', 'operation', 'gegl-operation-point-render.h'),
  join_paths(meson.source_root(), 'gegl', 'operation', 'gegl-operation-sink.h'),
  join_paths(meson.source_root(), 'gegl', 'operation', 'gegl-operation-source.h'),
]

foreach file: enscript_files
  path = file.split('/')
  basename = path[path.length() - 1]
  rendered_file = custom_target('@0@.html'.format(file).underscorify(),
    input: file,
    output: '@0@.html'.format(basename),
    command: [ enscript,
      '-E',
      '--color',
      '--language=html',
      '-p', '@OUTPUT@',
      '@INPUT@',
    ],
  )
endforeach
