tests_deps = [
  lib
]

tests_sources = files()

subdir('cpu')

catch2 = dependency('catch2-with-main', version: '>=3.4.0', static: true)
catch2_tests = executable(
  meson.project_name() + '_tests',
  tests_sources,
  dependencies: catch2,
  link_with: tests_deps,
  include_directories: inc,
  build_by_default: false
)

test('catch2 tests', catch2_tests)
