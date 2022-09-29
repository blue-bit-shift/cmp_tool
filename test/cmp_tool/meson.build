int_test_file = files('cmp_tool_integration_test.py')

pytest = find_program('pytest', required : false)
if pytest.found()
  test('Integration Test',
    pytest,
    args : ['--color=yes', '-vvv', int_test_file],
    depends : cmp_tool_exe,
    workdir : meson.project_build_root())
else
  message('Pytest framework not found! Skipping integration tests. Run pip install pytest.')
endif
