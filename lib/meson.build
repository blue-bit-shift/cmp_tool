subdir('common')
subdir('decompress')
subdir('icu_compress')
subdir('rdcu_compress')


incdir = include_directories('.', 'common', 'decompress', 'icu_compress', 'rdcu_compress')

cmp_lib = static_library('cmp',
  sources : [ common_sources, decompress_sources, icu_compress_sources, rdcu_compress_sources]
)
