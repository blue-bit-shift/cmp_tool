doc_layout_files = files([
  'DoxygenLayout.xml',
  'bootstrap.min.css',
  'customdoxygen.css',
  'bootstrap.min.js',
  'doxy-boot.js',
  'header.html',
  'footer.html'
])

doxygen = find_program('doxygen', required : false)

#Build a Doxyfile based on Doxyfile.in
cdata_doc = configuration_data()
cdata_doc.set('VERSION', meson.project_version())
cdata_doc.set('SRCDIR', meson.project_source_root())
cdata_doc.set('BUILDDIR', meson.project_build_root())
if find_program('dot', required : false).found()
  cdata_doc.set('HAVE_DOT', 'YES')
else
  cdata_doc.set('HAVE_DOT', 'NO')
endif

doxy_file = configure_file(
  input : 'Doxyfile.in',
  output : 'Doxyfile',
  configuration : cdata_doc,
  install : false,
)

custom_target('doc',
  input : doxy_file,
  output : 'html',
  depend_files : [doc_layout_files, main, cmplib_sources],
  command : [doxygen, '@INPUT@'],
  build_by_default : false,
  console : true,
)
