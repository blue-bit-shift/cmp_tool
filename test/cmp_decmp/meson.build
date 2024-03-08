test_case = files('test_cmp_decmp.c')
test_runner = test_runner_generator.process(test_case)

test_cmp_decmp = executable('test_cmp_decmp',
   test_case, test_runner,
   include_directories : incdir,
   link_with : [cmp_lib, test_common_lib],
   dependencies : unity_dep,
   build_by_default : false
)

test('Compression Decompression Unit Tests', test_cmp_decmp)
