test_case = files('test_cmp_icu.c')
test_runner = test_runner_generator.process(test_case)

test_cmp_icu = executable('test_cmp_icu',
   test_case, test_runner,
   include_directories : incdir,
   link_with : cmp_lib,
   dependencies : unity_dep,
   build_by_default : false
)

test('cmp_icu Unit Tests', test_cmp_icu)


test_case = files('test_decmp.c')
test_runner = test_runner_generator.process(test_case)

test_decmp = executable('test_decmp',
   test_case, test_runner,
   include_directories : incdir,
   link_with : cmp_lib,
   dependencies : unity_dep,
   build_by_default : false
)

test('Decompression Unit Tests', test_decmp)
