
xml_test_names = [
  'test-load',
  'test-roundtrip',
  'test-save',
]


foreach xml_test_name : xml_test_names
  xml_test = executable(xml_test_name,
    xml_test_name + '.c',
    include_directories: [ rootInclude, geglInclude, ],
    dependencies: [
      babl,
      glib,
      gobject,
    ],
    link_with: [
      gegl_lib,
    ]
  )
  test('xml_' + xml_test_name.underscorify(),
    xml_test,
    env: [
      'ABS_TOP_BUILDDIR=' + meson.build_root(),
      'ABS_TOP_SRCDIR='   + meson.source_root(),
      'GEGL_SWAP=RAM',
      'GEGL_PATH='+ join_paths(meson.build_root(), 'operations'),
    ],
    suite: 'xml',
  )

endforeach