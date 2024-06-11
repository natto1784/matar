lib_sources = files(
  'memory.cc',
  'bus.cc',
  'io.cc'
)

subdir('util')
subdir('cpu')

lib_cpp_args = []

if get_option('disassembler')
  lib_cpp_args += '-DDISASSEMBLER'
endif

lib = library(
  meson.project_name(),
  lib_sources,
  include_directories: inc,
  install: true,
  cpp_args: lib_cpp_args
)

import('pkgconfig').generate(lib)
