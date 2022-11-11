test_case = files('test_cmp_rdcu_cfg.c')
test_runner = test_runner_generator.process(test_case)

test_cmp_rdcu_cfg = executable('test_cmp_rdcu_cfg',
   test_case, test_runner,
   include_directories : incdir,
   link_with : cmp_lib,
   dependencies : unity_dep,
   build_by_default : false
)

test('Hardware Compressor Configuration Tests', test_cmp_rdcu_cfg)
