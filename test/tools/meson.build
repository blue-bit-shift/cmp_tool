generate_test_runner = find_program('generate_test_runner.rb')
test_runner_generator = generator(
  generate_test_runner,
  output: ['@BASENAME@_Runner.c'],
  arguments: ['@INPUT@', '@OUTPUT@']
)
