test_case = files('test_cmp_max_used_bits_list.c')
test_runner = test_runner_generator.process(test_case)

test_cmp_max_used_bits_list = executable('test_cmp_max_used_bits_list',
   test_case, test_runner,
   include_directories : incdir,
   link_with : cmp_lib,
   dependencies : unity_dep,
   build_by_default : false
)

test('max_used_bits List Unit Tests', test_cmp_max_used_bits_list)
