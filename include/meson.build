# generate the configuration file
cdata = configuration_data()
cdata.set_quoted('PROGRAM_NAME', 'cmp_tool')
cdata.set_quoted('VERSION_STR', meson.project_version())
if get_option('argument_input_mode')
  cdata.set('ARGUMENT_INPUT_MODE', '')
endif

configure_file(
  output : 'cmp_tool-config.h',
  configuration : cdata)

incdir = include_directories('.')
