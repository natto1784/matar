lib_sources = files(
  'memory.cc',
  'bus.cc'
)

subdir('util')
subdir('cpu')

lib_cpp_args = [ ]

fmt = dependency('fmt', version : '>=10.1.0', static: true)
if not fmt.found()
  fmt = dependency('fmt', version : '>=10.1.0', static: false)
  lib_cpp_args += '-DFMT_HEADER_ONLY'
endif

if get_option('disassembler')
  lib_cpp_args += '-DDISASSEMBLER'
endif

lib = library(
  meson.project_name(),
  lib_sources,
  dependencies: [fmt],
  include_directories: inc,
  install: true,
  cpp_args: lib_cpp_args
)

import('pkgconfig').generate(lib)
