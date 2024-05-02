int_test_file = files('cmp_tool_integration_test.py')

gen_test_data = executable('gen_test_data',
  'gen_test_data.c',
   c_args : '-Wno-overlength-strings',
)

test_data = custom_target('gen_test_data',
  output : ['ref_short_cadence_1_cmp.cmp', 'ref_short_cadence_2_cmp.cmp'],
  command : [gen_test_data, '@OUTPUT@']
)

pytest = find_program('pytest', required : false)
if pytest.found()
  test('cmp_tool Interface Test',
    pytest,
    args : ['--color=yes', '-vvv', int_test_file],
    env: test_env,
    depends : [cmp_tool_exe, test_data],
    timeout : 100,
    workdir : meson.project_build_root())
else
  message('Pytest framework not found! Skipping integration tests. Run pip install pytest.')
endif
