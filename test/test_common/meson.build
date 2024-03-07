pcg_proj = subproject('pcg-c-basic')
pcb_dep = pcg_proj.get_variable('libpcg_basic_dep')

test_common_lib = static_library(
   'test_common',
   'test_common.c',
   dependencies: [pcb_dep, unity_dep]
)
