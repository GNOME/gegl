
# image files
foreach _file : images
  configure_file(
    input: _file,
    output: '@PLAINNAME@',
    copy: true,
  )
endforeach

# Inheritance diagram
if is_variable('inheritance_txt') and dot.found()
  custom_target(
    'inheritance_png',
    input: inheritance_txt,
    output: 'inheritance.png',
    command: [dot, '@INPUT@', '-Tpng'],
    capture: true,
    build_by_default: true,
  )
endif
