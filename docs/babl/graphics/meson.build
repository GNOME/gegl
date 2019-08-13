
graphic_files = [
  'babl-16x16.svg',
  'babl-48x48.svg',
  'babl-a4poster.svg',
]

graphic_files_install = []

# Dummy config to copy index.html to build
graphic_files_install += configure_file(
  input:  'index.html',
  output: '@PLAINNAME@',
  configuration: configuration_data()
)


foreach file : graphic_files
  # Dummy config to copy svg files to build
  graphic_files_install += configure_file(
    input:  file,
    output: '@PLAINNAME@',
    configuration: configuration_data()
  )
endforeach

if rsvg_convert_bin.found()
  foreach file : graphic_files
    sizeinfo = (
      file.contains('a4poster')
      ? [ '-w', '256', ]
      : []
    )

    graphic_files_install += custom_target(
      file,
      input: file,
      output: '@BASENAME@.png',
      command: [
        rsvg_convert_bin,
        sizeinfo,
        '-o', '@OUTPUT@',
        '@INPUT@',
      ],
      install: false,
      build_by_default: true,
    )
  endforeach
endif
