tests_deps = [
  lib
]

src = include_directories('../src')

tests_sources = files(
  'main.cc',
  'bus.cc',
  'memory.cc'
)

subdir('cpu')
subdir('util')

tests_cpp_args = []

if get_option('disassembler')
  tests_cpp_args += '-DDISASSEMBLER'
endif

catch2 = dependency('catch2', version: '>=3.4.0', static: true)
catch2_tests = executable(
  'matar_tests',
  tests_sources,
  dependencies: catch2,
  link_with: tests_deps,
  include_directories: [inc, src],
  build_by_default: false,
  cpp_args: tests_cpp_args
)

test('catch2 tests', catch2_tests)
