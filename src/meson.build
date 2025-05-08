sources = files(
  'robin_table.c',
  'rapidhash.c',
  'siphash.c',
  'xxh64.c'
)

robin_table_lib = library(
  meson.project_name(), 
  sources,
  include_directories: inc,
  install: true
)

robin_table_dep = declare_dependency(
  include_directories: inc,
  link_with: robin_table_lib 
)

install_headers('../include/robin_table.h')

pkg = import('pkgconfig')

pkg.generate(
  libraries: robin_table_lib,
  version: meson.project_version(),
  name: meson.project_name(), 
  filebase: meson.project_name(),
  description: project_description 
)
