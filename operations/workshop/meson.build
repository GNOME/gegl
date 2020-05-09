subdir('external')
subdir('generated')

libraries = [
  { 'name': 'aces-rrt', },
  { 'name': 'alpha-inpaint', },
  { 'name': 'bilateral-filter-fast', },
  { 'name': 'boxblur-1d', },
  { 'name': 'boxblur', },
  { 'name': 'connected-components', },
  { 'name': 'demosaic-bimedian', },
  { 'name': 'demosaic-simple', },
  { 'name': 'ditto', },
  { 'name': 'edges-and-details', },
  { 'name': 'gcr', },
  { 'name': 'gradient-map', },
  { 'name': 'hstack', },
  { 'name': 'integral-image', },
  { 'name': 'rawbayer-load', },
  { 'name': 'segment-kmeans', },
  { 'name': 'selective-hue-saturation', },
  { 'name': 'voronoi-diagram', 'srcs': 'voronoi-diagram.cc', },
]

foreach lib : libraries
  libname = lib.get('name')
  libsrcs = lib.get('srcs', libname + '.c')
  gegl_operations += shared_library(libname,
    libsrcs,
    opencl_headers,
    include_directories: [ rootInclude, geglInclude, seamlessInclude, ],
    dependencies: [
      babl,
      glib,
      gobject,
      math,
    ],
    link_with: [
      gegl_lib,
      seamlessclone_lib,
    ],
    name_prefix: '',
    install: true,
    install_dir: get_option('libdir') / api_name,
  )
endforeach
