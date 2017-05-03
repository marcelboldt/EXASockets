cmake_minimum_required(VERSION 3.6)
project(EXASockets)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(../websockets)
add_subdirectory(exasockets)

add_executable(EXASockets main.cpp)
add_executable(exaresultset exaresultset.cpp)
add_executable(exaprepared exaprepared.cpp)
target_link_libraries(EXASockets exasockets)
target_link_libraries(exaresultset exasockets -lprofiler -ltcmalloc)
target_link_libraries(exaprepared exasockets)