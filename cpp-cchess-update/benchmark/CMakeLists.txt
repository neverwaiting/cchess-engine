
add_executable(bench_generate_performance bench_generate_performance.cc)
target_link_libraries(bench_generate_performance cchess_cc benchmark pthread #[[ tcmalloc ]])
