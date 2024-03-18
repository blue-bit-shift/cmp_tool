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

test_cases = []
subdir('decmp')
subdir('cmp_icu')
subdir('cmp_decmp')
subdir('cmp_data_types')
subdir('cmp_entity')
subdir('cmp_rdcu_cfg')
subdir('cmp_max_used_bits')


test_args = '-Wno-missing-declarations' # The test runner generator does not generate header files
fs = import('fs')

foreach test : test_cases
  test_src = test[0]
  test_description = test[1]

  test_name = fs.name(test_src).split('.')[0]
  test_runner = test_runner_generator.process(test_src)
  test_libs = [cmp_lib]

  if test_name == 'test_cmp_decmp' or test_name == 'test_cmp_icu'
    test_libs += test_common_lib
  endif

  test_exe = executable(test_name,
    test_src, test_runner,
    include_directories : incdir,
    link_with : test_libs,
    dependencies : unity_dep,
    c_args : test_args,
    build_by_default : false
  )

  test(test_description, test_exe)
endforeach
