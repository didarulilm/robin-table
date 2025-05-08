project(
  'robin-table',
  'c',
  version: '1.0.0',
  default_options: [
    'c_std=c99',
    'warning_level=3',  # -Wall -Wextra -Wpedantic
    'werror=true'
  ]
)

project_description = 'A fast Robin Hood hash table library in C'

add_project_arguments(
    '-D_POSIX_C_SOURCE=199309L',
    '-D_XOPEN_SOURCE=700',
    language: 'c'
)

inc = include_directories('include')

subdir('src')

if not meson.is_subproject()
  subdir('test')
endif
