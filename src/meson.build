lib_sources = files(
  'memory.cc',
  'bus.cc'
)

subdir('util')
subdir('cpu')

lib = library(
  meson.project_name(),
  lib_sources,
  install: true
)

import('pkgconfig').generate(lib)