int_test_file = files('cmp_tool_integration_test.py')

pytest = find_program('pytest', required : false)
if pytest.found()
  test('cmp_tool Interface Test',
    pytest,
    args : ['--color=yes', '-vvv', int_test_file],
    env: test_env,
    depends : cmp_tool_exe,
    timeout : 100,
    workdir : meson.project_build_root())
else
  message('Pytest framework not found! Skipping integration tests. Run pip install pytest.')
endif
