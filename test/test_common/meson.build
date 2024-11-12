pcg_proj = subproject('pcg-c-basic')
pcb_dep = pcg_proj.get_variable('libpcg_basic_dep')

chunk_round_trip = files('chunk_round_trip.c')
test_common_lib = static_library(
   'test_common',
   'test_common.c',
   chunk_round_trip,
   dependencies: [pcb_dep, unity_dep]
)
