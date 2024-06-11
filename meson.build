project('matar', 'cpp',
  version : '0.1',
  license : 'GPLv3',
  default_options : ['warning_level=3',
                     'werror=true',
                     'optimization=3',
                     'cpp_std=c++23',
                     'default_library=static'])

compiler = meson.get_compiler('cpp')

subdir('include')
subdir('src')
subdir('apps')

if get_option('tests')
  subdir('tests')
endif
