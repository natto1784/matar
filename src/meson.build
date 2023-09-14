lib_sources = files(
  'memory.cc',
  'bus.cc'
)

subdir('cpu')

fmt = dependency('fmt', version : '>=10.1.0')
lib = library(
  meson.project_name(),
  lib_sources,
  dependencies: [fmt],
  include_directories: inc,
  install: true,
  cpp_args: '-DFMT_HEADER_ONLY'
)

import('pkgconfig').generate(lib)
