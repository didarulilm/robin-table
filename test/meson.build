cc = meson.get_compiler('c')
test_inc = include_directories('include')
test_sources = files('t_robin_table.c')

test_exe = executable(
  't_robin_table', 
  test_sources,
  include_directories: [inc, test_inc],
  link_with: robin_table_lib 
)

test('t_robin_table', test_exe, verbose: true)
