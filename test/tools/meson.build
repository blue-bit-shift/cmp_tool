ruby = find_program('ruby', required : false)
if ruby.found()
  generate_test_runner = find_program('generate_test_runner.rb')
  test_runner_generator = generator(generate_test_runner,
    output: ['@BASENAME@_Runner.c'],
    arguments: ['@INPUT@', '@OUTPUT@']
  )
endif

# add checkpatch syntax-check target
checkpatch = find_program('checkpatch.pl', 'checkpatch', required : false)
if checkpatch.found()
  checkpatch_args = [
    '--no-tree', '-f',
    '--show-types',
    '--color=always',
    '--ignore', 'SPDX_LICENSE_TAG,PREFER_DEFINED_ATTRIBUTE_MACRO,EMBEDDED_FILENAME,BLOCK_COMMENT_STYLE,EMBEDDED_FUNCTION_NAME',
  ]
  run_target('syntax-check',
  command : [checkpatch, checkpatch_args, cmp_tool_src, common_sources, decompress_sources, icu_compress_sources, rdcu_compress_sources])
endif

