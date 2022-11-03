test_case = files('test_cmp_rdcu.c')
test_runner = test_runner_generator.process(test_case)

test_cmp_rdcu = executable('test_cmp_rdcu',
   test_case, test_runner,
   include_directories : incdir,
   link_with : cmp_lib,
   dependencies : unity_dep,
   build_by_default : false
)

test('Hardware Compressor Control Library Tests', test_cmp_rdcu)
