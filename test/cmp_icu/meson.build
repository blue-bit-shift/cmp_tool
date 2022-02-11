test_cmp_icu_files = files ([
  'test_cmp_icu.c'
])

test_cmp_icu = executable('test_cmp_icu',
   test_cmp_icu_files,
   include_directories : incdir,
   link_with : cmp_lib,
   # c_args : ['-DMAIN']
   dependencies : cunit_dep,
   build_by_default : false
)

test('cmp_icu unit test', test_cmp_icu)
