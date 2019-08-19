compositions_tests = files(
  'affine-with-negative.xml',
  'alien-map.xml',
  'apply-lens.xml',
  'apply-lens3.xml',
  'bump-map.xml',
  'checkerboard.xml',
  'clones.xml',
  'color-enhance.xml',
  'color-exchange.xml',
  'color-to-alpha.xml',
  'colors.xml',
  'composite-transform.xml',
  'contrast-curve.xml',
  'convolve1.xml',
  'convolve2.xml',
  'edge.xml',
  'fattal02.xml',
  'gamma.xml',
  'gegl.xml',
  'grey-json.xml',
  'hdr-color.xml',
  'illusion.xml',
  'image-compare.xml',
  'lens-flare.xml',
  'mantiuk06.xml',
  'noise-cell.xml',
  'noise-hurl.xml',
  'noise-simplex.xml',
  'noise-solid.xml',
  'posterize.xml',
  'rectangles.xml',
  'red-eye-removal.xml',
  'reflect.xml',
  'reflect2.xml',
  'reinhard05.xml',
  'rgb-params.xml',
  'rotate-on-center.xml',
  'rotate.xml',
  'scale-size-keepaspect.xml',
  'simple-scale.xml',
  'sinus.xml',
  'stretch-contrast.xml',
  'supernova.xml',
  'transform.xml',
  'weighted-blend.xml',
)
#run-matting-global.xml.sh
#run-matting-global-big.xml.sh

compositions_tests_without_opencl = files(
  'color-reduction.xml',
  'jpg-load-datauri.xml',
  'pnm-ascii-load.xml',
  'pnm-raw-load.xml',
  'rgbe-load.xml',
  'rgbe-save.xml',
  'shift.xml',
  'stretch-contrast-hsv.xml',
  'tiff-load.xml',
  'tile.xml',
)


if jasper.found()
  compositions_tests_without_opencl += 'jp2-load.xml'
endif
if libumfpack.found()
 # compositions_tests += 'matting-levin.xml'
endif

run_compositions = files('run-compositions.py')

if false
test('compositions_with_opencl',
  python,
  args: [
    run_compositions,
    '--build-dir='+ meson.build_root(),
    '--src-dir='  + meson.source_root(),
    '--xml-dir='  + meson.current_source_dir(),
    compositions_tests,
  ],
  is_parallel: false,
  timeout: 120,
  suite: 'compositions',
)
endif

test('compositions_without_opencl',
  python,
  args: [
    run_compositions,
    '--build-dir='+ meson.build_root(),
    '--src-dir='  + meson.source_root(),
    '--xml-dir='  + meson.current_source_dir(),
    '--without-opencl',
    compositions_tests_without_opencl,
  ],
  suite: 'compositions',
  is_parallel: false,
)
