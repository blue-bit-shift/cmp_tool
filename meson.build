project('cmp_tool', 'c',
  version : '0.12-b2',
  meson_version : '>= 0.56',
  license : 'GPL-2.0',
  default_options : [
    'warning_level=3',
    'c_std=gnu99'
  ]
)

cc = meson.get_compiler('c')

# Built-in options
use_debug = get_option('debug')

# Custom options
debug_level = get_option('debug_level')
feature_argument_input_mode = get_option('argument_input_mode')


# Compiler flags
cc_flags = ['-DDEBUGLEVEL=@0@'.format(debug_level)]
if use_debug
  debug_flags = [
    '-Wstrict-aliasing=1',
    '-Wcast-align',
    '-Wredundant-decls',
    '-Wundef',
    '-Wshadow',
    '-Wdeclaration-after-statement',
    '-Wstrict-prototypes',
    '-Wpointer-arith',
    '-Wvla',
    '-Wformat=2',
    '-Winit-self',
    '-Wfloat-equal',
    '-Wwrite-strings',
    '-Wold-style-definition',
    '-Waggregate-return',
    '-Wmissing-declarations',
    '-Wmissing-include-dirs'
  ]
  cc_flags += cc.get_supported_arguments(debug_flags)
endif
add_project_arguments(cc_flags, language : 'c')

if ['windows', 'cygwin'].contains(host_machine.system()) and cc.get_id() == 'gcc'
  # by default, MinGW on win32 behaves as if it ignores __attribute__((packed)),
  # you need to add -mno-ms-bitfields to make it work as expected.
  # See: https://wintermade.it/blog/posts/__attribute__packed-on-windows-is-ignored-with-mingw.html
  add_project_arguments('-mno-ms-bitfields', language : 'c')
  add_global_link_arguments('-static', language: 'c')
endif


# Subdirs
subdir('lib')
subdir('programs')
subdir('test')
subdir('doc/doxygen')
