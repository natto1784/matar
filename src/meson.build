lib_sources = files(
  'emulator.cc'
)

subdir('cpu')

lib = library(
  meson.project_name(),
  lib_sources,
  include_directories: inc,
  install: true
)

import('pkgconfig').generate(lib)