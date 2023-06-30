cmake_minimum_required(VERSION 3.0)

project(match_for_engine)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")

aux_source_directory(../c-cchess-update C_SRCS)
aux_source_directory(../cpp-cchess-update CC_SRCS)

add_library(cchess_c ${C_SRCS})
add_library(cchess_cc ${CC_SRCS})

# add_executable(match_for_engine match_for_engine.cc)
# target_link_libraries(match_for_engine cchess_c cchess_cc) # tcmalloc)
#
# add_executable(match_for_stockfish match_for_stockfish.cc)
# target_link_libraries(match_for_stockfish cchess_c cchess_cc)

# add_executable(match_for_stockfish_and_eleeye match_for_stockfish_and_eleeye.cc)
# target_link_libraries(match_for_stockfish_and_eleeye cchess_c cchess_cc)

add_subdirectory(muduo-websocket)

add_executable(online_stockfish_engine online_stockfish_engine.cc)
target_link_libraries(online_stockfish_engine cchess_c cchess_cc wsun_websocket wsun_http muduo_net muduo_base jsoncpp pthread)
