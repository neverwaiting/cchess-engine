cmake_minimum_required(VERSION 3.0)

project(cc-cchess)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")

set(cchess_SRCS
	board.cc
	search_engine.cc
  )
add_library(cchess_cc ${cchess_SRCS})

add_subdirectory(test)
