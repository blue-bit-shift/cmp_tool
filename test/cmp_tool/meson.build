int_test_file = files('cmp_tool_integration_test.py')

pytest = find_program('pytest', required : false)
test('Integration Test', pytest, args : ['--color=yes', '-vvv', int_test_file])
