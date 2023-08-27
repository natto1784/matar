target_deps = [
  lib
]

target_sources = files(
  'main.cc'
)

executable(
  meson.project_name(),
  target_sources,
  link_with: target_deps,
  include_directories: inc,
  install : true,
)