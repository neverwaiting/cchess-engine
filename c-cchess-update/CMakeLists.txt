cmake_minimum_required(VERSION 3.0)

project(c-cchess)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")

aux_source_directory(. CCHESS_SRCS)
add_library(cchess_c ${CCHESS_SRCS})

add_subdirectory(test)
# add_subdirectory(benchmark)
