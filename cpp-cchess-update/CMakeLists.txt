cmake_minimum_required(VERSION 3.0)

project(cc-cchess)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

aux_source_directory(. CCHESS_SRCS)
add_library(cchess_cc ${CCHESS_SRCS})

add_subdirectory(test)
add_subdirectory(benchmark)
