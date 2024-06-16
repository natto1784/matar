project('matar', 'cpp',
  version : '0.1',
  license : 'GPLv3',
  default_options : ['warning_level=3',
                     'werror=true',
                     'optimization=3',
                     'cpp_std=c++23',
                     'default_library=static'])

lib_cpp_args = []
compiler = meson.get_compiler('cpp')

if get_option('disassembler')
  lib_cpp_args += '-DDISASSEMBLER'
endif

if get_option('gdb_debug')
  lib_cpp_args += '-DGDB_DEBUG'
endif


subdir('include')
subdir('src')
subdir('apps')

if get_option('tests')
  subdir('tests')
endif
