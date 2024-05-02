if get_option('fuzzer').disabled()
  subdir_done()
endif

fuzz_common = files('fuzz_helpers.c', 'fuzz_data_producer.c')
fuzz_targets = ['fuzz_compression.c', 'fuzz_round_trip.c']

add_languages('cpp', native: false) # libFuzzingEngine needs c++

foreach target : fuzz_targets
  file_name = target
  target_name = file_name.split('.').get(0)

  fuzz_exe = executable(target_name,
    fuzz_common, file_name,
    include_directories : incdir,
    link_with : cmp_lib,
    link_args : get_option('fuzzer_ldflags'),
    link_language : 'cpp' # libFuzzingEngine needs c++
  )

   corpus_path = run_command('./download_corpus.sh', target_name, check: true).stdout().split()

  test(target_name + ' 10 min',
    fuzz_exe,
    args : ['-rss_limit_mb=2560', '-timeout=25', '-max_total_time=600', corpus_path],
    env : test_env,
    is_parallel : false,
    # suite : 'fuzzing',
    timeout : 605,
  )

  test(target_name + ' non stop',
    fuzz_exe,
    args : ['-rss_limit_mb=2560', '-timeout=25', corpus_path],
    env : test_env,
    is_parallel : false,
    # suite : 'fuzzing',
    timeout : 0,
    verbose : true
  )
endforeach
