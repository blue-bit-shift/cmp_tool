project('doxygen-awesome-css',
  version: '2.2.0',
  meson_version: '>=0.54.0',
  license: 'MIT',
)

# Files to be installed
doxygen_awesome_files= files(
  'doxygen-awesome-darkmode-toggle.js',
  'doxygen-awesome-fragment-copy-button.js',
  'doxygen-awesome-interactive-toc.js',
  'doxygen-awesome-paragraph-link.js',
  'doxygen-awesome-sidebar-only-darkmode-toggle.css',
  'doxygen-awesome-sidebar-only.css',
  'doxygen-awesome.css'
)


doxygen_awesome_css_dep = declare_dependency(
  sources : doxygen_awesome_files,
  variables : {'doxygen_awesome_dir': meson.current_source_dir(), 'number': '3'}
)



