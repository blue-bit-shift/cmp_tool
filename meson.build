project('cmp_tool', 'c',
  version : '0.10-b.2',
  meson_version : '>= 0.56',
  license : 'GPL-2.0',
  default_options : ['warning_level=3', 'c_std=gnu99']
)

add_project_arguments('-DDEBUGLEVEL=1', language : 'c')

if (host_machine.system() == 'windows' or host_machine.system() == 'cygwin') and meson.get_compiler('c').get_id() == 'gcc'
  # by default, MinGW on win32 behaves as if it ignores __attribute__((packed)),
  # you need to add -mno-ms-bitfields to make it work as expected.
  # See: https://wintermade.it/blog/posts/__attribute__packed-on-windows-is-ignored-with-mingw.html
  add_project_arguments('-mno-ms-bitfields', language : 'c')
  add_global_link_arguments('-static', language: 'c')
endif

subdir('include')
subdir('lib')

main = files('cmp_tool.c')

cmp_tool_exe = executable('cmp_tool',
  sources : main,
  include_directories : incdir,
  link_with : cmp_lib,
  install : 'true'
)

subdir('test')
subdir('doc/doxygen')
