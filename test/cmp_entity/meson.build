test_case = files('test_cmp_entity.c')
test_runner = test_runner_generator.process(test_case)

test_cmp_entity = executable('test_cmp_entity',
   test_case, test_runner,
   include_directories : incdir,
   link_with : cmp_lib,
   dependencies : unity_dep,
   build_by_default : false
)

test('Compression Entity Unit Tests', test_cmp_entity)
