headers = files(
  'memory.hh',
  'bus.hh',
  'header.hh',
)

inc = include_directories('.')

subdir('cpu')
subdir('util')
subdir('io')

install_headers(headers, subdir: meson.project_name(), preserve_path: true)