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
  command : [checkpatch, checkpatch_args, main, common_sources, decompress_sources,
             icu_compress_sources, rdcu_compress_sources])
endif

# add cppcheck inspector target
cppcheck = find_program('cppcheck', required : false)
if cppcheck.found()
  cppcheck_args = [
    '--project=' + join_paths(meson.project_build_root(), 'compile_commands.json'),
    # '--clang',
    '--cppcheck-build-dir='+meson.current_build_dir(),
    '--std=c89',
    '--enable=all',
    '--inconclusive'
  ]
  run_target('cppcheck',
    command : [cppcheck, cppcheck_args]
  )
endif

subdir('tools')

subdir('cmp_tool')

unity_dep = dependency('unity', fallback : ['unity', 'unity_dep'])

subdir('test_common')
subdir('decmp')
subdir('cmp_icu')
subdir('cmp_decmp')
subdir('cmp_data_types')
subdir('cmp_entity')
subdir('cmp_rdcu_cfg')
subdir('cmp_max_used_bits')
