cmake_minimum_required(VERSION 3.0)

project(c-cchess)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -O2")

set(cchess_SRCS
	board.c
	search_engine.c
	player_piece.c
	rc4.c
	zobrist.c
	zobrist_position.c
  )

add_library(cchess_c ${cchess_SRCS})

add_subdirectory(test)
