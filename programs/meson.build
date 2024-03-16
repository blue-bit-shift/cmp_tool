cmp_tool_src = files([
  'cmp_guess.c',
  'cmp_io.c',
  'cmp_tool.c',
  'rdcu_pkt_to_file.c'
])


# generate the cmp_tool-config.h configuration file
cdata = configuration_data()
cdata.set_quoted('PROGRAM_NAME', 'cmp_tool')
cdata.set_quoted('CMP_TOOL_VERSION', meson.project_version())
if feature_argument_input_mode
  cdata.set('ARGUMENT_INPUT_MODE', 1)
endif

configure_file(
  output : 'cmp_tool-config.h',
  configuration : cdata
)


cmp_tool_exe = executable('cmp_tool',
  sources : cmp_tool_src,
  include_directories : incdir,
  link_with : cmp_lib,
  install : true
)
