project('pcg-c-basic', 'c',
  version: '0.9',
  license: 'Apache 2.0',
)

inc = include_directories('.')
libpcg_basic = static_library('pcg_basic',
  'pcg_basic.c',
  include_directories : inc,
  install : true)

libpcg_basic_dep = declare_dependency(include_directories : inc,
  link_with : libpcg_basic)
