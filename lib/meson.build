cmplib_sources = files([
  'cmp_data_types.c',
  'cmp_icu.c',
  'cmp_support.c',
  'rdcu_ctrl.c',
  'rmap.c',
  'cmp_entity.c',
  'cmp_io.c',
  'decmp.c',
  'rdcu_pkt_to_file.c',
  'cmp_guess.c',
  'cmp_rdcu_cfg.c',
  'cmp_rdcu.c',
  'rdcu_cmd.c',
  'rdcu_rmap.c',
  'cmp_max_used_bits.c',
  'cmp_max_used_bits_list.c'
])

cmp_lib = static_library('cmp_lib',
  sources : cmplib_sources,
  include_directories : incdir,
  c_args : ['-DDEBUGLEVEL=1'],
#  install : 'true' # linking under windows mingw only works if this is set
)
