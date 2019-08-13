
test_list = [
  'bilateral-filter',
  'box-blur',
  'brightness-contrast',
  'color-temperature',
  'edge-sobel',
  'gaussian-blur',
  'invert-linear',
  'levels',
  'mono-mixer',
  'motion-blur',
  'noise-reduction',
  'oilify',
  'opacity',
  'pixelize',
  'snn-mean',
  'svg-src-over',
  'threshold',
  'value-invert',
  'vignette',
]

opencl_test = find_program('opencl_test.sh')
foreach test_name: test_list
  xml_file = meson.current_source_dir() / test_name + '.xml'
  out_dir  = meson.current_build_dir()
  out_name = test_name + '.png'

  test(test_name,
    opencl_test,
    env: [
      'ABS_BUILDDIR='+ out_dir,
      'GEGL_BIN='+ gegl_bin.full_path(),
      'GEGL_IMGCMP_BIN='+ gegl_imgcmp.full_path(),
      'XML_FILE='+ xml_file,
      'OUT_FILE='+ out_name,
      'GEGL_SWAP=RAM',
      'GEGL_PATH='+ meson.build_root() / 'operations',
    ],
    suite: 'opencl',
    is_parallel: false,
  )
endforeach
