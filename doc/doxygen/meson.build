Doxyfile_template = files('Doxyfile.in')

doc_layout_files = files(
  'header.html',
  'meson.build',
  'plato-logo-med.png',
  'doxygen-awesome-css/doxygen-awesome-darkmode-toggle.js',
  'doxygen-awesome-css/doxygen-awesome-fragment-copy-button.js',
  'doxygen-awesome-css/doxygen-awesome-interactive-toc.js',
  'doxygen-awesome-css/doxygen-awesome-paragraph-link.js',
  'doxygen-awesome-css/doxygen-awesome-sidebar-only-darkmode-toggle.css',
  'doxygen-awesome-css/doxygen-awesome-sidebar-only.css',
  'doxygen-awesome-css/doxygen-awesome.css',
)


#Build a Doxyfile based on Doxyfile.in
cdata_doc = configuration_data()
cdata_doc.set('VERSION', meson.project_version())
cdata_doc.set('SRCDIR', meson.project_source_root())
cdata_doc.set('DOXYDIR', meson.current_source_dir())
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
    depend_files : [doxy_file, doc_layout_files, main, cmplib_sources], # test files are missing
    command : [doxygen, '@INPUT@'],
    build_by_default : false,
    console : true,
  )
else
  warning('Documentation disabled without doxygen')
endif
