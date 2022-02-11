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
  'cmp_rdcu.c',
  'rdcu_cmd.c',
  'rdcu_rmap.c'
])

cmp_lib = static_library('cmp_lib',
  sources : cmplib_sources,
  include_directories : incdir,
  c_args : ['-DDEBUGLEVEL=1']
)
