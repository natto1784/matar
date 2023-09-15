project('matar', 'cpp',
  version : '0.1',
  license : 'GPLv3',
  default_options : ['warning_level=3',
                     'werror=true',
                     'optimization=3'])

compiler = meson.get_compiler('cpp')

if compiler.has_argument('-std=c++23')
  add_global_arguments('-std=c++23', language: 'cpp')
elif compiler.has_argument('-std=c++2b')
  add_global_arguments('-std=c++2b', language: 'cpp')
else
  error(compiler.get_id() + ' ' + compiler.version() + 'does not meet the compiler requirements')
endif

'''
TODO: use <print> and <format> instead of libfmt once LLVM 17 is out

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

subdir('tests')
