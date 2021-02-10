
# image files
foreach _file : images
  configure_file(
    input: _file,
    output: '@PLAINNAME@',
    copy: true,
  )
endforeach

# Inheritance diagram
if dot.found()
  custom_target(
    'inheritance_png',
    input: inheritance_txt,
    output: 'inheritance.png',
    command: [dot, '-Tpng', '-o@OUTPUT@', '@INPUT@'],
    build_by_default: true,
  )
endif
