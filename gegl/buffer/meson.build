if host_cpu_family == 'x86_64'

  lib_gegl_x86_64_v2 = static_library('gegl-x86-64-v2', 'gegl-algorithms.c',
    include_directories:[geglInclude, rootInclude],
    dependencies:[glib, babl],
    c_args: [gegl_cflags ] + x86_64_v2_flags
  )

  lib_gegl_x86_64_v3 = static_library('gegl-x86-64-v3', 'gegl-algorithms.c',
    include_directories:[geglInclude, rootInclude],
    dependencies:[glib, babl],
    c_args: [gegl_cflags ] + x86_64_v3_flags
  )
elif host_cpu_family == 'arm'
  lib_gegl_arm_neon = static_library('gegl-arm-neon', 'gegl-algorithms.c',
    include_directories:[geglInclude, rootInclude],
    dependencies:[glib, babl],
    c_args: [gegl_cflags ] + arm_neon_flags
  )

endif

gegl_sources += files(
  'gegl-algorithms.c',
  'gegl-buffer-access.c',
  'gegl-buffer-config.c',
  'gegl-buffer-enums.c',
  'gegl-buffer-iterator.c',
  'gegl-buffer-iterator2.c',
  'gegl-buffer-linear.c',
  'gegl-buffer-load.c',
  'gegl-buffer-matrix2.c',
  'gegl-buffer-save.c',
  'gegl-buffer-swap.c',
  'gegl-buffer.c',
  'gegl-compression-nop.c',
  'gegl-compression-rle.c',
  'gegl-compression-zlib.c',
  'gegl-compression.c',
  'gegl-memory.c',
  'gegl-rectangle.c',
  'gegl-sampler-cubic.c',
  'gegl-sampler-linear.c',
  'gegl-sampler-lohalo.c',
  'gegl-sampler-nearest.c',
  'gegl-sampler-nohalo.c',
  'gegl-sampler.c',
  'gegl-scratch.c',
  'gegl-tile-alloc.c',
  'gegl-tile-backend-buffer.c',
  'gegl-tile-backend-file-async.c',
  'gegl-tile-backend-ram.c',
  'gegl-tile-backend-swap.c',
  'gegl-tile-backend.c',
  'gegl-tile-handler-cache.c',
  'gegl-tile-handler-chain.c',
  'gegl-tile-handler-empty.c',
  'gegl-tile-handler-log.c',
  'gegl-tile-handler-zoom.c',
  'gegl-tile-handler.c',
  'gegl-tile-source.c',
  'gegl-tile-storage.c',
  'gegl-tile.c',
)

gegl_introspectable_headers += files(
  'gegl-buffer-backend.h',
  'gegl-buffer-enums.h',
  'gegl-buffer-iterator.h',
  'gegl-buffer-matrix2.h',
  'gegl-buffer-swap.h',
  'gegl-buffer.h',
  'gegl-memory.h',
  'gegl-rectangle.h',
  'gegl-scratch.h',
  'gegl-tile-backend.h',
  'gegl-tile-handler.h',
  'gegl-tile-source.h',
)

gegl_headers += files(
  'gegl-tile.h',
)
