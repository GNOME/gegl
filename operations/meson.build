
gegl_operations_build_dir = meson.current_build_dir()
gegl_operations = []

subdir('common-gpl3+')
subdir('common-cxx')
subdir('common')
subdir('core')
subdir('external')
subdir('generated')
subdir('json')
subdir('seamless-clone')
subdir('transform')
if get_option('workshop')
  subdir('workshop')
endif
