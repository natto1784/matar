lib_sources = files(
  'bus.cc',
)

if get_option('gdb_debug')
  lib_sources += files('gdb_rsp.cc')
endif

subdir('util')
subdir('cpu')
subdir('io')

lib = library(
  meson.project_name(),
  lib_sources,
  include_directories: inc,
  install: true,
  cpp_args: lib_cpp_args
)

import('pkgconfig').generate(lib)
