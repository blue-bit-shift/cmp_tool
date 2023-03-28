Doxyfile_template = files('Doxyfile.in')

doc_layout_files = files(
  'header.html',
  'plato-logo-med.png'
)

doxygen_awesome_css_proj = subproject('doxygen-awesome-css')
doxygen_awesome_css_files = doxygen_awesome_css_proj.get_variable('doxygen_awesome_files')
doxygen_awesome_css_dep = doxygen_awesome_css_proj.get_variable('doxygen_awesome_css_dep')
doxygen_awesome_dir = doxygen_awesome_css_dep.get_variable(internal : 'doxygen_awesome_dir')
message(doxygen_awesome_dir)

#Build a Doxyfile based on Doxyfile.in
cdata_doc = configuration_data()
cdata_doc.set('VERSION', meson.project_version())
cdata_doc.set('SRCDIR', meson.project_source_root())
cdata_doc.set('DOXYDIR', meson.current_source_dir())
cdata_doc.set('AWESOMEDIR', doxygen_awesome_dir)
cdata_doc.set('BUILDDIR', meson.project_build_root())
if find_program('dot', required : false).found()
  cdata_doc.set('HAVE_DOT', 'YES')
else
  cdata_doc.set('HAVE_DOT', 'NO')
endif

doxygen = find_program('doxygen', required : false)

if doxygen.found()
  doxy_file = configure_file(
    input : Doxyfile_template,
    output : 'Doxyfile',
    configuration : cdata_doc,
    install : false,
  )

  custom_target('doc',
    input : doxy_file,
    output : 'generated_documentation',
    depend_files : [doxy_file, doc_layout_files, doxygen_awesome_css_files, main, cmplib_sources], # test files are missing
    command : [doxygen, '@INPUT@'],
    build_by_default : false,
    console : true,
  )
else
  warning('Documentation disabled without doxygen')
endif
