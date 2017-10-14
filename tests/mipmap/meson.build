mipmap_tests = [
  'invert.sh',
  'invert-crop.sh',
  'unsharp-crop.sh',
  'unsharp.sh',
  'rotate.sh',
  'rotate-crop.sh',
]

foreach _test: mipmap_tests
  test(_test, find_program(_test),
    env: test_env,
  )
endforeach
