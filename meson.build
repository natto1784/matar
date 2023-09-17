project('matar', 'cpp',
  version : '0.1',
  license : 'GPLv3',
  default_options : ['warning_level=3',
                     'werror=true',
                     'optimization=3',
                     'cpp_std=c++20'])

compiler = meson.get_compiler('cpp')

'''
TODO: use <print> and <format> instead of libfmt once LLVM 17 is out

if compiler.has_argument('-std=c++2c')
  add_global_arguments('-std=c++2c', language: 'cpp')
elif compiler.has_argument('-std=c++23')
  add_global_arguments('-std=c++23', language: 'cpp')
elif compiler.has_argument('-std=c++2b')
  add_global_arguments('-std=c++2b', language: 'cpp')
elif compiler.has_argument('-std=c++20')
  add_global_arguments('-std=c++20', language: 'cpp')
else
  error(compiler.get_id() + ' ' + compiler.version() + 'does not meet the compiler requirements')
endif

if compiler.has_argument('-fexperimental-library')
  add_global_arguments('-fexperimental-library', language: 'cpp')
else
  error(compiler.get_id() + ' ' + compiler.version() + 'does not support -fexperimental-library')
endif
'''

inc = include_directories('include')

subdir('include')
subdir('src')
subdir('apps')

if get_option('tests')
  subdir('tests')
endif
