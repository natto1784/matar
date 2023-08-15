lib_sources = files(
  'matar.cc'
)

subdir('cpu')

lib = library(
  meson.project_name(),
  lib_sources,
  include_directories: includes,
  install : true
)