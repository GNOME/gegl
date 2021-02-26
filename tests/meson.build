
# Test runner
test_runner = find_program('test-runner.py')

# Set up test environment variables
gegl_test_env = environment()
gegl_test_env.set('ABS_TOP_SRCDIR', project_source_root)
gegl_test_env.set('ABS_TOP_BUILDDIR', project_build_root)
gegl_test_env.set('GEGL_SWAP', 'RAM')
gegl_test_env.set('GEGL_PATH', project_build_root / 'operations')
if os_win32
  gegl_test_env.prepend('PATH', gegl_library_build_dir)
endif

# Environment for tests without OpenCL
gegl_test_env_no_ocl = gegl_test_env
gegl_test_env_no_ocl.set('GEGL_USE_OPENCL', 'no')

# common test includes
gegl_test_includes = [
  rootInclude,
  geglInclude,
]

# Common test dependencies
gegl_test_deps = [
  babl,
  glib,
  gobject,
  math,
]

# common test paralllel flag
gegl_test_parallel = get_option('parallel-tests')


# Tests
subdir('simple')
subdir('buffer')
subdir('xml')
subdir('mipmap')
subdir('opencl')
subdir('compositions')
if pygobject3.found()
  subdir('python')
endif
if get_option('operation-test')
  subdir('operations')
endif
if avlibs_found and gexiv2.found()
  subdir('ff-load-save')
endif
