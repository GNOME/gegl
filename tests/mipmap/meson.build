
# environment
mipmap_test_env = gegl_test_env_no_ocl
mipmap_test_env.set('GEGL_TILE_SIZE','8x8')
mipmap_test_env.set('GEGL_MIPMAP_RENDERING','1')

rotate_test_env = mipmap_test_env
rotate_test_env.set('GEGL_THREADS','1')

# gegl mipmap tests
#   test parameter dictionary layout
#   <test name>:
#     [<gegl op list>],
#     [<img-cmp arg list>],
#     env,
#
mipmap_tests = {
  'invert':  [
    ['invert'],
    ['10'],
    mipmap_test_env,
  ],
  'invert-crop':  [
    ['invert',
     'crop', 'x=147', 'y=66', 'width=200', 'height=200'],
    ['10'],
    mipmap_test_env,
  ],
  'rotate':  [
    ['rotate', 'degrees=10.0'],
    ['20'],
    rotate_test_env,
  ],
  'rotate-crop':  [
    ['rotate', 'degrees=15',
     'crop', 'x=147', 'y=66', 'width=200','height=200'],
    ['10'],
    mipmap_test_env,
  ],
  'unsharp':  [
    ['unsharp-mask', 'scale=10.0'],
    ['20'],
    mipmap_test_env,
  ],
  'unsharp-crop':  [
    ['unsharp-mask', 'scale=10.0',
     'crop', 'x=147', 'y=66', 'width=200', 'height=200'],
    ['10'],
    mipmap_test_env,
  ],
}

mipmap_test_image = join_paths(
  meson.current_source_dir() ,'..', 'compositions', 'data',
  'car-stack.png'
)

foreach _test, _args : mipmap_tests
  _gegl_ops = _args[0]
  _imgcmp_args = _args[1]
  _env = _args[2]

  test( _test.underscorify(),
    test_runner,
    args: [
      '--test-name', _test,
      '--output-dir', meson.current_build_dir() / 'output',
      '--reference-path', meson.current_source_dir() / 'reference',
      '--input-file', mipmap_test_image,
      '--gegl-exe', gegl_bin,
      '--gegl-scale', '0.33',
      '--gegl-ops', _gegl_ops,
      '--imgcmp-exe', gegl_imgcmp,
      '--imgcmp-args', _imgcmp_args,
    ],
    env: _env,
    suite: 'mipmap',
    timeout: 60,
    is_parallel: gegl_test_parallel,
  )
endforeach
