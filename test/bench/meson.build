bench_files = files(
  'bench.c',
  'benchfn.c',
  'timefn.c',
  'leon3_grtimer.c',
  'leon3_grtimer_longcount.c'
)

bench = executable('bench',
   bench_files,
   include_directories : incdir,
   link_with : cmp_lib,
   c_args : '-Wno-overlength-strings', # Set compiler flag to ignore warnings about overlength strings, which may improve compilation speed
   build_by_default : false
)

benchmark('bench',
  bench,
  timeout : 300,
  verbose : true
)
